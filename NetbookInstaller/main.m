//
//  main.m
//  NetbookInstaller
//
//  Created by Evan Lojewski on 5/15/09.
//  Copyright UCCS 2009. All rights reserved.
//

#import <Cocoa/Cocoa.h>

int main(int argc, char *argv[])
{
	// TODO: make sure everything is realeased properly... (It's not)
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	return NSApplicationMain(argc,  (const char **) argv);
	[pool release];
}
