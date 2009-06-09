//
//  Installer.h
//  NetbookInstaller
//
//  Created by Evan Lojewski on 5/16/09.
//  Copyright 2009. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <SecurityFoundation/SFAuthorization.h>
#import "SystemInformation.h"
#import "NetbookInstallerController.h"
#import "HexEditor.h"


@interface Installer : NSObject {
	SystemInformation*	systemInfo;
	NSString*			extensionsDirectory;
	AuthorizationRef	authRef;
	id					sender;

	
}

- (BOOL) performInstall: (SystemInformation*) systemInfo;

// Get root authorization;
- (BOOL) getAuthRef;

- (BOOL) copyFrom: (NSString*) source toDir: (NSString*) destination;
- (BOOL) makeDir: (NSString*) dir;
- (BOOL) deleteFile: (NSString*) file;
- (BOOL) hidePath: (NSString*) path;
- (BOOL) showPath: (NSString*) path;


// TODO: make a BOM or similar to do this automaticaly... there really is no need for specific function
- (BOOL) installDisplayProfile;
- (BOOL) installPrefPanes;
- (BOOL) installSystemPrefPanes;
- (BOOL) installSystemConfiguration;
- (BOOL) installExtraFiles;



					
- (BOOL) setPermissions: (UInt8) perms onPath: (NSString*) path recursivly: (BOOL) recursiv;
- (BOOL) setOwner: (NSString*) owner andGroup: (NSString*) group OnPath: (NSString*) path recursivly: (BOOL) recursiv;

// Installer Options
- (BOOL) installBootloader;
- (BOOL) installExtensions;
- (BOOL) hideFiles;
- (BOOL) showFiles;
- (BOOL) installDSDT;
- (BOOL) setRemoteCD: (BOOL) remoteCD;
- (BOOL) dissableHibernation: (BOOL) hibernation;
- (BOOL) setQuietBoot: (BOOL) quietBoot;
- (BOOL) fixBluetooth;

- (BOOL) installMirrorFriendlyGraphics;


// DSD patch routines
- (BOOL) getDSDT;
- (BOOL) patchDSDT;
- (BOOL) patchDSDT: (BOOL) forcePatch;

// Kext support (patching and copying)
- (BOOL) patchGMAkext;
- (BOOL) patchFramebufferKext;
- (BOOL) patchIO80211kext;
- (BOOL) patchBluetooth;
- (BOOL) installLocalExtensions;
- (BOOL) copyDependencies;


- (id) initWithSender: (id) sender;


@end
