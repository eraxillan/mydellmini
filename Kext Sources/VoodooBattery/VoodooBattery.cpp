/*
 --- VoodooBattery ---
 (C) 2009 Superhai
 
 Changelog
 ----------------------------------------------
 1.2.1  09/3/09, Modified by meklort
  - Updated to calculate rate if acpi gives a ones compimented value
  - Added cucle count for patched mini 9's that fall sback on the orig w/out a patched dsdt.
 1.2.0	19/1/09
  - Initial Release
 ----------------------------------------------
 
 Contact	http://www.superhai.com/
 
 */

#include "Support.h"
#include "VoodooBattery.h"

#pragma mark -
#pragma mark VoodooBattery Controller
#pragma mark -
#pragma mark IOService

OSDefineMetaClassAndStructors(VoodooBattery, IOService)

IOService * 
VoodooBattery::probe(IOService * provider, SInt32 * score) {
	
	// Call superclass
	if (IOService::probe(provider, score) != this) return 0;

	IORegistryIterator *	iterator;
	IORegistryEntry *		entry;
	OSString *				pnp;

	// We need to look for batteries and if not found we unload
	BatteryCount = 0;
	iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
	pnp = OSString::withCString(PnpDeviceIdBattery);
	if (iterator) {
		while (entry = iterator->getNextObject()) {
			if (entry->compareName(pnp)) {
				DebugLog("Found acpi pnp battery");
				BatteryDevice[BatteryCount++] = OSDynamicCast(IOACPIPlatformDevice, entry);
				if (BatteryCount >= MaxBatteriesSupported) break;
			}
		}
		iterator->release();
		iterator = 0;
	}
	
	DebugLog("Found %u batteries", BatteryCount);
	if (BatteryCount == 0) return 0;

	// We will also try to find an A/C adapter in acpi space
	AcAdapterCount = 0;
	iterator = IORegistryIterator::iterateOver(gIOACPIPlane, kIORegistryIterateRecursively);
	pnp = OSString::withCString(PnpDeviceIdAcAdapter);
	if (iterator) {
		while (entry = iterator->getNextObject()) {
			if (entry->compareName(pnp)) {
				DebugLog("Found acpi pnp ac adapter");
				AcAdapterDevice[AcAdapterCount++] = OSDynamicCast(IOACPIPlatformDevice, entry);
				if (AcAdapterCount >= MaxAcAdaptersSupported) break;
			}
		}
		iterator->release();
		iterator = 0;
	}

	DebugLog("Found %u ac adapters", AcAdapterCount);
	
	return this;
}

bool
VoodooBattery::start(IOService * provider) {
	
	// Call superclass
	if (!IOService::start(provider)) return false;

	// Printout banner
	InfoLog("%s %s (%s) %s %s [%s]",
			KextProductName,
			KextVersion,
			KextConfig,
			KextBuildDate,
			KextBuildTime,
			KextOSX);
	InfoLog("(C) 2009 Superhai, All Rights Reserved");
	
	for (UInt8 i = 0; i < BatteryCount; i++) {
		DebugLog("Attaching and starting battery %s", BatteryDevice[i]->getName());
		BatteryPowerSource[i] = VoodooBatteryPowerSource::NewBattery();
		if (BatteryPowerSource[i]) {
			if (BatteryPowerSource[i]->attach(BatteryDevice[i]) && BatteryPowerSource[i]->start(this)) {
				BatteryPowerSource[i]->registerService(0);
				BatteryPowerSource[i]->ParentService = this;
			} else {
				ErrorLog("Error on battery attach %u", i);
				return false;
			}
		}
	}
	
	for (UInt8 i = 0; i < AcAdapterCount; i++) {
		if (attach(AcAdapterDevice[i])) {
			DebugLog("A/C adapter %s available", AcAdapterDevice[i]->getName());
		}
	}
	
	WorkLoop = getWorkLoop();
	Poller = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action,
																			 this,
																			 &VoodooBattery::Update));
	if (!Poller || !WorkLoop) return false;
	if (WorkLoop->addEventSource(Poller) != kIOReturnSuccess) return false;
	
	PMinit();
	registerPowerDriver(this, PowerStates, 2);
	provider->joinPMtree(this);
	
	return true;
}

void
VoodooBattery::stop(IOService * provider) {
	Poller->cancelTimeout();
	PMstop();
	for (UInt8 i = 0; i < BatteryCount; i++) {
		BatteryPowerSource[i]->ParentService = 0;
		BatteryPowerSource[i]->detach(BatteryDevice[i]);
		BatteryPowerSource[i]->stop(this);
	}
	IOService::stop(provider);
}

