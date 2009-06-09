//
//  Installer.m
//  NetbookInstaller
//
//  Created by Evan Lojewski on 5/16/09.
//  Copyright 2009. All rights reserved.
//

#import "Installer.h"

@implementation Installer

- (id) initWithSender: (id) parent
{
	[self init];
	
	sender = parent;
	return self;
}
- (BOOL) performInstall: (SystemInformation*) sysInfo
{
	systemInfo = sysInfo;
	if(![self getAuthRef]) return NO;

	// FIXME: [sender updateBlah] doesnt seam to hapen instantly. Since the install is quick, we are unable to show reliable data to the user
	
	
	[sender updatePorgressBar: 0];
	
	[sender updateStatus:NSLocalizedString(@"Creating /Extra", nil)];
	[self installExtraFiles];
	[sender updatePorgressBar: 3];
	
	//return YES;

	[sender updateStatus:NSLocalizedString(@"Installing Display Profiles", nil)];
	[self installDisplayProfile];
	[sender updatePorgressBar: 1];

	[sender updateStatus:NSLocalizedString(@"Installing Preference Panes", nil)];
	[self installPrefPanes];
	[sender updatePorgressBar: 1];
	
	if([sender install1055PrefPane])
	{
		[sender updateStatus:NSLocalizedString(@"Installing System Preference Panes", nil)];
		[self installSystemPrefPanes];
	}
	[sender updatePorgressBar: 1];
	

	[sender updateStatus:NSLocalizedString(@"Instaling Power Managment bundle", nil)];
	[self installSystemConfiguration];
	[sender updatePorgressBar: 1];
	
	if([sender regenerateDSDT]) [self installDSDT];

	
	if([sender installExtensions]){
		[sender updateStatus:NSLocalizedString(@"Installing Extensions", nil)];
		[self installExtensions];
		[sender updatePorgressBar: 24];

		if([sender mirrorFriendlyGMA]) 
		{
			[sender installMirrorFriendlyGraphics];
		}
		else 
		{
			[sender updateStatus:NSLocalizedString(@"Patching GMA950 Extension", nil)];
			[self patchGMAkext];
			[sender updatePorgressBar: 5];
		
			[sender updateStatus:NSLocalizedString(@"Patching Framebuffer Extension", nil)];
			[self patchFramebufferKext];
			[sender updatePorgressBar: 5];
		}
		
		[sender updateStatus:NSLocalizedString(@"Patching Wireless Extension", nil)];
		[self patchIO80211kext];
		[sender updatePorgressBar: 5];
		
		[sender updateStatus:NSLocalizedString(@"Patching Bluetooth", nil)];
		[self patchBluetooth];
		[sender updatePorgressBar: 5];
		

		[sender updateStatus:NSLocalizedString(@"Copying Extension dependencies", nil)];
		[self copyDependencies];
		[sender updatePorgressBar: 5];
		

	}

	[sender updateStatus:NSLocalizedString(@"Verifying Quiet Boot state", nil)];
	[self setQuietBoot:			[sender enableQuietBoot]];
	
	return YES;

	
	[sender updateStatus:NSLocalizedString(@"Verifying Hibernation state", nil)];
	[self dissableHibernation:	[sender dissableHibernation]];
	
	[sender updateStatus:NSLocalizedString(@"Verifying RemoteCD State", nil)];
	[self setRemoteCD:			[sender enableRemoteCD]]; // TODO: code this
	[sender updatePorgressBar: 5];

	[sender updateStatus:NSLocalizedString(@"Verifying Bootloader", nil)];
	if([sender bootloaderType] != NONE) [self installBootloader];
	[sender updatePorgressBar: 10];

	// These funcitons have not been coded yet
	if([sender hideFiles]) {
		if([systemInfo efiHidden])  [self showFiles];
		else						[self hideFiles];
	}
	
	if([sender fixBluetooth]) [self fixBluetooth];
	[sender updatePorgressBar: 30];
	
	[sender updateStatus:NSLocalizedString(@"Complete", nil)];


	
	

	
	
	return NO;
}

