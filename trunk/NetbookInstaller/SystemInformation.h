//
//  SystemInformation.h
//  NetbookInstaller
//
//  Created by Evan Lojewski on 5/15/09.
//  Copyright 2009. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <CoreServices/CoreServices.h>

#import <sys/stat.h>
#import <unistd.h>

struct uint128 {
	UInt64 upper;
	UInt64 lower;
};


#define BOOTLOADER_CHAMELEON_R431		0xd593439fcf1479a403054db491c0e928
#define BOOTLOADER_PCEFI_V9				0xa17b6a642a53a661bed651c83be369ed

#define MINI9_BLUETOOTH_VENDOR			16700
#define MINI9_BLUETOOTH_DEVICE			668

#define MINI10V_BLUETOOTH_VENDOR		0
#define MINI10V_BLUETOOTH_DEVICE		0

#define S10_BLUETOOTH_VENDOR			0
#define S10_BLUETOOTH_DEVICE			0

enum machine { MINI9, VOSTRO_A90, MINI10V, LENOVO_S10, UNKNOWN};
enum bootloader { CHAMELEON_R431, PCEFIV9, NONE};
enum scrollMethod { MEKLORT, VOODOO, FFSCROLL };



@interface SystemInformation : NSObject {
	enum machine		machineType;
	enum scrollMethod	twoFingerScrolling;
	enum bootloader		installedBootloader;

	
	int		efiVersion;
	int		netbookInstallerVersion;
	
	NSUInteger	bluetoothVendorId;
	NSUInteger	bluetoothDeviceId;
	
	bool		dsdtInstalled;
	bool		keyboardPrefPaneInstalled;
	bool		remoteCDEnabled;
	bool		hibernationDissabled;
	bool		quietBoot;
	bool		bluetoothPatched;
	bool		mirrorFriendlyGMA;
	bool		efiHidden;
	
	NSString*	extensionsFolder;
	NSString*	bootPartition;

}

- (bool) dsdtInstalled;
- (bool) keyboardPrefPaneInstalled;
- (bool) remoteCDEnabled;
- (bool) hibernationDissabled;
- (NSString*) bootPartition;
- (NSString*) extensionsFolder;
- (bool) quietBoot;
- (bool) bluetoothPatched;
- (bool) mirrorFriendlyGMA;
- (bool) efiHidden;
- (enum bootloader) installedBootloader;
- (enum machine) machineType;
- (void) machineType: (enum machine) newMachineType;

- (NSUInteger) bluetoothVendorId;
- (NSUInteger) bluetoothDeviceId;

- (void) determineInstallState;
- (void) determineMachineType;
- (void) determinebootPartition;
- (void) determineDSDTState;
- (void) determineRemoteCDState;
- (void) determineHibernateState;
- (void) determineQuiteBootState;
- (void) determineGMAVersion;
- (void) determineHiddenState;
- (void) determineBluetoothState;
- (void) determinekeyboardPrefPaneInstalled;
- (void) determineBootloader;



@end