IOReturn
VoodooBattery::setPowerState(unsigned long state, IOService * device) {
	if (state) {
		DebugLog("%s We are waking up", device->getName());
		QuickPoll = QuickPollCount;
		CheckDevices();
	} else {
		DebugLog("%s We are sleeping", device->getName());
		QuickPoll = 0;
	}
	return kIOPMAckImplied;
}

IOReturn
VoodooBattery::message(UInt32 type, IOService * provider, void * argument) {
	if (type == kIOACPIMessageDeviceNotification) {
		DebugLog("%s kIOACPIMessageDeviceNotification", provider->getName());
		QuickPoll = QuickPollCount;
		CheckDevices();
	} else {
		DebugLog("%s %08X", provider->getName(), type);
	}
	
	return kIOReturnSuccess;
}

#pragma mark Own

void
VoodooBattery::Update(void) {
	DebugLog("ExternalPowerConnected %x BatteriesAreFull %x", ExternalPowerConnected, BatteriesAreFull);
	if (ExternalPowerConnected && BatteriesAreFull) {
		DebugLog("NoPoll");
	} else {
		if (QuickPoll) {
			DebugLog("QuickPoll %u", QuickPoll);
			Poller->setTimeoutMS(QuickPollInterval);
			QuickPoll--;
		} else {
			DebugLog("NormalPoll");
			Poller->setTimeoutMS(NormalPollInterval);
		}
	}
	BatteriesAreFull = true;
	for (UInt8 i = 0; i < BatteryCount; i++) {
		if (BatteryConnected[i]) BatteryStatus(i);
		if (!AcAdapterCount) ExternalPowerConnected |= CalculatedAcAdapterConnected[i];
	}
	if (!AcAdapterCount) ExternalPower(ExternalPowerConnected);
}

void
VoodooBattery::CheckDevices(void) {
	UInt32 acpi = 0;
	bool change;
	Poller->cancelTimeout();
	BatteriesConnected = false;
	DebugLog("CheckDevices");
	for (UInt8 i = 0; i < BatteryCount; i++) {
		if (kIOReturnSuccess == BatteryDevice[i]->evaluateInteger(AcpiStatus, &acpi)) {
			change = BatteryConnected[i];
			BatteryConnected[i] = (acpi & 0x10) ? true : false;
			if (BatteryConnected[i] != change) BatteryInformation(i);
			BatteriesConnected |= BatteryConnected[i];
		}
	}
	ExternalPowerConnected = false;
	for (UInt8 i = 0; i < AcAdapterCount; i++) {
		if (kIOReturnSuccess == AcAdapterDevice[i]->evaluateInteger(AcpiPowerSource, &acpi)) {
			AcAdapterConnected[i] = acpi ? true : false;
			ExternalPowerConnected |= AcAdapterConnected[i];
		}
	}
	if (BatteriesConnected) {
		Update();
	} else {
		ExternalPowerConnected = true;		// Safe to assume without batteries you need ac
	}
	ExternalPower(ExternalPowerConnected);
	DebugLog("BatteriesConnected %x ExternalPowerConnected %x", BatteriesConnected, ExternalPowerConnected);
}