- (BOOL) getAuthRef
{
	AuthorizationRef authorizationRef;
	
    AuthorizationItem right = { "com.mydellmini.Installer", 0, NULL, 0 };
	AuthorizationItem admin = { kAuthorizationRightExecute, 0, NULL, 0};
	AuthorizationItem rights[2];
	rights[0] = right;
	rights[1] = admin;
    AuthorizationRights rightSet = { 2, rights };
	OSStatus status;
	//MyAuthorizedCommand myCommand;
    AuthorizationFlags flags = kAuthorizationFlagDefaults | kAuthorizationFlagPreAuthorize | kAuthorizationFlagInteractionAllowed | kAuthorizationFlagExtendRights;
	
	
	/// Create a new authorization reference which will later be passed to the tool.
	status = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, flags, &authorizationRef);
	if (status != errAuthorizationSuccess)
	{
		//NSLog(@"RDPPFramework NSFileManager+RDPPFrameworkAdditions secureCopy:toPath:authenticate: failed to create AuthorizationRef.  Return code was %d", status);
		return NO;
	}	
	
	// We only use this were we are copying to /Library so always athorize.
	status = AuthorizationCopyRights(authorizationRef, &rightSet, kAuthorizationEmptyEnvironment, flags, NULL);
	if (status!=errAuthorizationSuccess)
	{
		//NSLog(@"RDPPFramework NSFileManager+RDPPFrameworkAdditions secureCopy:toPath:authenticate: failed to authorize.  Return code was %d", status);
		return NO;
	}
	
	authRef = authorizationRef;
	
	return YES;
}



- (BOOL) copyFrom: (NSString*) source toDir: (NSString*) destination
{
	OSStatus status;
	
	char* args[4];
	//args[0] = "-Rfp";
	args[0] = "-Rf";	// Specify group wheel
	
	args[1] = (char*) [source cStringUsingEncoding:NSASCIIStringEncoding];
	args[2] = (char*) [destination cStringUsingEncoding:NSASCIIStringEncoding];
	args[3] = NULL;
	
	
	//NSLog(@"/bin/cp %s %s %s\n", args[0], args[1], args[2]);
	
	status = AuthorizationExecuteWithPrivileges(authRef, "/bin/cp", kAuthorizationFlagDefaults, args, nil);
	
	if(status == 0) return YES;
	else return NO;
}

- (BOOL) hidePath: (NSString*) path
{
	OSStatus status;
	
	char* args[3];
	//args[0] = "-Rfp";
	args[0] = "hidden";	// Specify group wheel
	
	args[1] = (char*) [path cStringUsingEncoding:NSASCIIStringEncoding];
	args[2] = NULL;
	
	
	//NSLog(@"/bin/cp %s %s %s\n", args[0], args[1], args[2]);
	
	status = AuthorizationExecuteWithPrivileges(authRef, "/usr/bin/chflags", kAuthorizationFlagDefaults, args, nil);
	
	if(status == 0) return YES;
	else return NO;
}

- (BOOL) showPath: (NSString*) path
{
	OSStatus status;
	
	char* args[3];
	//args[0] = "-Rfp";
	args[0] = "nohidden";	// Specify group wheel
	
	args[1] = (char*) [path cStringUsingEncoding:NSASCIIStringEncoding];
	args[2] = NULL;
	
	
	//NSLog(@"/bin/cp %s %s %s\n", args[0], args[1], args[2]);
	
	status = AuthorizationExecuteWithPrivileges(authRef, "/usr/bin/chflags", kAuthorizationFlagDefaults, args, nil);
	
	if(status == 0) return YES;
	else return NO;
}


- (BOOL) setPermissions: (UInt8) perms onPath: (NSString*) path recursivly: (BOOL) recursiv
{
	return NO;
}

- (BOOL) setOwner: (NSString*) owner andGroup: (NSString*) group OnPath: (NSString*) path recursivly: (BOOL) recursiv
{
	return NO;
}


