/*
 * Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.2 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 * 
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef _APPLEPS2SYNAPTICSTOUCHPAD_H
#define _APPLEPS2SYNAPTICSTOUCHPAD_H

#include "ApplePS2MouseDevice.h"
#include <IOKit/hidsystem/IOHIPointing.h>

//#define BUTTONS_SWAPED


#define RELATIVE_PACKET_SIZE	3
#define ABSOLUTE_PACKET_SIZE	6

#define ABSOLUTE_X_MIN			1472
#define ABSOLUTE_X_MAX			5312
#define ABSOLUTE_Y_MIN			1568
#define ABSOLUTE_Y_MAX			4288

// _touchPadModeByte bitmap
#define ABSOLUTE_MODE_BIT			0x80
#define RATE_MODE_BIT				0x40
// bit 5 undefined
// bit 4 undefined
#define SLEEP_MODE_BIT				0x08
#define GESTURES_MODE_BIT			0x04
//	#define PACKET_SIZE				0x02		// Only used for serial touchpads, not ps2
#define W_MODE_BIT					0x01

// The following is for reading _touchPadModeByte bitmap
#define ABSOLUTE_MODE			((_touchPadModeByte & ABSOLUTE_MODE_BIT) >> 7)
#define RELATIVE_MODE		    !(ABSOLUTE_MODE)
#define RATE_80_PPS				((_touchPadModeByte & RATE_MODE_BIT) >> 6)
#define SLEEPING				((_touchPadModeByte & SLEEP_MODE_BIT) >> 3)
#define GESTURES				((_touchPadModeByte & GESTURES_MODE_BIT) >> 2)
#define W_MODE					(_touchPadModeByte & W_MODE_BIT)

// Read the _capabilties (16 bit number) (These are ONLY true if W_MODE is also true)
#define CAP_PALM_DETECT				(_capabilties & 0x0001)
#define CAP_MULTIFINGER				(_capabilties & 0x0002)
// #define CAP_RESERVER_CAP1		(_capabilties & 0x0004)
#define CAP_FOUR_BUTTONS			(_capabilties & 0x0008)
// #define CAP_RESERVER_CAP2		(_capabilties & 0x0010)
// #define CAP_RESERVER_CAP3		(_capabilties & 0x0020)
// #define CAP_RESERVER_CAP4		(_capabilties & 0x0040)
// #define CAP_RESERVER_CAP5		(_capabilties & 0x0080)
// #define CAP_RESERVER_CAP6		(_capabilties & 0x0100)
// #define CAP_RESERVER_CAP7		(_capabilties & 0x0200)
// #define CAP_RESERVER_CAP8		(_capabilties & 0x0400)
// #define CAP_RESERVER_CAP9		(_capabilties & 0x0800)
// #define CAP_RESERVER_CAP10		(_capabilties & 0x1000)
// #define CAP_RESERVER_CAP11		(_capabilties & 0x2000)
// #define CAP_RESERVER_CAP12		(_capabilties & 0x4000)
#define CAP_W_MODE					(_capabilties & 0x8000) 

// The folowing are available W Modes values
// Rquires CAP_MULTIFINGER (values 0 to 2)
#define W_TWOFINGERS				0
#define W_THREEPLUS					1
// Requires INFO_PEN (values 3)
#define W_PEN						2
// #define W_RESERVED					3
// Requires CAP_PALM_DETECT (values 4 to 15)
#define W_FINGER_MIN				4
#define W_FINGER_MAX				7
#define W_FAT_FINGER_MIN			8
#define W_FAT_FINGER_MAX			14
#define W_MAX_CONTACT				15

// The flowing are available Z values (if we want to use them, W should be sufficient)
#define Z_NO_FINGER					0
#define Z_FINGER_NEAR				10
#define Z_LIGHT_FINGER				30
#define Z_NORMAL_FINGER				80
#define Z_HEAVY_FINGER				110
#define Z_FULL_FINGER				200
#define Z_MAX						255


// ModelID (info about hardware)
#define INFO_SENSOR					((_modelId & 0x3F0000) >> 16)
#define INFO_180					((_modelId & 0x800000) >> 23)
#define INFO_PORTRAIT				((_modelId & 0x400000) >> 22)
#define INFO_HARDWARE				((_modelId & 0x00FE00) >> 9)
#define INFO_NEWABS					((_modelId & 0x000080) >> 7)
#define INFO_SIMPLECMD				((_modelId & 0x000010) >> 5)
#define INTO_PEN					((_modelId & 0x000020) >> 6)

// Boundaries for sidescrolling (curently undefined
#define HORIZ_SCROLLING_BOUNDARY
#define VERT_SCROLLING_BOUNDARY


// Possible touchpad events
#define DEFAULT_EVENT				0
#define SCROLLING				1 << 1
#define HORIZONTAL_SCROLLING	1 << 2
#define VERTICAL_SCROLLING		1 << 3
#define ZOOMIN					1 << 4
#define MOVEMENT				1 << 5

// kST_** = Synaptics Commands (Information queries)
#define kST_IdentifyTouchpad		0x00
#define kST_getTouchpadModeByte		0x01
#define kST_getCapabilities			0x02
#define kST_getModelID				0x03
#define kST_unknown1				0x04
#define kST_unknown2				0x05
#define kST_getSerialNumberPrefix	0x06
#define kST_getSerialNumberSuffix	0x07
#define kST_getResolution			0x08




static char *model_names [] = {	// 16 models currenlty in this list
	"Unknown",
	"Standard TouchPad (TM41xx134)",
	"Mini Module (TM41xx156)",
	"Super Module (TM41xx180)",	
	"Romulan Module",					// Specification does not list (reserved)
	"Apple Module",						// Specification does not list (reserved)
	"Single Chip",						// Specification does not list (reserved)
	"Flexible pad (discontinued)",
	"Ultra-thin Module (TM41xx220)",
	"Wide pad Module (TW41xx230)",
	"Twin Pad module",					// Specification does not list (reserved)
	"Stamp Pad Module (TM41xx240)",
	"SubMini Module (TM41xx140)",
	"MultiSwitch module (TBD)",
	"Standard Thin",					// Specification does not list (reserved)
	"Advanced Technology Pad (TM41xx301)",
	"Ultra-thin Module, connector reversed (TM41xx221)"
};


// Sensor reslutions (yes, I COULD use a struct, but I dont feal like it
#define UNKNOWN_RESOLUTION_X	85
#define UNKNOWN_RESOLUTION_Y	94	

// Resolutions of the sensor (in X x Y)
/*static UInt32 model_resolution [][2] = {
	{85, 94},
	{91, 124},
	{57, 58},
	{UNKNOWN_RESOLUTION_X, UNKNOWN_RESOLUTION_Y},
	{UNKNOWN_RESOLUTION_X, UNKNOWN_RESOLUTION_Y},
	{UNKNOWN_RESOLUTION_X, UNKNOWN_RESOLUTION_Y},
	{UNKNOWN_RESOLUTION_X, UNKNOWN_RESOLUTION_Y},
	{85, 94},
	{73, 96},
	{UNKNOWN_RESOLUTION_X, UNKNOWN_RESOLUTION_Y},
	{187, 170},
	{122, 76},
	{UNKNOWN_RESOLUTION_X, UNKNOWN_RESOLUTION_Y},
	{UNKNOWN_RESOLUTION_X, UNKNOWN_RESOLUTION_Y},
	{UNKNOWN_RESOLUTION_X, UNKNOWN_RESOLUTION_Y},
	{UNKNOWN_RESOLUTION_X, UNKNOWN_RESOLUTION_Y}
};*/


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// ApplePS2SynapticsTouchPad Class Declaration
//

