//
//  PS2PrefrencePanePref.m
//  PS2PrefrencePane
//
//  Created by Evan Lojewski on 3/26/09.
//  Copyright (c) 2009 UCCS. All rights reserved.
//

#import "PS2PrefrencePanePref.h"
#include <IOKit/IOKitLib.h>


@implementation PS2PrefrencePanePref

- (void) mainViewDidLoad
{
}

- (bool) setPrefrences
{
	CFMutableDictionaryRef	dictRef;
	io_iterator_t			iter;
    io_service_t			service;
    kern_return_t			kr;
    CFNumberRef				numberRef;
    SInt32					constantOne = 1;
	
	
	
	NSLog([[NSString alloc] initWithCString:"Setting prefrences..." encoding:NSASCIIStringEncoding]);

    
    // The bulk of this code locates all instances of our driver running on the system.
	
	// First find all children of our driver. As opposed to nubs, drivers are often not registered
	// via the registerServices call because nothing is expected to match to them. Unregistered
	// objects in the I/O Registry are not returned by IOServiceGetMatchingServices.
	
	// IOBlockStorageServices is our child in the I/O Registry
	dictRef = IOServiceMatching("IOHIDPointer"); 
    if (!dictRef) {
		NSLog([[NSString alloc] initWithCString:"IOServiceMatching returned NULL.\n" encoding:NSASCIIStringEncoding]);
        return false;
    }
    
    // Create an iterator over all matching IOService nubs.
    // This consumes a reference on dictRef.
    kr = IOServiceGetMatchingServices(kIOMasterPortDefault, dictRef, &iter);
    if (KERN_SUCCESS != kr) {
		NSLog([[NSString alloc] initWithCString:"IOServiceGetMatchingServices returned 0x%08x.\n" encoding:NSASCIIStringEncoding]);

		//fprintf(stderr, "IOServiceGetMatchingServices returned 0x%08x.\n", kr);
        return false;
    }
	
    // Create a dictionary to pass to our driver. This dictionary has the key "MyProperty"
	// and the value an integer 1. 
    dictRef = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, 
                                        &kCFTypeDictionaryKeyCallBacks,
                                        &kCFTypeDictionaryValueCallBacks);
    
    numberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &constantOne);    
    CFDictionarySetValue(dictRef, CFSTR("MyProperty"), numberRef);    
    CFRelease(numberRef);
	
    // Iterate across all instances of IOBlockStorageServices.
	while ((service = IOIteratorNext(iter))) {
        io_registry_entry_t parent;
        
        // Now that our child has been found we can traverse the I/O Registry to find our driver.
		kr = IORegistryEntryGetParentEntry(service, kIOServicePlane, &parent);
        if (KERN_SUCCESS != kr) {
			NSLog([[NSString alloc] initWithCString:"IORegistryEntryGetParentEntry returned 0x%08x.\n" encoding:NSASCIIStringEncoding]);
			//			fprintf(stderr, "IORegistryEntryGetParentEntry returned 0x%08x.\n", kr);
        }
        else {
            // We're only interested in the parent object if it's our driver class.
			if (IOObjectConformsTo(parent, "ApplePS2SynapticsTouchPad")) {
                // This is the function that results in ::setProperties() being called in our
                // kernel driver. The dictionary we created is passed to the driver here.
                kr = IORegistryEntrySetCFProperties(parent, dictRef);
				NSLog([[NSString alloc] initWithCString:"Sent message to kext" encoding:NSASCIIStringEncoding]);
                if (KERN_SUCCESS != kr) {
					NSLog([[NSString alloc] initWithCString:"IORegistryEntrySetCFProperties returned 0x%08x.\n" encoding:NSASCIIStringEncoding]);
					//					fprintf(stderr, "IORegistryEntrySetCFProperties returned 0x%08x.\n", kr);
                }
            } else {
				NSLog([[NSString alloc] initWithCString:"Unable to locate Touchpad kext.\n" encoding:NSASCIIStringEncoding]);
//				IOObjectRelease(parent);
//				IOObjectRelease(service);

//				return false
			}
            
            // Done with the parent object.
			IOObjectRelease(parent);
        }
        
        // Done with the object returned by the iterator.
		IOObjectRelease(service);
    }
	
    if (iter != IO_OBJECT_NULL) {
        IOObjectRelease(iter);
        iter = IO_OBJECT_NULL;
    }
	
    if (dictRef) {
        CFRelease(dictRef);
        dictRef = NULL;
    } 
	NSLog([[NSString alloc] initWithCString:"All done :)\n" encoding:NSASCIIStringEncoding]);

	return true;
}

- (IBAction) setTapToClick: (id) sender
{
	//_tapToClick = tapToClick;
	if([sender state] == NSOnState) {
		[_draggingCheckbox setEnabled:true];
		[_dragLockCheckbox setEnabled:true];

	} else {
		[_draggingCheckbox setEnabled:false];
		[_dragLockCheckbox setEnabled:false];
	}
	[self setPrefrences];
} //(bool) tapToClick;

- (IBAction) setDragable: (id) sender
{
	if([sender state] == NSOnState) {
		[_dragLockCheckbox setEnabled:true];
	} else {
		[_dragLockCheckbox setEnabled:false];
	}
	[self setPrefrences];

}

- (IBAction) setDragLock: (id) sender
{
	[self setPrefrences];

}

- (IBAction) setSrolling: (id) sender
{
	if([sender state] == NSOnState) {
		[_horizontalScrolling setEnabled:true];
		[_scrollSpeedSlider setEnabled:true];
		[_scrollAreaSlider setEnabled:true];
	} else {
		[_horizontalScrolling setEnabled:false];
		[_scrollSpeedSlider setEnabled:false];
		[_scrollAreaSlider setEnabled:false];
	}
	[self setPrefrences];

}

- (IBAction) setHorizScrolling: (id) sender
{
	[self setPrefrences];
}

- (IBAction) setTrackpadSpeed: (id) sender
{
	[self setPrefrences];

}

- (IBAction) setScrollSpeed: (id) sender
{
	[self setPrefrences];

}

- (IBAction) setScrollArea: (id) sender
{
	[self setPrefrences];
	
}

- (IBAction) setAcceleration: (id) sender
{
	[self setPrefrences];

}


@end
