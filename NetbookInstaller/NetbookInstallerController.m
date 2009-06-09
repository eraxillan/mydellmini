//
//  NetbookInstallerController.m
//  NetbookInstaller
//
//  Created by Evan Lojewski on 5/15/09.
//  Copyright 2009. All rights reserved.
//

#import "NetbookInstallerController.h"


@implementation NetbookInstallerController

/***
 ** awakeFromNib
 **		This function handles intializing the window before it is displayed. 
 **		It obtains the system info and sets the checkboxes and labels
 **
 ***/
- (void) awakeFromNib {	
	// This is run whenever ANY nib file is loaded
	if(!initialized) [self initializeApplication];
}

- (void) initializeApplication
{
	NSDictionary* infoDict;
	systemInfo = [[SystemInformation alloc] init];
	appBundle = [NSBundle mainBundle];
	infoDict = [appBundle infoDictionary];
	
	[systemInfo determineInstallState];
	
	
	// Set the version lable and window title (does not fully rebrand, but it's good enough)
	[mainWindow	  setTitle:		  [infoDict objectForKey:@"CFBundleExecutable"]];	// Bundle Name would work as well
	[versionLabel setStringValue: [infoDict objectForKey:@"CFBundleVersion"]];
	
	// Initialize botloader dropdown
	NSMutableArray* bootOptions = [[NSMutableArray alloc] init];
	int i = 0;
	while(i < NUM_SUPPORTED_BOOTLOADERS) {
		// Or I could use for loops... 
		[bootOptions addObject:[[NSString alloc] initWithCString:bootLoaderName[i]]];
		i++;
	}
	
	[bootloaderComboBox addItemsWithObjectValues:bootOptions];
	if([systemInfo installedBootloader] != NONE)
		// Bootloader is installed, we do not force an upgrade
		[bootloaderComboBox selectItemAtIndex:[systemInfo installedBootloader]];
	else
	{
		// No bootloader is installed, default to the latest version available
		[bootloaderComboBox selectItemAtIndex:CHAMELEON_R431];
		[bootloaderCheckbox setState:true];
	}
	
	
	
	
	// Initialize warning label
	// TODO: place all of the strings in a file (info.plist.string)
	switch([systemInfo machineType]) {
		case MINI9: [warningLabel setStringValue:NSLocalizedString(@"Mini 9 Warning", nil)];
			break;
		case MINI10V: [warningLabel setStringValue:NSLocalizedString(@"Mini 10v Warning", nil)];
			break;
		case VOSTRO_A90: [warningLabel setStringValue:NSLocalizedString(@"Vostro A90 Warning", nil)];
			break;
		case LENOVO_S10: [warningLabel setStringValue:NSLocalizedString(@"S10 Warning", nil)];
			break;
		case UNKNOWN:
		default: [warningLabel setStringValue:NSLocalizedString(@"Unknown Warning", nil)];
			break;
	}
	
	if([systemInfo installedBootloader] == CHAMELEON_R431) {
		[bootloaderCheckbox setState: false];
		[bootloaderComboBox setEnabled:false];
	}
	
	// Initialize checkboxes (TODO: commented checkboxes need to be initialized
	//extensionsCheckbox;
	//oldGMACheckbox;
	
	if([systemInfo efiHidden])
	{
		[showhideFilesCheckbox setState:false];
		[showhideFilesCheckbox setTitle:[[@"Show " stringByAppendingString:[infoDict objectForKey:@"CFBundleExecutable"]] stringByAppendingString:@" Files"]];
	} 
	else
	{
		[showhideFilesCheckbox setState:true];
		[showhideFilesCheckbox setTitle:[[@"Hide " stringByAppendingString:[infoDict objectForKey:@"CFBundleExecutable"]] stringByAppendingString:@" Files"]];
		
	}
	[dsdtCheckbox setState:![systemInfo dsdtInstalled]];
	[keyboardPrefPaneCheckbox setState: ![systemInfo keyboardPrefPaneInstalled]];
	if([systemInfo remoteCDEnabled])
	{
		[remoteCDCheckbox setState:false];
		[remoteCDCheckbox setTitle:NSLocalizedString(@"Disable Remote CD", nil)];
	} 
	else
	{
		[remoteCDCheckbox setState:true];
		[remoteCDCheckbox setTitle:NSLocalizedString(@"Enable Remote CD", nil)];
		
	}
	
	if([systemInfo hibernationDissabled])
	{
		[hibernateChecbox setState:false];
		[hibernateChecbox setTitle:NSLocalizedString(@"Enable hibernation", nil)];
	} 
	else
	{
		[hibernateChecbox setState:true];
		[hibernateChecbox setTitle:NSLocalizedString(@"Disable hibernation", nil)];
		
	}
	
	if([systemInfo quietBoot])
	{
		[quietBootCheckbox setState:false];
		[quietBootCheckbox setTitle:NSLocalizedString(@"Disable Quiet Boot", nil)];
	} 
	else
	{
		[quietBootCheckbox setState:false];
		[quietBootCheckbox setTitle:NSLocalizedString(@"Enable Quiet Boot", nil)];
		
	}
	[bluetoothCheckbox setState:![systemInfo bluetoothPatched]];
	[targetVolume setStringValue:[systemInfo bootPartition]];
}


