//
//  NetbookInstallerController.h
//  NetbookInstaller
//
//  Created by Evan Lojewski on 5/15/09.
//  Copyright 2009 UCCS. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "SystemInformation.h"
#import "Installer.h"

@interface NetbookInstallerController : NSObject {
	SystemInformation*	systemInfo;
	NSBundle*			appBundle;
	IBOutlet NSWindow*			mainWindow;
	
	IBOutlet NSMatrix*		installationMethod;
	
	IBOutlet NSButton*		installButton;
	
	IBOutlet NSButton*		bootloaderCheckbox;
	IBOutlet NSButton*		extensionsCheckbox;
	IBOutlet NSButton*		oldGMACheckbox;
	IBOutlet NSButton*		showhideFilesCheckbox;
	IBOutlet NSButton*		dsdtCheckbox;
	IBOutlet NSButton*		keyboardPrefPaneCheckbox;
	IBOutlet NSButton*		remoteCDCheckbox;
	IBOutlet NSButton*		hibernateChecbox;
	IBOutlet NSButton*		quietBootCheckbox;
	IBOutlet NSButton*		bluetoothCheckbox;
	
	IBOutlet NSComboBox*	bootloaderComboBox;
	
	IBOutlet NSProgressIndicator*	progressBar;
	IBOutlet NSTextField*	warningLabel;
	IBOutlet NSTextField*	versionLabel;
	IBOutlet NSTextField*	statusLabel;
	IBOutlet NSTextField*	targetVolume;
	
	BOOL					installing;
	BOOL					initialized;
	
	
	
}

- (void) awakeFromNib;

- (IBAction) openAboutWindow: (id) sender;

- (void) applicationDidFinishLaunching:(id)application;
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication;
- (void)applicationWillTerminate:(NSNotification *)aNotification;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender;

- (void) initializeApplication;

- (void) unknownMachineAlert:(NSAlert *)alert returnCode:(int)returnCode contextInfo:(void *)contextInfo;

- (IBAction) installationMethodModified: (id) sender;
- (IBAction) performInstall: (id) sender;
- (IBAction) bootloaderModified: (id) sender;
- (IBAction) extensionsModified: (id) sender;

- (BOOL) isMachineSupported;
/*
 - (IBAction) oldGMAModified: (id) sender;
 - (IBAction) showHideFilesModified: (id) sender;
 - (IBAction) dsdtModified: (id) sender;
- (IBAction) keyboardPrefPaneModified: (id) sender;
 - (IBAction) remoteCDModified: (id) sender;
 - (IBAction) hibernateModified: (id) sender;
 - (IBAction) quietBootModified: (id) sender;
 - (IBAction) bluetoothModified: (id) sender;
 */

- (void) setProgress: (UInt8) progress;

// Requested system State
- (BOOL) enableQuietBoot;
- (BOOL) dissableHibernation;
- (BOOL) enableRemoteCD;
- (BOOL) installExtensions;
- (enum bootloader) bootloaderType;

- (BOOL) fixBluetooth;
- (BOOL) mirrorFriendlyGMA;
- (BOOL) regenerateDSDT;
- (BOOL) fixBluetooth;
- (BOOL) toggleVisibility;
- (BOOL) hideFiles;

- (BOOL) install1055PrefPane;

- (BOOL) updatePorgressBar: (NSUInteger) percent;
- (BOOL) updateStatus: (NSString*) status;


@end