void
VoodooBattery::BatteryInformation(UInt8 battery) {
	PowerUnitIsWatt = false;
	if (BatteryConnected[battery]) {
		DebugLog("Battery %u Connected", battery);
		OSObject * acpi;
		if (kIOReturnSuccess == BatteryDevice[battery]->evaluateObject(AcpiBatteryInformation, &acpi)) {
			if (acpi && (OSTypeIDInst(acpi) == OSTypeID(OSArray))) {
				OSArray * info = OSDynamicCast(OSArray, acpi);
				if (GetValueFromArray(info, 0) == 0x00000000) PowerUnitIsWatt = true;
				Battery[battery].DesignCapacity										= GetValueFromArray(info, 1);
				Battery[battery].LastFullChargeCapacity								= GetValueFromArray(info, 2);
				Battery[battery].Technology											= GetValueFromArray(info, 3);
				Battery[battery].DesignVoltage										= GetValueFromArray(info, 4);
				Battery[battery].DesignCapacityWarning								= GetValueFromArray(info, 5);
				Battery[battery].DesignCapacityLow									= GetValueFromArray(info, 6);
				if (!Battery[battery].DesignVoltage) Battery[battery].DesignVoltage = DummyVoltage;
				if (PowerUnitIsWatt) {
					UInt32 volt = Battery[battery].DesignVoltage / 1000;
					if ((Battery[battery].DesignCapacity / volt) > 900) {
						WarningLog("Battery reports mWh but uses mAh (%u)",
								   Battery[battery].DesignCapacity / volt);
						PowerUnitIsWatt = false;
					} else {
						Battery[battery].DesignCapacity /= volt;
						Battery[battery].LastFullChargeCapacity /= volt;
						Battery[battery].DesignCapacityWarning /= volt;
						Battery[battery].DesignCapacityLow /= volt;
					}					
				}
				if (Battery[battery].DesignCapacity < Battery[battery].LastFullChargeCapacity) {
					WarningLog("Battery reports lower design capacity than maximum charged (%u/%u)",
							   Battery[battery].DesignCapacity, Battery[battery].LastFullChargeCapacity);
					if (Battery[battery].LastFullChargeCapacity < AcpiMax) {
						UInt32 temp = Battery[battery].DesignCapacity;
						Battery[battery].DesignCapacity = Battery[battery].LastFullChargeCapacity;
						Battery[battery].LastFullChargeCapacity = temp;
					}
				}
				// Publish to our IOKit powersource
				BatteryPowerSource[battery]->setMaxCapacity(Battery[battery].LastFullChargeCapacity);
				BatteryPowerSource[battery]->setDesignCapacity(Battery[battery].DesignCapacity);
				BatteryPowerSource[battery]->setExternalChargeCapable(true);
				BatteryPowerSource[battery]->setBatteryInstalled(true);
				BatteryPowerSource[battery]->setLocation(StartLocation + battery);
				BatteryPowerSource[battery]->setAdapterInfo(0);
				BatteryPowerSource[battery]->setDeviceName(GetSymbolFromArray(info, 9));
				BatteryPowerSource[battery]->setSerial(GetSymbolFromArray(info, 10));
				BatteryPowerSource[battery]->setSerialString(GetSymbolFromArray(info, 10));
				BatteryPowerSource[battery]->setManufacturer(GetSymbolFromArray(info, 12));
				if(info->getCount() > 13) BatteryPowerSource[battery]->setCycleCount(GetValueFromArray(info, 13));
				else if (Battery[battery].LastFullChargeCapacity && Battery[battery].DesignCapacity) {
					UInt32 last		= Battery[battery].LastFullChargeCapacity;
					UInt32 design	= Battery[battery].DesignCapacity;
					UInt32 cycle	= 2 * (10 - (last * 10 / design)) / 3;
					BatteryPowerSource[battery]->setCycleCount(cycle);
				}
				acpi->release();
			} else {
				WarningLog("Error in ACPI data");
				BatteryConnected[battery] = false;
			}
		}
	} else {
		DebugLog("Battery %u Disconnected", battery);
		BatteryPowerSource[battery]->BlankOutBattery();
		CalculatedAcAdapterConnected[battery] = false;
	}
	BatteryPowerSource[battery]->updateStatus();
}