/***
 ** isMachineSupported
 **		This function checks the system info class's machineType varaible 
 **		and insures that not only is it supported, but that the nessicary
 **		extensions exist.
 **
 ***/
- (BOOL) isMachineSupported {
	BOOL			supported, isDir;
	NSFileManager* fileManager;
	NSString		*path, *fullPath;
	
	fileManager = [NSFileManager defaultManager];
	path = [appBundle resourcePath];
	
	switch([systemInfo machineType]) {
		case MINI9:
		case VOSTRO_A90:
			fullPath = [NSString stringWithFormat:@"%@/Mini 9 Extensions/",path];
			supported = [fileManager fileExistsAtPath: fullPath isDirectory: &isDir];
			if(!isDir) supported = NO;
			break;
		
		case MINI10V:
			fullPath = [NSString stringWithFormat:@"%@/Mini 10v Extensions/",path];
			supported = [fileManager fileExistsAtPath: fullPath isDirectory: &isDir];
			if(!isDir) supported = NO;
			break;
			
		case LENOVO_S10:
			fullPath = [NSString stringWithFormat:@"%@/S10 Extensions/",path];
			supported = [fileManager fileExistsAtPath: fullPath isDirectory: &isDir];
			if(!isDir) supported = NO;			break;
		case UNKNOWN:
		default:
			supported = NO;
			break;
	}
	
	if(!supported) [systemInfo machineType: UNKNOWN];
	return supported;
}

/***
 ** applicationDidFinishLoading
 **		This function creates an alert if we are on an unsupported machine
 **		This is NOT in awake from nib because the alert cannot attach to the window
 **		when it isn't done being created / visible
 **
 ***/
- (void) applicationDidFinishLaunching:(id)application
{
	initialized = YES;
	if(![self isMachineSupported]) {
		// Look into NSRunAlertPanel
		NSAlert *alert = [[[NSAlert alloc] init] autorelease];
		[alert addButtonWithTitle:NSLocalizedString(@"Continue", nil)];
		[alert addButtonWithTitle:NSLocalizedString(@"Cancel", nil)];
		[alert setMessageText:NSLocalizedString(@"Unsupported Device", nil)];
		[alert setInformativeText:NSLocalizedString(@"You are running this applicaiton on an unsupported device. Are you sure you want to continue?", nil)];
		[alert setAlertStyle:NSWarningAlertStyle];
		[alert beginSheetModalForWindow:mainWindow modalDelegate:self didEndSelector:@selector(unknownMachineAlert:returnCode:contextInfo:) contextInfo:nil];
	}
}

/***
 ** applicationShouldTerminateAfterLastWindowClosed
 **		This funciton tells Mac OS X to terminate the program when the windows close
 **
 ***/
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
	return YES;
}


- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	if(installing) return NSTerminateCancel;
	else return NSTerminateNow;
}

/***
 ** unknownMachineAlert
 **		This function handles the alert and exit's if the user selects cancle
 **
 ***/
- (void) unknownMachineAlert:(NSAlert *)alert returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
	if(returnCode == NSAlertSecondButtonReturn)
	{
		exit(-1);
	}
}

/***
 ** performInstall
 **		This function is called when the install button is pressed.
 **		It creates an installer and passes it needed information.
 **
 ***/
- (IBAction) performInstall:  (id) sender {
	Installer* installer;
	
	installing = YES;	// Dissable applicaiton closing while it's doing stuff
	
	installer = [[Installer alloc] initWithSender: self];
	
	[installButton setEnabled:false];
	[progressBar setHidden:false];
	[progressBar startAnimation: sender];
	
	
	// TODO: pass the self object so that the window can be updated
	// TODO: When pressed, dissable window closes
	
	BOOL status = [installer performInstall: systemInfo];
	if(!status)
	{
		// do something, the install failed
		// TODO: Alert the user that the install failed
		NSAlert *alert = [[[NSAlert alloc] init] autorelease];
		[alert addButtonWithTitle:NSLocalizedString(@"Continue", nil)];
		[alert setMessageText:NSLocalizedString(@"Installation Failed", nil)];
		[alert setInformativeText:NSLocalizedString(@"The installation failed. Please look at consol.app for more information about the failure.", nil)];
		[alert setAlertStyle:NSWarningAlertStyle];
		[alert beginSheetModalForWindow:mainWindow modalDelegate:nil didEndSelector:nil contextInfo:nil];
		
	}
	else
	{
		// Reboot if needed
	}
	
	
	[installer release];
	
	// TODO:  Force a restart if needed
	
	//[progressBar setHidden:true];
	//[progressBar stopAnimation: sender];
	
	installing = NO;
}

