/*
 --- VoodooBattery ---
 (C) 2009 Superhai
 
 Contact	http://www.superhai.com/
 
 */

#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/pwr_mgt/IOPMPowerSource.h>
#include <IOKit/pwr_mgt/RootDomain.h>
#include <IOKit/IOTimerEventSource.h>

// Constants

const UInt8	MaxBatteriesSupported = 16;
const UInt8 MaxAcAdaptersSupported = 4;
const UInt8	AverageBoundPercent = 25;

const UInt32	QuickPollInterval =		  1000;
const UInt32	NormalPollInterval =	 60000;
const UInt32	QuickPollPeriod =		 60000;
const UInt32	QuickPollCount = QuickPollPeriod / QuickPollInterval;

const UInt32	AcpiUnknown =		0xFFFFFFFF;
const UInt32	AcpiMax =			0x80000000;

const UInt32	DummyVoltage =			 12000;


const UInt32	StartLocation = kIOPMPSLocationLeft;

// String constants

const char *	PnpDeviceIdBattery		= "PNP0C0A";
const char *	PnpDeviceIdAcAdapter	= "ACPI0003";
const char *	AcpiStatus				= "_STA";
const char *	AcpiPowerSource			= "_PSR";
const char *	AcpiBatteryInformation	= "_BIF";
const char *	AcpiBatteryStatus		= "_BST";


static const OSSymbol * unknownObjectKey		= OSSymbol::withCString("");
static const OSSymbol * designCapacityKey		= OSSymbol::withCString(kIOPMPSDesignCapacityKey);
static const OSSymbol * deviceNameKey			= OSSymbol::withCString(kIOPMDeviceNameKey);
static const OSSymbol * fullyChargedKey			= OSSymbol::withCString(kIOPMFullyChargedKey);
static const OSSymbol * instantAmperageKey		= OSSymbol::withCString("InstantAmperage");
static const OSSymbol * instantTimeToEmptyKey	= OSSymbol::withCString("InstantTimeToEmpty");
static const OSSymbol * softwareSerialKey		= OSSymbol::withCString("BatterySerialNumber");
static const OSSymbol * chargeStatusKey			= OSSymbol::withCString("ChargeStatus");
static const OSSymbol * permanentFailureKey		= OSSymbol::withCString("Permanent Battery Failure");

static IOPMPowerState PowerStates[2] = 
{ {1,0,0,0,0,0,0,0,0,0,0,0}, {1,2,2,2,0,0,0,0,0,0,0,0} };

enum {																								// Apple loves to have the goodies private
    kIOPMSetValue				= (1<<16),
    kIOPMSetDesktopMode			= (1<<17),
    kIOPMSetACAdaptorConnected	= (1<<18)
};

enum {
	BatteryFullyCharged	= 0,
	BatteryDischarging	= (1<<0),
	BatteryCharging		= (1<<1),
	BatteryCritical		= (1<<2),
//	BatteryWithAc		= (1<<7)
};

struct BatteryClass {
	UInt32	LastFullChargeCapacity;
	UInt32	DesignCapacity;
	UInt32	DesignCapacityWarning;
	UInt32	DesignCapacityLow;
	UInt32	DesignVoltage;
	UInt32	Technology;
	UInt32	State;
	UInt32	PresentVoltage;
	UInt32	PresentRate;
	UInt32	AverageRate;
	UInt32	RemainingCapacity;
	UInt32	LastRemainingCapacity;
};

class VoodooBatteryPowerSource : public IOPMPowerSource {
	OSDeclareDefaultStructors(VoodooBatteryPowerSource)
private:
protected:
	IOService *	ParentService;
	void	BlankOutBattery(void);
	void	setDesignCapacity(unsigned int val);
	void	setDeviceName(OSSymbol * sym);
	void	setFullyCharged(bool charged);
	void	setInstantAmperage(int mA);
	void	setInstantaneousTimeToEmpty(int seconds);
	void	setSerialString(OSSymbol * sym);
	void	rebuildLegacyIOBatteryInfo(void);
public:
	static	VoodooBatteryPowerSource * NewBattery(void);
	virtual IOReturn	message(UInt32 type, IOService * provider, void * argument);
	friend class VoodooBattery;
};

class VoodooBattery : public IOService {
	OSDeclareDefaultStructors(VoodooBattery)
private:
	// *** Integers ***
	UInt8	BatteryCount;
	UInt8	AcAdapterCount;
	UInt32	QuickPoll;
	// *** Booleans ***
	bool	BatteryConnected[MaxBatteriesSupported];
	bool	AcAdapterConnected[MaxAcAdaptersSupported];
	bool	CalculatedAcAdapterConnected[MaxBatteriesSupported];
	bool	ExternalPowerConnected;
	bool	BatteriesConnected;
	bool	BatteriesAreFull;
	bool	PowerUnitIsWatt;
	// *** Other ***
	BatteryClass				Battery[MaxBatteriesSupported];
	IOACPIPlatformDevice *		BatteryDevice[MaxBatteriesSupported];
	IOACPIPlatformDevice *		AcAdapterDevice[MaxAcAdaptersSupported];
	VoodooBatteryPowerSource *	BatteryPowerSource[MaxBatteriesSupported];
	IOWorkLoop *				WorkLoop;
	IOTimerEventSource *		Poller;
	// *** Methods ***
	void	Update(void);
	void	CheckDevices(void);
	void	BatteryInformation(UInt8 battery);
	void	BatteryStatus(UInt8 battery);
	void	ExternalPower(bool status);
public:
	// *** IOService ***
	virtual	IOService *	probe(IOService * provider, SInt32 * score);
	virtual	bool		start(IOService * provider);
	virtual void		stop(IOService * provider);
	virtual IOReturn	setPowerState(unsigned long state, IOService * device);
	virtual IOReturn	message(UInt32 type, IOService * provider, void * argument);
};

UInt32
GetValueFromArray(OSArray * array, UInt8 index) {
	OSObject * object = array->getObject(index);
	if (object && (OSTypeIDInst(object) == OSTypeID(OSNumber))) {
		OSNumber * number = OSDynamicCast(OSNumber, object);
		if (number) return number->unsigned32BitValue();
	}
	return -1;
}

OSSymbol *
GetSymbolFromArray(OSArray * array, UInt8 index) {
	OSObject * object = array->getObject(index);
	if (object && (OSTypeIDInst(object) == OSTypeID(OSData))) {
		OSData * data = OSDynamicCast(OSData, object);
		if (data->appendByte(0x00, 1)) {
			return (OSSymbol *) OSSymbol::withCString((const char *) data->getBytesNoCopy());
		}
	}
	if (object && (OSTypeIDInst(object) == OSTypeID(OSString))) {
		OSString * string = OSDynamicCast(OSString, object);
		if (string) return OSDynamicCast(OSSymbol, string);
	}
	return (OSSymbol *) unknownObjectKey;
}