- (BOOL) makeDir: (NSString*) dir
{
	OSStatus status;
	
	char* args[3];
	//args[0] = "-d";
	args[0] = "-p";	// TODO: Specify group wheel, user root
	
	args[1] = (char*) [dir cStringUsingEncoding:NSASCIIStringEncoding];
	args[2] = NULL;
	
	
	//NSLog(@"/bin/mkdir %s %s %s\n", args[0], args[1], args[2]);
	
	status = AuthorizationExecuteWithPrivileges(authRef, "/bin/mkdir", kAuthorizationFlagDefaults, args, nil);
	
	if(status == 0) return YES;
	else return NO;
}

-(BOOL) deleteFile: (NSString*) file
{
	// TODO: sanity check
	OSStatus status;
	
	char* args[3];
	//args[0] = "-d";
	args[0] = "-rf";	// TODO: make this configurable
	
	args[1] = (char*) [file cStringUsingEncoding:NSASCIIStringEncoding];
	args[2] = NULL;
	
		
	status = AuthorizationExecuteWithPrivileges(authRef, "/bin/rm", kAuthorizationFlagDefaults, args, nil);
	
	if(status == 0) return YES;
	else return NO;
}











// Installer Options
- (BOOL) installBootloader
{
	
	NSString* bootPath;
	if([systemInfo installedBootloader] == [sender bootloaderType]) return YES;
	
	switch([sender bootloaderType])
	{
		case CHAMELEON_R431:
			bootPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/bootloader/chameleion2/"];
			break;
		case PCEFIV9:
			bootPath = [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/bootloader/pcefiv9/"];
			break;
		default:
			return YES;
	}
	
									
											
	OSStatus status;
	char* args[6];

	// fdisk -f boot0 -u -y /dev/rdisk0
	args[0] = "-f";	// Specify group wheel
	
	args[1] = (char*) [[bootPath stringByAppendingString: @"boot0"] cStringUsingEncoding:NSASCIIStringEncoding];
	args[2] = "-u";
	args[3] = "-y";
	args[4] = "/dev/rdisk0";		// TODO: calculate this (read form SystemInformaiton)
	args[5] = NULL;
	
	
	status = AuthorizationExecuteWithPrivileges(authRef, "/usr/sbin/fdisk", kAuthorizationFlagDefaults, args, nil);
	
	if(status != 0) return NO;

	
	
	// dd if=boot1h of=/dev/rdisk0s2
	args[1] = (char*) [[@"if=" stringByAppendingString:[bootPath stringByAppendingString: @"boot1h"]] cStringUsingEncoding:NSASCIIStringEncoding];
	args[2] = "of=/dev/rdisk0s2";
	args[3] = NULL;
	
	
	status = AuthorizationExecuteWithPrivileges(authRef, "/bin/dd", kAuthorizationFlagDefaults, args, nil);
	
	
	if(status != 0) return NO;
	
	[self copyFrom:[bootPath stringByAppendingString: @"boot"] toDir:@"/"];
	[self hidePath:@"/boot"];
	return YES;

}

- (BOOL) installExtensions
{
	BOOL status = YES;
	NSMutableArray* sourceExtensions = [[NSMutableArray alloc] initWithCapacity: 10];
	NSString* destinationExtensions =  [systemInfo extensionsFolder];

	// Install Extensions
	switch([systemInfo machineType]) {
		case MINI9:
			[sourceExtensions addObject: [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/Extnesions/"]];			
			[sourceExtensions addObject: [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/Mini 9 Extensions/"]];
			break;
			
		case VOSTRO_A90:
			[sourceExtensions addObject: [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/Extensions/"]];
			[sourceExtensions addObject: [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/Mini 9 Extensions/"]];
			break;
			
		case MINI10V:
			[sourceExtensions addObject: [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/Extensions/"]];
			[sourceExtensions addObject: [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/Mini 10v Extensions/"]];
			break;
			
		case LENOVO_S10:
			[sourceExtensions addObject: [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/Extensions/"]];
			[sourceExtensions addObject: [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/S10 Extensions/"]];
			break;
			
		default:
			[sourceExtensions addObject: [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/Extensions/"]];
			break;
	}
	
	[self makeDir: destinationExtensions];
	
	while([sourceExtensions count] > 0) {
		NSString* current = [sourceExtensions objectAtIndex: 0];
		[sourceExtensions removeObjectAtIndex: 0];
		if(![self copyFrom: current toDir: destinationExtensions]) status = NO;
	}
	
	
	return status;
}

- (BOOL) hideFiles
{
	[self hidePath:@"/boot"];
	[self hidePath:@"/Extra"];
	return YES;
}

- (BOOL) showFiles
{
	[self showPath:@"/boot"];
	[self showPath:@"/Extra"];
	return YES;
}

- (BOOL) installDSDT
{
	OSStatus status;

	// TODO: make the dsdt compile / decompiler into a framework / dylib

	[self makeDir:@"/tmp/dsdt/"];
	[self copyFrom:[[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/DSDTPatcher/"] toDir:@"/tmp/dsdt/"];
	
	char *args[] = { "PWD=/tmp/dsdt/", "/tmp/dsdt/DSDTPatcher", 0};
	
	// TODO: no need for a script here, just get it from the registery...
	status = AuthorizationExecuteWithPrivileges(authRef, "/usr/bin/env", kAuthorizationFlagDefaults, args, nil);
	
	if(status != 0) return NO;
	else return YES;

	//return [self copyFrom:@"/tmp/dsdt/dsdt.aml" toDir:@"/Extra"];
	// AuthorizationExecuteWithPrivileges does not block. As such, copying teh file wont work (since it doesnt exist yet).
	// This method will be rewritten to use teh DSDTPatcher class inctead of the file so that we can do stuff correctly
}

// TODO
- (BOOL) setRemoteCD: (BOOL) remoteCD
{
	NSMutableDictionary *dict;
	NSDictionary* save;		// TOOD: test as it may not be needed

	dict = [[NSMutableDictionary alloc] initWithDictionary:(NSDictionary*)CFPreferencesCopyMultiple(NULL,
													CFSTR("com.apple.NetworkBrowser"),
													kCFPreferencesCurrentUser,
													kCFPreferencesAnyHost)];
	
	
	if(([[dict objectForKey:@"EnableODiskBrowsing"] boolValue] && [[dict objectForKey:@"ODSSupported"] boolValue]) == remoteCD) return YES;
	
	[dict setObject:[[NSNumber alloc] initWithBool:remoteCD] forKey:@"EnableODiskBrowsing"];
	[dict setObject:[[NSNumber alloc] initWithBool:remoteCD] forKey:@"ODSSupported"];
	
	
	save = [[NSDictionary alloc] initWithDictionary: dict];

	// Save the preference file (for this uesr).
	// I could have used teh NSDicionary like all of the other preference files, but this is probably better
	CFPreferencesSetMultiple ((CFDictionaryRef) dict,
							  NULL,
							  CFSTR("com.apple.NetworkBrowser"),
							  kCFPreferencesCurrentUser,
							  kCFPreferencesAnyHost);
	return YES;
}

- (BOOL) dissableHibernation: (BOOL) dissable
{
	UInt8 state;
	
	if(([systemInfo hibernationDissabled]) ^ dissable)	// Setting have changed
	{
		NSMutableDictionary*	propertyList= [[NSMutableDictionary alloc] initWithContentsOfFile:@"/Library/Preferences/SystemConfiguration/com.apple.PowerManagement.plist"];
	
		NSMutableDictionary* powerStates = [[NSMutableDictionary alloc] initWithDictionary:[propertyList objectForKey:@"Custom Profile"]];
		NSMutableDictionary* acPowerState = [[NSMutableDictionary alloc] initWithDictionary:[powerStates objectForKey:@"AC Power"]];
		NSMutableDictionary* battPowerState = [[NSMutableDictionary alloc] initWithDictionary:[powerStates objectForKey:@"Battery Power"]];
	
		if(dissable) state = 0;
		else state = 3;
		
		[acPowerState   setObject: [NSNumber numberWithInt:state] forKey:@"Hibernate Mode"];
		[battPowerState setObject: [NSNumber numberWithInt:state] forKey:@"Hibernate Mode"];

		
		
		[powerStates setObject: acPowerState forKey:@"AC Power"];
		[powerStates setObject: battPowerState forKey:@"Battery Power"];
		[propertyList setObject: powerStates forKey:@"Custom Profile"];


		[propertyList writeToFile:@"/tmp/com.apple.PowerManagement.plist" atomically: NO]; 
		if(!dissable) [self deleteFile:@"/var/vm/sleepimage"];

		return [self copyFrom:@"/tmp/com.apple.PowerManagement.plist" toDir:@"/Library/Preferences/SystemConfiguration/"];
	}
	return YES;
}

- (BOOL) setQuietBoot: (BOOL) quietBoot
{
	NSMutableDictionary*	bootSettings =  [[NSMutableDictionary alloc] initWithContentsOfFile:[[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/ExtraFiles/com.apple.Boot.plist"]];
	NSString* setting = [bootSettings objectForKey:@"Quiet Boot"];
	
	if([setting isEqualToString:@"Yes"] && quietBoot == false)
	{
		// Dissable Quiet Boot
		[bootSettings removeObjectForKey:@"Quiet Boot"];
		[bootSettings setObject:[NSNumber numberWithInt:5] forKey:@"Timeout"];
	}
	else if(![setting isEqualToString:@"Yes"] && quietBoot == true)
	{
		[bootSettings removeObjectForKey:@"Timeout"];
		[bootSettings setObject:@"Yes" forKey:@"Quiet Boot"];

	}
	[bootSettings writeToFile:@"/tmp/com.apple.Boot.plist" atomically: NO];
	return [self copyFrom:@"/tmp/com.apple.Boot.plist" toDir:@"/Extra/"];
}

- (BOOL) fixBluetooth
{
	return [self deleteFile:@"/Library/Preferences/com.apple.Bluetooth.plist"];
}




// Support Files
- (BOOL) installExtraFiles
{
	NSString* source = [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/ExtraFiles/"];
	return [self copyFrom: source toDir: @"/Extra/"];
}

- (BOOL) installDisplayProfile
{
	NSString* source = [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/DisplayProfiles/"];
	return [self copyFrom: source toDir: @"/Library/ColorSync/Profiles/"];
}

- (BOOL) installSystemConfiguration
{
	NSString* source = [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/SystemConfiguration/"];
	return [self copyFrom: source toDir: @"/System/Library/SystemConfiguration/"];
}

- (BOOL) installPrefPanes
{
	NSString* source = [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/3rdPartyPrefPanes/"];
	return [self copyFrom: source toDir: @"/Library/PreferencePanes/"];
}

- (BOOL) installSystemPrefPanes
{
	NSString* source = [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/PrefPanes/"];
	return [self copyFrom: source toDir: @"/System/Library/PreferencePanes/"];
	
}

- (BOOL) installLaunchAgents
{
	NSString* source = [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/LaunchAgnets/"];
	return [self copyFrom: source toDir: @"/Library/LaunchAgents/"];
}


// DSD patch routines
- (BOOL) getDSDT
{
	return NO;
}

- (BOOL) patchDSDT
{
	return NO;
}

- (BOOL) patchDSDT: (BOOL) forcePatch
{
	return NO;
}


// Kext support (patching and copying)
- (BOOL) patchGMAkext
{
	[self copyFrom:@"/System/Library/Extensions/AppleIntelGMA950.kext" toDir:[systemInfo extensionsFolder]];	

	// Find: 8680A227
	// Replace: 8680AE27
	
	HexEditor* editor = [HexEditor alloc];
	
	UInt32 findBytes = 0x27A28086;
	UInt32 replaceBytes = 0x27AE8086;
	char findString[] = {'0', 'x', '2', '7', 'A', '2', '8', '0', '8', '6'};
	char replaceString[] = {'0', 'x', '2', '7', 'E', '2', '8', '0', '8', '6'};

	
	// Patch the binary file
	NSData* find = [[NSData alloc] initWithBytes:&findBytes length:4];
	NSData* replace = [[NSData alloc] initWithBytes:&replaceBytes length:4];
	[self copyFrom:@"/System/Library/Extensions/AppleIntelGMA950.kext" toDir:[systemInfo extensionsFolder]];
	editor = [editor initWithData:[[NSData alloc] initWithContentsOfFile:@"/System/Library/Extensions/AppleIntelGMA950.kext/Contents/MacOS/AppleIntelGMA950"]];
	[editor find: find andReplace: replace];
	[[editor data] writeToFile:@"/tmp/AppleIntelGMA950" atomically: NO];
	[self copyFrom:@"/tmp/AppleIntelGMA950" toDir:[[systemInfo extensionsFolder] stringByAppendingString:@"/AppleIntelGMA950.kext/Contents/MacOS/"]];	
	
	[find release];
	[replace release];
	

	// Patch the Info.plist, and NSDictionary would have worked as well.
	find = [[NSData alloc] initWithBytes:findString length:10];
	replace = [[NSData alloc] initWithBytes:replaceString length:10];

	editor = [editor initWithData:[[NSData alloc] initWithContentsOfFile:@"/System/Library/Extensions/AppleIntelGMA950.kext/Contents/Info.plist"]];
	[editor find: find andReplace: replace];
	[[editor data] writeToFile:@"/tmp/Info.plist" atomically: NO];
	[self copyFrom:@"/tmp/Info.plist" toDir:[[systemInfo extensionsFolder] stringByAppendingString:@"/AppleIntelGMA950.kext/Contents/"]];	
	
	[find release];
	[replace release];
	[editor release];
	return NO;
}

- (BOOL) patchFramebufferKext
{
	// Find: 8680A227
	// Replace: 8680AE27
	
	HexEditor* editor = [HexEditor alloc];
	
	UInt32 findBytes = 0x27A28086;
	UInt32 replaceBytes = 0x27AE8086;
	char findString[] = {'0', 'x', '2', '7', 'A', '2', '8', '0', '8', '6'};
	char replaceString[] = {'0', 'x', '2', '7', 'A', 'E', '8', '0', '8', '6'};
	
	
	// Patch the binary file
	NSData* find = [[NSData alloc] initWithBytes:&findBytes length:4];
	NSData* replace = [[NSData alloc] initWithBytes:&replaceBytes length:4];
	[self copyFrom:@"/System/Library/Extensions/AppleIntelIntegratedFramebuffer.kext" toDir:[systemInfo extensionsFolder]];
	editor = [editor initWithData:[[NSData alloc] initWithContentsOfFile:@"/System/Library/Extensions/AppleIntelIntegratedFramebuffer.kext/AppleIntelIntegratedFramebuffer"]];
	[editor find: find andReplace: replace];
	[[editor data] writeToFile:@"/tmp/AppleIntelIntegratedFramebuffer" atomically: NO];
	[self copyFrom:@"/tmp/AppleIntelIntegratedFramebuffer" toDir:[[systemInfo extensionsFolder] stringByAppendingString:@"/AppleIntelIntegratedFramebuffer.kext/"]];	
	
	[find release];
	[replace release];
	
	
	// Patch the Info.plist, and NSDictionary would have worked as well.
	find = [[NSData alloc] initWithBytes:findString length:10];
	replace = [[NSData alloc] initWithBytes:replaceString length:10];
	
	editor = [editor initWithData:[[NSData alloc] initWithContentsOfFile:@"/System/Library/Extensions/AppleIntelIntegratedFramebuffer.kext/Info.plist"]];
	[editor find: find andReplace: replace];
	[[editor data] writeToFile:@"/tmp/Info.plist" atomically: NO];
	[self copyFrom:@"/tmp/Info.plist" toDir:[[systemInfo extensionsFolder] stringByAppendingString:@"/AppleIntelIntegratedFramebuffer.kext/"]];	
	
	[find release];
	[replace release];
	[editor release];
	return NO;
}

- (BOOL) patchIO80211kext
{
	NSMutableDictionary* plist = [[NSMutableDictionary alloc] initWithContentsOfFile:@"/System/Library/Extensions/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist"];
	NSMutableDictionary *personalities, *bcmpci;
	NSMutableArray* ids;
	[self copyFrom:@"/System/Library/Extensions/IO80211Family.kext" toDir:[systemInfo extensionsFolder]];	
	
	personalities = [[NSMutableDictionary alloc] initWithDictionary:[plist objectForKey:@"IOKitPersonalities"]];
	
	bcmpci = [[NSMutableDictionary alloc] initWithDictionary:[personalities objectForKey:@"Broadcom 802.11 PCI"]];
	ids = [[NSMutableArray alloc] initWithArray:[bcmpci objectForKey:@"IONameMatch"]];

	[ids addObject:@"pci14e4,4306"];
	[ids addObject:@"pci14e4,4309"];
	[ids addObject:@"pci14e4,4315"];
	[ids addObject:@"pci14e4,4320"];
	[ids addObject:@"pci14e4,4324"];
	[ids addObject:@"pci14e4,4329"];
	[ids addObject:@"pci14e4,432a"];

	
	[bcmpci setObject:ids forKey:@"IONameMatch"];
	[personalities setObject:bcmpci forKey:@"Broadcom 802.11 PCI"];
	[plist setObject:personalities forKey:@"IOKitPersonalities"];

	// Save the file and write it to the new one
	[plist writeToFile:@"/tmp/Info.plist" atomically: NO];

	[self copyFrom:@"/tmp/Info.plist" toDir:[[systemInfo extensionsFolder] stringByAppendingString:@"/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/"]];	

	[ids release];
	[bcmpci release];
	[personalities release];
	[plist release];

	return YES;
}


//----------			patchBluetooth			----------//
- (BOOL) patchBluetooth
{
	if(![systemInfo bluetoothDeviceId] || ![systemInfo bluetoothVendorId]) return NO;	// Unknown device / vendor
	// TODO: if unknown, search through IORegistery
	
	[self copyFrom:@"/System/Library/Extensions/IOBluetoothFamily.kext" toDir:[systemInfo extensionsFolder]];
	
	NSMutableDictionary* plist = [[NSMutableDictionary alloc] initWithContentsOfFile:@"/System/Library/Extensions/IOBluetoothFamily.kext/Contents/PlugIns/BroadcomUSBBluetoothHCIController.kext/Contents/Info.plist"];
	NSMutableDictionary* properties = [[NSMutableDictionary alloc] initWithDictionary:[plist objectForKey:@"IOKitPersonalities"]];
	NSMutableDictionary* bcmusb = [[NSMutableDictionary alloc] initWithDictionary:[properties objectForKey:@"Broadcom2046FamilyUSBBluetoothHCIController_37A"]];
	
	[bcmusb setObject:[[NSNumber alloc] initWithInt:[systemInfo bluetoothDeviceId]] forKey:@"idProduct"];
	[bcmusb setObject:[[NSNumber alloc] initWithInt:[systemInfo bluetoothVendorId]] forKey:@"idVendor"];
	
	[properties setObject: bcmusb forKey:@"Broadcom2046FamilyUSBBluetoothHCIController_37A"];
	[plist setObject: properties forKey:@"IOKitPersonalities"];
	
	[plist writeToFile:@"/tmp/Info.plist" atomically:NO];
	return [self copyFrom: @"/tmp/Info.plist" toDir: @"/Extra/IOBluetoothFamily.kext/Contents/PlugIns/BroadcomUSBBluetoothHCIController.kext/Contents/Info.plist"];
}

//----------			installLocalExtensions			----------//
- (BOOL) installLocalExtensions
{
	NSString* source;
	source = [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/LocalExtensions/"];
	return [self copyFrom: source toDir: @"/System/Library/Extensions/"];
}

//----------			copyDependencies			----------//
- (BOOL) copyDependencies
{
	[self copyFrom:@"/System/Library/Extensions/IONetworkingFamily.kext" toDir:[systemInfo extensionsFolder]];
	[self copyFrom:@"/System/Library/Extensions/IONDRVSupport.kext" toDir:[systemInfo extensionsFolder]];
	[self copyFrom:@"/System/Library/Extensions/IOGraphicsFamily.kext" toDir:[systemInfo extensionsFolder]];
	
	return YES;
}


- (BOOL) installMirrorFriendlyGraphics
{
	NSString* source = [[[NSBundle mainBundle] resourcePath] stringByAppendingString: @"/SupportFiles/OldGMA/"];	 
	return [self copyFrom: source toDir: [systemInfo extensionsFolder]];
}



@end