/***
 ** openAboutWindows
 **		This opens the about window by loading the about nib.
 **		The check is very simple and could (should) be much better / fixed
 **
 ***/
- (IBAction) openAboutWindow: (id) sender
{
	NSArray* windows;
	// TODO: Fix the check to ensure it really is not open
	
	// Check to make sure only one (the main window) exists.
	// Since there are only two possible windows (main + about), this is acceptable
	//if([[NSApp windows] count] < 2) [NSBundle loadNibNamed:@"about" owner:self];
	
	
	// Use
	
	int i = 0;
	windows = [NSApp windows];
	while(i < [windows count])
	{
		if([windows objectAtIndex:i]  && [[[windows objectAtIndex:i] title] isEqualToString:@"About"]) return;
		i++;
	}
	[NSBundle loadNibNamed:@"about" owner:self];
}

/***
 ** installationMethodModified
 **		This is called when the installation method checkbox is chaned, enables
 **		or disabled all of the checkboxes
 **		
 ***/
- (IBAction) installationMethodModified: (id) sender
{

	bool state = [[sender cells] indexOfObject: [sender selectedCell]];
	//bool state = true;
	// Dissable / enable all checkboxes
	[bootloaderCheckbox			setEnabled: state];
	//if(state) 	[oldGMACheckbox setEnabled: [oldGMACheckbox state]];
	//else		[oldGMACheckbox setEnabled: false];
	
	[extensionsCheckbox			setEnabled: state];
	[oldGMACheckbox				setEnabled: state && [extensionsCheckbox state]];

	[showhideFilesCheckbox		setEnabled: state];
	[dsdtCheckbox				setEnabled: state];
	[keyboardPrefPaneCheckbox	setEnabled: state];
	[remoteCDCheckbox			setEnabled: state];
	[hibernateChecbox			setEnabled: state];
	[quietBootCheckbox			setEnabled: state];
	[bluetoothCheckbox			setEnabled: state];
	[bootloaderComboBox			setEnabled: state && [bootloaderCheckbox state]];

}


- (IBAction) bootloaderModified: (id) sender
{
	// Dissable / enable target option
	[bootloaderComboBox setEnabled: [sender state]];
//	[targetDiskBlah
}


- (IBAction) extensionsModified: (id) sender
{
	[oldGMACheckbox setEnabled: [sender state]];
	// Dissable / enable GMA checbox
}

- (void)applicationWillTerminate:(NSNotification *)aNotification
{
	[systemInfo release];
}


- (void) setProgress: (UInt8) progress
{
	[progressBar setValue:[NSNumber numberWithInt: progress]];
}

////////////////

- (BOOL) enableQuietBoot
{
	if(![quietBootCheckbox state]) return [systemInfo quietBoot];
	else return (![systemInfo quietBoot]);
}

- (BOOL) dissableHibernation
{
	if(![hibernateChecbox state]) return [systemInfo hibernationDissabled];
	else return (![systemInfo hibernationDissabled]);
}

- (BOOL) enableRemoteCD
{
	if(![remoteCDCheckbox state]) return [systemInfo remoteCDEnabled];
	else return (![systemInfo remoteCDEnabled]);
}


- (BOOL) installExtensions
{
	return [extensionsCheckbox state];
}

- (enum bootloader) bootloaderType
{
	if([bootloaderCheckbox state]) return CHAMELEON_R431;
	else return NONE;
}

- (BOOL) fixBluetooth
{
	return [bluetoothCheckbox state];
}

- (BOOL) mirrorFriendlyGMA
{
	return [oldGMACheckbox state];
}

- (BOOL) regenerateDSDT
{
	return [dsdtCheckbox state];
}
- (BOOL) toggleVisibility
{
	return [showhideFilesCheckbox state];
}

- (BOOL) install1055PrefPane
{
	return [keyboardPrefPaneCheckbox state];
}

- (BOOL) updatePorgressBar: (NSUInteger) percent
{
	//[progressBar setValue: [[NSNumber alloc] initWithInt: percent]];
	[progressBar incrementBy: percent];
	return YES;
}
- (BOOL) updateStatus: (NSString*) status
{
	[statusLabel setStringValue:status];
	return YES;
}	

- (BOOL) hideFiles
{
	return [showhideFilesCheckbox state];
}

@end
