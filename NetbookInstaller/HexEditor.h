//
//  HexEditor.h
//  NetbookInstaller
//
//  Created by meklort on 6/1/09.
//  Copyright 2009 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface HexEditor : NSObject {
	NSMutableData*	data;
}
- (NSMutableData*) data;
- (id) initWithData: (NSData*) initialData;
- (NSUInteger) find: (NSData*) needle andReplace: (NSData*) replace;
@end