void
VoodooBattery::BatteryStatus(UInt8 battery) {
	OSObject * acpi;
	if (kIOReturnSuccess == BatteryDevice[battery]->evaluateObject(AcpiBatteryStatus, &acpi)) {
		if (acpi && (OSTypeIDInst(acpi) == OSTypeID(OSArray))) {
			OSArray * status = OSDynamicCast(OSArray, acpi);
#if DEBUG 
			setProperty(BatteryDevice[battery]->getName(), status);
#endif
			UInt32 TimeRemaining = 0;
			//UInt32 HighAverageBound, LowAverageBound; 
 			bool warning = false;
			bool critical = false;
			bool bogus = false;
			uint64_t now = mach_absolute_time();
			
			

			Battery[battery].State = GetValueFromArray(status, 0);

			// In case of a state change, reset stuff
			if(Battery[battery].State ^ GetValueFromArray(status, 0)) {	// We changed battery states, reset all averages
				TimeSinceStateChange = now;

				Battery[battery].AverageRate = 0;
				Battery[battery].PresentRate = 0;
				Poller->setTimeoutMS(StateChangeInterval);	// Overwrite teh quickpoll
				//UInt32 interval = QuickPoll ? 3600 / (StateChangeInterval / 1000) : 3600 / (NormalPollInterval / 1000);
				Battery[battery].LastRemainingCapacity = 0;
				//switch (Battery[battery].State & 0x3) {
				//	case BatteryFullyCharged:
				BatteryPowerSource[battery]->setTimeRemaining(0xFFFF);	// tell the os we are calculating
				if ((Battery[battery].State & 0x3) == BatteryDischarging) {
					ExternalPower(false);
					CalculatedAcAdapterConnected[battery] = false;
				} else {
					ExternalPower(true);
					CalculatedAcAdapterConnected[battery] = true;
				}

				
				BatteryPowerSource[battery]->rebuildLegacyIOBatteryInfo();
				BatteryPowerSource[battery]->updateStatus();
				acpi->release();
			} else {
				Battery[battery].PresentRate = GetValueFromArray(status, 1);
			}
			
			if(now < (TimeSinceStateChange + SettleTime)) {
				acpi->release();
				return;
			}
			
			
			Battery[battery].RemainingCapacity = GetValueFromArray(status, 2);
			Battery[battery].PresentVoltage = GetValueFromArray(status, 3);
			
			Battery[battery].AverageRate = Battery[battery].PresentRate;
			
			if (PowerUnitIsWatt) {
				UInt32 volt = Battery[battery].DesignVoltage / 1000;
				Battery[battery].PresentRate /= volt;
				Battery[battery].RemainingCapacity /= volt;
			}
			
			/*
			// Average rate calculation
			if (!Battery[battery].PresentRate || (Battery[battery].PresentRate == AcpiUnknown)) {
				UInt32 delta = (Battery[battery].RemainingCapacity > Battery[battery].LastRemainingCapacity ?
								Battery[battery].RemainingCapacity - Battery[battery].LastRemainingCapacity :
								Battery[battery].LastRemainingCapacity - Battery[battery].RemainingCapacity);
				UInt32 interval = QuickPoll ? 3600 / (QuickPollInterval / 1000) : 3600 / (NormalPollInterval / 1000);
				// Check if we really want this here
				Battery[battery].PresentRate = delta ? delta * interval : interval;
			}
			if (!Battery[battery].AverageRate) Battery[battery].AverageRate = Battery[battery].PresentRate;
			Battery[battery].AverageRate += Battery[battery].PresentRate;
			//Battery[battery].AverageRate >>= 1;
			HighAverageBound = Battery[battery].PresentRate * (100 + AverageBoundPercent) / 100;
			LowAverageBound  = Battery[battery].PresentRate * (100 - AverageBoundPercent) / 100;
			if (Battery[battery].AverageRate > HighAverageBound) {
				Battery[battery].AverageRate = HighAverageBound;
			}
			if (Battery[battery].AverageRate < LowAverageBound) {
				Battery[battery].AverageRate = LowAverageBound;
			}
			
			*/
			

			// Voltage
			if (!Battery[battery].PresentVoltage || (Battery[battery].PresentVoltage == AcpiUnknown)) {
				Battery[battery].PresentVoltage = Battery[battery].DesignVoltage;
			}
			
			// Check battery state
			switch (Battery[battery].State & 0x3) {
				case BatteryFullyCharged:
					DebugLog("Battery %u Full", battery);
					ExternalPower(true);

					CalculatedAcAdapterConnected[battery] = true;
					//BatteriesAreFull &= true;
					BatteriesAreFull = true;
					BatteryPowerSource[battery]->setIsCharging(false);
					BatteryPowerSource[battery]->setFullyCharged(true);
					if ((Battery[battery].LastRemainingCapacity >= Battery[battery].DesignCapacity) ||
						(Battery[battery].LastRemainingCapacity == 0)) {
						BatteryPowerSource[battery]->setCurrentCapacity(Battery[battery].LastFullChargeCapacity);
					} else {
						BatteryPowerSource[battery]->setCurrentCapacity(Battery[battery].LastRemainingCapacity);
					}
					if(((SInt32) Battery[battery].PresentRate) & 0x8000) BatteryPowerSource[battery]->setInstantAmperage(((SInt32) (0xFFFF - Battery[battery].PresentRate)));
					else BatteryPowerSource[battery]->setInstantAmperage((SInt32) Battery[battery].PresentRate);
					if(((SInt32) Battery[battery].AverageRate) & 0x8000) BatteryPowerSource[battery]->setAmperage(((SInt32) (0xFFFF - Battery[battery].AverageRate)));
					else BatteryPowerSource[battery]->setAmperage((SInt32) Battery[battery].AverageRate);
					break;
				case BatteryDischarging:
					DebugLog("Battery %u Discharging", battery);
					ExternalPower(false);
					CalculatedAcAdapterConnected[battery] = false;
					BatteriesAreFull = false;
					BatteryPowerSource[battery]->setIsCharging(false);
					BatteryPowerSource[battery]->setFullyCharged(false);
					BatteryPowerSource[battery]->setCurrentCapacity(Battery[battery].RemainingCapacity);
					if(((SInt32) Battery[battery].PresentRate) & 0x8000) BatteryPowerSource[battery]->setInstantAmperage(-1 * ((SInt32) (0xFFFF - Battery[battery].PresentRate)));
					else BatteryPowerSource[battery]->setInstantAmperage((SInt32) (0x0000));
					if(((SInt32) Battery[battery].AverageRate) & 0x8000) BatteryPowerSource[battery]->setAmperage(-1 * ((SInt32) (0xFFFF - Battery[battery].AverageRate)));
					else BatteryPowerSource[battery]->setAmperage((SInt32) (0x0000));
					break;
				case BatteryCharging:
					ExternalPower(true);
					DebugLog("Battery %u Charging", battery);
					CalculatedAcAdapterConnected[battery] = true;
					BatteriesAreFull = false;
					BatteryPowerSource[battery]->setIsCharging(true);
					BatteryPowerSource[battery]->setFullyCharged(false);
					BatteryPowerSource[battery]->setCurrentCapacity(Battery[battery].RemainingCapacity);
					if(((SInt32) Battery[battery].PresentRate) & 0x8000) BatteryPowerSource[battery]->setInstantAmperage(((SInt32) (0xFFFF - Battery[battery].PresentRate)));
					else BatteryPowerSource[battery]->setInstantAmperage((SInt32) Battery[battery].PresentRate);
					if(((SInt32) Battery[battery].AverageRate) & 0x8000) BatteryPowerSource[battery]->setAmperage(((SInt32) (0xFFFF - Battery[battery].AverageRate)));
					else BatteryPowerSource[battery]->setAmperage((SInt32) Battery[battery].AverageRate);
					break;
				default:
					WarningLog("Bogus status data from battery %u (%x)", battery, Battery[battery].State);
					BatteriesAreFull = false;
					BatteryPowerSource[battery]->setIsCharging(false);
					BatteryPowerSource[battery]->setFullyCharged(false);
					BatteryPowerSource[battery]->setCurrentCapacity(Battery[battery].RemainingCapacity);
					bogus = true;
					break;
			}
			
			// Remaining capacity
			if (!Battery[battery].RemainingCapacity || (Battery[battery].RemainingCapacity == AcpiUnknown)) {
				WarningLog("Battery %u has no remaining capacity reported", battery);
			} else {
				TimeRemaining = (Battery[battery].AverageRate ?
								 60 * Battery[battery].RemainingCapacity / Battery[battery].AverageRate :
								 0xffff);
				BatteryPowerSource[battery]->setTimeRemaining(TimeRemaining);
				TimeRemaining = (Battery[battery].PresentRate ?
								 60 * Battery[battery].RemainingCapacity / Battery[battery].PresentRate :
								 0xffff);
				BatteryPowerSource[battery]->setInstantaneousTimeToEmpty(TimeRemaining);
			}
			
			warning		= Battery[battery].RemainingCapacity <= Battery[battery].DesignCapacityWarning;
			critical	= Battery[battery].RemainingCapacity <= Battery[battery].DesignCapacityLow;
			if (Battery[battery].State & BatteryCritical) {
				DebugLog("Battery %u is critical", battery);
				critical = true;
			}
			BatteryPowerSource[battery]->setAtWarnLevel(warning);
			BatteryPowerSource[battery]->setAtCriticalLevel(critical);
			BatteryPowerSource[battery]->setVoltage(Battery[battery].PresentVoltage);
			if (critical && bogus) {
				BatteryPowerSource[battery]->setErrorCondition((OSSymbol *) permanentFailureKey);				
			}
			BatteryPowerSource[battery]->rebuildLegacyIOBatteryInfo();
			BatteryPowerSource[battery]->updateStatus();
			acpi->release();
		} else {
			WarningLog("Error in ACPI data");
			BatteryConnected[battery] = false;
		}
	}
	Battery[battery].LastRemainingCapacity = Battery[battery].RemainingCapacity;
}