class ApplePS2SynapticsTouchPad : public IOHIPointing 
{
	OSDeclareDefaultStructors( ApplePS2SynapticsTouchPad );

private:
    ApplePS2MouseDevice * _device;
    UInt32                _interruptHandlerInstalled:1;
    UInt32                _powerControlHandlerInstalled:1;
    UInt8                 _packetBuffer[ABSOLUTE_PACKET_SIZE];
    UInt32                _packetByteCount;
    IOFixed               _resolution;
    UInt16                _touchPadVersion;
    UInt8                 _touchPadModeByte;
	UInt32				  _touchpadIntormation;
	UInt32				  _capabilties;
	UInt32				  _modelId;
	UInt8				  _event;
	UInt8				  _prevEvent;
	long long			  _serialNumber;
	
	bool				  _horizScroll;
	bool				  _vertScroll;
	
	UInt32				  _prevX;
	UInt32				  _prevY;
	UInt32				  _prevButtons;
	AbsoluteTime		  _prevPacketTime;
	bool				  _newStream;

	virtual void   dispatchRelativePointerEventWithPacket( UInt8 * packet, UInt32  packetSize );
	virtual void   dispatchAbsolutePointerEventWithPacket( UInt8 * packet, UInt32  packetSize );
	
    virtual void   setCommandByte( UInt8 setBits, UInt8 clearBits );

	
	// Synaptic specific stuff... (added by meklort)
	virtual bool   setRelativeMode();
	virtual bool   setAbsoluteMode();
	virtual bool   setStreamMode( bool enable );

	virtual bool   getCapabilities();
	virtual bool   getModelID();
	virtual bool   identifyTouchpad();
	virtual bool   getTouchpadModes();
	virtual bool   getSerialNumber();
	virtual bool   getResolutions();

	

	
    virtual void   setTouchPadEnable( bool enable );
    virtual UInt32 getTouchPadData( UInt8 dataSelector );
    virtual bool   setTouchPadModeByte( UInt8 modeByteValue,
                                        bool  enableStreamMode = false );

	virtual void   free();
	virtual void   interruptOccurred( UInt8 data );
    virtual void   setDevicePowerState(UInt32 whatToDo);

protected:
	virtual IOItemCount buttonCount();
	virtual IOFixed     resolution();

public:
    virtual bool init( OSDictionary * properties );
    virtual ApplePS2SynapticsTouchPad * probe( IOService * provider,
                                               SInt32 *    score );
    
    virtual bool start( IOService * provider );
    virtual void stop( IOService * provider );
    
    virtual UInt32 deviceType();
    virtual UInt32 interfaceID();

	virtual IOReturn setParamProperties( OSDictionary * dict );
};

#endif /* _APPLEPS2SYNAPTICSTOUCHPAD_H */
