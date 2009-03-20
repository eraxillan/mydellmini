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


#define RELATIVE_PACKET_SIZE	3
#define ABSOLUTE_PACKET_SIZE	6

//TODO set thit bit
#define ABSOULTE_MODE_BITMAP		0
#define REALTIVE_MODE_BITMAP		0

//#define ABSULUTE_MODE			(_touchPadModeByte & ABSOULTE_MODE_BITMAP)
#define ABSULUTE_MODE			0
//#define RELATIVE_MODE			(_touchPadModeByte & REALTIVE_MODE_BITMAP)
#define RELATIVE_MODE			1
#define W_MODE					0

// Boundaries for sidescrolling
#define HORIZ_SCROLLING_BOUNDARY
#define VERT_SCROLLING_BOUNDARY


// Possible touchpad events
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


static char *model_names [] = {
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
	UInt8				  _modelId;
	
	bool				  _horizScroll;
	bool				  _vertScroll;
	
	UInt32				  _prevX;
	UInt32				  _prevY;
	UInt32				  _prevButtons;

	virtual void   dispatchRelativePointerEventWithPacket( UInt8 * packet, UInt32  packetSize );
	virtual void   dispatchAbsolutePointerEventWithPacket( UInt8 * packet, UInt32  packetSize );
	
    virtual void   setCommandByte( UInt8 setBits, UInt8 clearBits );

	//virtual void   setRelativeMode( boot enable );
	//virtual void   setAbsoluteMode( boot enable );
	virtual bool   getCapabilities();
	virtual bool   getModelID();

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