void
VoodooBattery::ExternalPower(bool status) {
	for (UInt8 i = 0; i < BatteryCount; i++) {
		BatteryPowerSource[i]->setExternalConnected(status);
	}
	getPMRootDomain()->receivePowerNotification(kIOPMSetACAdaptorConnected | (kIOPMSetValue * status));
}

#pragma mark -
#pragma mark VoodooBattery PowerSource Device
#pragma mark -
#pragma mark IOPMPowerSource

OSDefineMetaClassAndStructors(VoodooBatteryPowerSource, IOPMPowerSource)

IOReturn
VoodooBatteryPowerSource::message(UInt32 type, IOService * provider, void * argument) {

	if (ParentService) ParentService->message(type, provider, argument);
	return kIOReturnSuccess;
}

#pragma mark Own

VoodooBatteryPowerSource *
VoodooBatteryPowerSource::NewBattery(void) {
	
	// Create and initialize our powersource
	VoodooBatteryPowerSource * battery = new VoodooBatteryPowerSource;
	if (battery) {
		if (battery->init()) return battery;
		battery->release();
	}
	return 0;
}

void VoodooBatteryPowerSource::BlankOutBattery(void) {
	setBatteryInstalled(false);
	setCycleCount(0);
	setAdapterInfo(0);
	setIsCharging(false);
	setCurrentCapacity(0);
	setMaxCapacity(0);
	setTimeRemaining(0);
	setAmperage(0);
	setVoltage(0);
	properties->removeObject(manufacturerKey);
	removeProperty(manufacturerKey);
	properties->removeObject(serialKey);
	removeProperty(serialKey);
	properties->removeObject(batteryInfoKey);
	removeProperty(batteryInfoKey);
	properties->removeObject(errorConditionKey);
	removeProperty(errorConditionKey);
	properties->removeObject(chargeStatusKey);
	removeProperty(chargeStatusKey);
	rebuildLegacyIOBatteryInfo();
}

