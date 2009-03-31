//
//  PS2PrefrencePanePref.h
//  PS2PrefrencePane
//
//  Created by Evan Lojewski on 3/26/09.
//  Copyright (c) 2009 UCCS. All rights reserved.
//

#import <PreferencePanes/PreferencePanes.h>


@interface PS2PrefrencePanePref : NSPreferencePane 
{
	IBOutlet NSSlider* _trackpadSlider;
	IBOutlet NSSlider* _scrollSpeedSlider;
	IBOutlet NSSlider* _scrollAreaSlider;

	IBOutlet NSSlider* _trackpadSensitivitySlider;

	
	IBOutlet NSButton* _tapToClickCheckbox;
	IBOutlet NSButton* _draggingCheckbox;
	IBOutlet NSButton* _dragLockCheckbox;
	
	IBOutlet NSButton* _edgescrollChecbox;
	IBOutlet NSButton* _horizontalScrolling;
	
	IBOutlet NSTextField* _scrollSpeedText;
	IBOutlet NSTextField* _scrollAreaText;
	
	IBOutlet bool _scrollEnabled;
	
	// Touchpad prefrences
	bool _tapToClick;
	bool _dragable;
	bool _dragLock;
	bool _scrolling;
	int _trackpadSpeed;
	int _scrollSpeed;
	int _accelerationRate;
	
	// Keyboard prefrences
	bool _swapWindows;
	bool _enableScrollKey;
	
	//int buttonEnabled;
	
	
}

- (void) mainViewDidLoad;

- (bool) setPrefrences;

- (IBAction) setTapToClick: (id) sender;
- (IBAction) setDragable: (id) sender;
- (IBAction) setDragLock: (id) sender;
- (IBAction) setSrolling: (id) sender;
- (IBAction) setHorizScrolling: (id) sender;
- (IBAction) setTrackpadSpeed: (id) sender;
- (IBAction) setScrollSpeed: (id) sender;
- (IBAction) setScrollArea: (id) sender;
- (IBAction) setAcceleration: (id) sender;


@end