void VoodooBatteryPowerSource::setDesignCapacity(unsigned int val)
{
    OSNumber *n = OSNumber::withNumber(val, 32);
    setPSProperty(designCapacityKey, n);
    n->release();
}

void VoodooBatteryPowerSource::setDeviceName(OSSymbol * sym)
{
    if (sym)
        setPSProperty(deviceNameKey, (OSObject *) sym);
}

void VoodooBatteryPowerSource::setFullyCharged(bool charged)
{
    setPSProperty( fullyChargedKey, 
				  (charged ? kOSBooleanTrue:kOSBooleanFalse) );
}

void VoodooBatteryPowerSource::setInstantAmperage(int mA)
{
    OSNumber *n = OSNumber::withNumber(mA, 32);
    if (n) {
        setPSProperty(instantAmperageKey, n);
        n->release();
    }
}

void VoodooBatteryPowerSource::setInstantaneousTimeToEmpty(int seconds)
{
    OSNumber *n = OSNumber::withNumber(seconds, 32);
    if (n) {
        setPSProperty(instantTimeToEmptyKey, n);
        n->release();
    }
}

void VoodooBatteryPowerSource::setSerialString(OSSymbol * sym)
{
	if (sym)
        setPSProperty(softwareSerialKey, (OSObject *) sym);
}

void VoodooBatteryPowerSource::rebuildLegacyIOBatteryInfo(void)
{
    OSDictionary        *legacyDict = OSDictionary::withCapacity(5);
    uint32_t            flags = 0;
    OSNumber            *flags_num = NULL;
    
    if(externalConnected()) flags |= kIOPMACInstalled;
    if(batteryInstalled()) flags |= kIOPMBatteryInstalled;
    if(isCharging()) flags |= kIOPMBatteryCharging;
    
    flags_num = OSNumber::withNumber((unsigned long long)flags, 32);
    legacyDict->setObject(kIOBatteryFlagsKey, flags_num);
    flags_num->release();
	
    legacyDict->setObject(kIOBatteryCurrentChargeKey, properties->getObject(kIOPMPSCurrentCapacityKey));
    legacyDict->setObject(kIOBatteryCapacityKey, properties->getObject(kIOPMPSMaxCapacityKey));
    legacyDict->setObject(kIOBatteryVoltageKey, properties->getObject(kIOPMPSVoltageKey));
    legacyDict->setObject(kIOBatteryAmperageKey, properties->getObject(kIOPMPSAmperageKey));
    legacyDict->setObject(kIOBatteryCycleCountKey, properties->getObject(kIOPMPSCycleCountKey));
    
    setLegacyIOBatteryInfo(legacyDict);
    
    legacyDict->release();
}
