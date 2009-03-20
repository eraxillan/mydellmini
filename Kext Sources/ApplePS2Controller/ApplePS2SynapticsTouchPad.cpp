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

#include <IOKit/assert.h>
#include <IOKit/IOLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include "ApplePS2SynapticsTouchPad.h"

enum {
    //
    // Relative mode, 40 packets/s, sleep mode disabled
    //
    kModeByteValueGesturesEnabled  = 0x00,
    kModeByteValueGesturesDisabled = 0x04
};

// =============================================================================
// ApplePS2SynapticsTouchPad Class Implementation
//

#define super IOHIPointing
OSDefineMetaClassAndStructors(ApplePS2SynapticsTouchPad, IOHIPointing);

UInt32 ApplePS2SynapticsTouchPad::deviceType()
{ return NX_EVS_DEVICE_TYPE_MOUSE; };

UInt32 ApplePS2SynapticsTouchPad::interfaceID()
{ return NX_EVS_DEVICE_INTERFACE_BUS_ACE; };

IOItemCount ApplePS2SynapticsTouchPad::buttonCount() { return 2; };
IOFixed     ApplePS2SynapticsTouchPad::resolution()  { return _resolution; };

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool ApplePS2SynapticsTouchPad::init( OSDictionary * properties )
{
    //
    // Initialize this object's minimal state. This is invoked right after this
    // object is instantiated.
    //
    
    if (!super::init(properties))  return false;

    _device                    = 0;
    _interruptHandlerInstalled = false;
    _packetByteCount           = 0;
    _resolution                = (100) << 16; // (100 dpi, 4 counts/mm)
    _touchPadModeByte          = kModeByteValueGesturesDisabled;

    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

ApplePS2SynapticsTouchPad *
ApplePS2SynapticsTouchPad::probe( IOService * provider, SInt32 * score )
{
    //
    // The driver has been instructed to verify the presence of the actual
    // hardware we represent. We are guaranteed by the controller that the
    // mouse clock is enabled and the mouse itself is disabled (thus it
    // won't send any asynchronous mouse data that may mess up the
    // responses expected by the commands we send it).
    //

    ApplePS2MouseDevice * device  = (ApplePS2MouseDevice *) provider;
    PS2Request *          request = device->allocateRequest();
    bool                  success = false;
    
    if (!super::probe(provider, score) || !request) return 0;

    //
    // Send an "Identify TouchPad" command and see if the device is
    // a Synaptics TouchPad based on its response.  End the command
    // chain with a "Set Defaults" command to clear all state.
    //

    request->commands[0].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[0].inOrOut  = kDP_SetDefaultsAndDisable;
    request->commands[1].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[1].inOrOut  = kDP_SetMouseResolution;
    request->commands[2].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[2].inOrOut  = 0;
    request->commands[3].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[3].inOrOut  = kDP_SetMouseResolution;
    request->commands[4].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[4].inOrOut  = 0;
    request->commands[5].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[5].inOrOut  = kDP_SetMouseResolution;
    request->commands[6].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[6].inOrOut  = 0;
    request->commands[7].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[7].inOrOut  = kDP_SetMouseResolution;
    request->commands[8].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[8].inOrOut  = 0;
    request->commands[9].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[9].inOrOut  = kDP_GetMouseInformation;	// 424bit data structure
    request->commands[10].command = kPS2C_ReadDataPort;
    request->commands[10].inOrOut = 0;		// Read first byte
    request->commands[11].command = kPS2C_ReadDataPort;
    request->commands[11].inOrOut = 0;		// Read second byte
    request->commands[12].command = kPS2C_ReadDataPort;
    request->commands[12].inOrOut = 0;		// REad third byte
    request->commands[13].command = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[13].inOrOut = kDP_SetDefaultsAndDisable;
    request->commandsCount = 14;
    device->submitRequestAndBlock(request);

	_touchpadIntormation = 0;
	_touchpadIntormation = request->commands[10].inOrOut | (request->commands[11].inOrOut << 8) | (request->commands[12].inOrOut << 16);
	
    if ( request->commandsCount == 14 &&
         request->commands[11].inOrOut == 0x47 )
    {
        _touchPadVersion = (request->commands[12].inOrOut & 0x0f) << 8
                         |  request->commands[10].inOrOut;

        //
        // Only support 4.x or later touchpads.
        //

        if ( _touchPadVersion >= 0x400 ) success = true;
    }

    device->freeRequest(request);

    return (success) ? this : 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool ApplePS2SynapticsTouchPad::start( IOService * provider )
{ 
    UInt64 gesturesEnabled;

    //
    // The driver has been instructed to start. This is called after a
    // successful probe and match.
    //

    if (!super::start(provider)) return false;

    //
    // Maintain a pointer to and retain the provider object.
    //

    _device = (ApplePS2MouseDevice *) provider;
    _device->retain();

    //
    // Announce hardware properties.
    //

    IOLog("ApplePS2Trackpad: Synaptics TouchPad v%d.%d\n",
          (UInt8)(_touchPadVersion >> 8), (UInt8)(_touchPadVersion));
	setProperty("TouchpadInfo", _touchpadIntormation, sizeof(_touchpadIntormation));
	IOLog("ApplePS@Trackpad: Hardware Information: 0x%X\n",  _touchpadIntormation);

	getCapabilities();
	
	getModelID();
	
    //
    // Write the TouchPad mode byte value.
    //

    setTouchPadModeByte(_touchPadModeByte);

    //
    // Advertise the current state of the tapping feature.
    //

    gesturesEnabled = (_touchPadModeByte == kModeByteValueGesturesEnabled)
                    ? 1 : 0;
    setProperty("Clicking", gesturesEnabled, sizeof(gesturesEnabled)*8);

    //
    // Must add this property to let our superclass know that it should handle
    // trackpad acceleration settings from user space.  Without this, tracking
    // speed adjustments from the mouse prefs panel have no effect.
    //

    setProperty(kIOHIDPointerAccelerationTypeKey, kIOHIDTrackpadAccelerationType);

    //
    // Install our driver's interrupt handler, for asynchronous data delivery.
    //

    _device->installInterruptAction(this,
        OSMemberFunctionCast(PS2InterruptAction, this, &ApplePS2SynapticsTouchPad::interruptOccurred));
    _interruptHandlerInstalled = true;

    //
    // Enable the mouse clock (should already be so) and the mouse IRQ line.
    //

    setCommandByte( kCB_EnableMouseIRQ, kCB_DisableMouseClock );

    //
    // Finally, we enable the trackpad itself, so that it may start reporting
    // asynchronous events.
    //

    setTouchPadEnable(true);

    //
	// Install our power control handler.
	//

	_device->installPowerControlAction( this,
        OSMemberFunctionCast(PS2PowerControlAction, this, &ApplePS2SynapticsTouchPad::setDevicePowerState));
	_powerControlHandlerInstalled = true;

    return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ApplePS2SynapticsTouchPad::stop( IOService * provider )
{
    //
    // The driver has been instructed to stop.  Note that we must break all
    // connections to other service objects now (ie. no registered actions,
    // no pointers and retains to objects, etc), if any.
    //

    assert(_device == provider);

    //
    // Disable the mouse itself, so that it may stop reporting mouse events.
    //

    setTouchPadEnable(false);

    //
    // Disable the mouse clock and the mouse IRQ line.
    //

    setCommandByte( kCB_DisableMouseClock, kCB_EnableMouseIRQ );

    //
    // Uninstall the interrupt handler.
    //

    if ( _interruptHandlerInstalled )  _device->uninstallInterruptAction();
    _interruptHandlerInstalled = false;

    //
    // Uninstall the power control handler.
    //

    if ( _powerControlHandlerInstalled ) _device->uninstallPowerControlAction();
    _powerControlHandlerInstalled = false;

	super::stop(provider);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ApplePS2SynapticsTouchPad::free()
{
    //
    // Release the pointer to the provider object.
    //

    if (_device)
    {
        _device->release();
        _device = 0;
    }

    super::free();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ApplePS2SynapticsTouchPad::interruptOccurred( UInt8 data )
{
    //
    // This will be invoked automatically from our device when asynchronous
    // events need to be delivered. Process the trackpad data. Do NOT issue
    // any BLOCKING commands to our device in this context.
    //
    // Ignore all bytes until we see the start of a packet, otherwise the
    // packets may get out of sequence and things will get very confusing.
    //
    if (_packetByteCount == 0 && ((data == kSC_Acknowledge) || !(data & 0x08)))
    {
        return;
    }

    //
    // Add this byte to the packet buffer. If the packet is complete, that is,
    // we have the three bytes, dispatch this packet for processing.
    //

    _packetBuffer[_packetByteCount++] = data;
    
	
    if (RELATIVE_MODE && (_packetByteCount == RELATIVE_PACKET_SIZE))
    {
        dispatchRelativePointerEventWithPacket(_packetBuffer, RELATIVE_PACKET_SIZE);
        _packetByteCount = 0;
	} else if (ABSULUTE_MODE && (_packetByteCount == ABSOLUTE_PACKET_SIZE)) {
		dispatchAbsolutePointerEventWithPacket(_packetBuffer, ABSOLUTE_PACKET_SIZE);
		_packetByteCount = 0;
	}
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ApplePS2SynapticsTouchPad::
dispatchAbsolutePointerEventWithPacket( UInt8 * packet,
									   UInt32  packetSize )
{
	// Packet without W mode dissabled	(twice as long as relativ mode, first two bits specify which packer (10 = 1, 11 = 2)
	//	7		6		5			4			3		2		1		0
	
	//	{1}		{0}		Finger		Reserved	{0}		Gesture Right	Left
	//	Y11		Y10		Y9			Y8			X11		X10		X9		X8			(Y 11..8) (X 11..8)
	//	Z7		Z6		Z5			Z4			Z3		Z2		Z1		Z0			(Z  7..0)
	//	{1}		{1}		Y12			X12			{0}		Gesture	Right	Left		(Y12) (X12)
	//	X7		X6		X5			X4			X3		X2		X1		X0			(X 7..0)
	//	Y7		Y6		Y5			Y4			Y3		Y2		Y1		Y0			(Y 7..0)
	
	// With W mode enabes
	//	7		6		5			4			3		2		1		0
	
	//	{1}		{0}		W3			W2			{0}		W1		Right	Left		(W 3..2) (W1)
	//	Y11		Y10		Y9			Y8			X11		X10		X9		X8			(Y 11..8) (X 11..8)
	//	Z7		Z6		Z5			Z4			Z3		Z2		Z1		Z0			(Z  7..0)
	//	{1}		{1}		Y12			X12			{0}		W0		R/D		R/L			(W0) (Y12) (X12)
	//	X7		X6		X5			X4			X3		X2		X1		X0			(X 7..0)
	//	Y7		Y6		Y5			Y4			Y3		Y2		Y1		Y0			(Y 7..0)
	
	UInt32 buttons = 0;
	UInt8 event = 0;
	UInt8 Wvalue = 0xFFFFFFFF;
	UInt32	absX, absY;		// Acual size is 8 * 12 or 96, we only use 8*8 or 64
	UInt32	pressureZ;
	AbsoluteTime now;

	//bool rightButton, leftButton;
	
	// Read the packets and put teh info into usable variables...
	//rightButton = ((packet[0] & 0x2) >> 1);
	//leftButton =   (packet[0] & 0x1);
	buttons |= (packet[0] & 0x3);	// buttons =  last two bits of byte one of packet
	
	pressureZ = packet[2];												//	  (max value is 255)
	//			0 000 1111 1111	 0 1111 0000 0000			 1 0000 0000 0000 (max value is 6143)
	absX = ((0XFF & packet[4]) | ((packet[3] & 0x0F) << 8) | ((packet[3] & 0x10) << (12 - 4)));		// TODO verify this (aka its wrong)
	absY = ((0xFF & packet[5]) | ((packet[3] & 0x0F) << 4) | ((packet[3] & 0x20) << (12 - 5)));				// VERIFY this too
	
	UInt32 dx = _prevX - absX;
	UInt32 dy = absY - _prevY;	// Y is negative for ps2 (according to synaptics)
	

	
	if(W_MODE) {
		Wvalue = ((packet[3] & 0x4) >> 2) | ((packet[0] & 0x8) >> 2);	// (max value = 15)
	} else {
		if(((packet[0] & 0x10) >> 4))	Wvalue = -1;		// No finger
		else							Wvalue = 6;	// finger (4 through 7 = finger)
	}
	
	
	// Now that we have read the info from the packet, lets handel it
	
	// Check if we just had a finger press (on the right side of the touchpad_
	
	//if((buttons & 0x1) ^ (_prevButtons & 0x1))	{
		// If left button state changed
	//}
	
	//if((buttons & 0x2) ^ (_prevButtons & 0x2))	{		// Right button change
	//}

	
	// For now we only support movement on teh trackpad
	event = MOVEMENT;
	
	switch(event) {
		case HORIZONTAL_SCROLLING:
			//  virtual void dispatchScrollWheelEvent(short deltaAxis1,
			//short deltaAxis2,
			//short deltaAxis3,
			//AbsoluteTime ts);
			
			// Calculate the HorixontalDelta based on dx
			
			dispatchScrollWheelEvent(0, /*scrollWheelHorizontalDelta*/ dx, 0, now);
			break;
		case VERTICAL_SCROLLING:
			dispatchScrollWheelEvent(dy /*scrollWheelVerticalDelta*/, 0, 0, now);
			break;
		case MOVEMENT:
		default:
			// Lets send the movement info to the os.
			// Default is movement...
			clock_get_uptime((uint64_t *)&now);
			dispatchRelativePointerEvent(dx, dy, buttons, now);
	}
	   
	_prevX = absX;
	_prevY = absY;
	_prevButtons = buttons;
	// Absolute pointer event may not be needed?? (Look in IOHIDFamily -> IOHIDPointer.cpp
	/*dispatchAbsolutePointerEvent(IOGPoint *  newLoc,
								 IOGBounds *	bounds,
								 UInt32		buttonState,
								 bool		proximity,
								 int		pressure,
								 int		pressureMin,
								 int		pressureMax,
								 int		stylusAngle,
								 AbsoluteTime	ts);*/
	
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ApplePS2SynapticsTouchPad::
     dispatchRelativePointerEventWithPacket( UInt8 * packet,
                                             UInt32  packetSize )
{
    //
    // Process the three byte relative format packet that was retreived from the
    // trackpad. The format of the bytes is as follows:
    //
    //  7  6  5  4  3  2  1  0
    // -----------------------
    // YO XO YS XS  1  M  R  L
    // X7 X6 X5 X4 X3 X3 X1 X0  (X delta)
    // Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0  (Y delta)
    //

    UInt32       buttons = 0;
    SInt32       dx, dy;
    AbsoluteTime now;

    if ( (packet[0] & 0x1) ) buttons |= 0x1;  // left button   (bit 0 in packet)
    if ( (packet[0] & 0x2) ) buttons |= 0x2;  // right button  (bit 1 in packet)
    if ( (packet[0] & 0x4) ) buttons |= 0x4;  // middle button (bit 2 in packet)
    
    dx = ((packet[0] & 0x10) ? 0xffffff00 : 0 ) | packet[1];
    dy = -(((packet[0] & 0x20) ? 0xffffff00 : 0 ) | packet[2]);

    clock_get_uptime((uint64_t *)&now);
    
    dispatchRelativePointerEvent(dx, dy, buttons, now);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ApplePS2SynapticsTouchPad::setTouchPadEnable( bool enable )
{
    //
    // Instructs the trackpad to start or stop the reporting of data packets.
    // It is safe to issue this request from the interrupt/completion context.
    //

    PS2Request * request = _device->allocateRequest();
    if ( !request ) return;

    // (mouse enable/disable command)
    request->commands[0].command = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[0].inOrOut = (enable)?kDP_Enable:kDP_SetDefaultsAndDisable;
    request->commandsCount = 1;
    _device->submitRequestAndBlock(request);
    _device->freeRequest(request);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

UInt32 ApplePS2SynapticsTouchPad::getTouchPadData( UInt8 dataSelector )
{
    PS2Request * request     = _device->allocateRequest();
    UInt32       returnValue = (UInt32)(-1);

    if ( !request ) return returnValue;

    // Disable stream mode before the command sequence.
    request->commands[0].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[0].inOrOut  = kDP_SetDefaultsAndDisable;

    // 4 set resolution commands, each encode 2 data bits.
    request->commands[1].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[1].inOrOut  = kDP_SetMouseResolution;
    request->commands[2].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[2].inOrOut  = (dataSelector >> 6) & 0x3;

    request->commands[3].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[3].inOrOut  = kDP_SetMouseResolution;
    request->commands[4].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[4].inOrOut  = (dataSelector >> 4) & 0x3;

    request->commands[5].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[5].inOrOut  = kDP_SetMouseResolution;
    request->commands[6].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[6].inOrOut  = (dataSelector >> 2) & 0x3;

    request->commands[7].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[7].inOrOut  = kDP_SetMouseResolution;
    request->commands[8].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[8].inOrOut  = (dataSelector >> 0) & 0x3;

    // Read response bytes.
    request->commands[9].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[9].inOrOut  = kDP_GetMouseInformation;
    request->commands[10].command = kPS2C_ReadDataPort;
    request->commands[10].inOrOut = 0;
    request->commands[11].command = kPS2C_ReadDataPort;
    request->commands[11].inOrOut = 0;
    request->commands[12].command = kPS2C_ReadDataPort;
    request->commands[12].inOrOut = 0;

    request->commandsCount = 13;
    _device->submitRequestAndBlock(request);

    if (request->commandsCount == 13) // success?
    {
        returnValue = ((UInt32)request->commands[10].inOrOut << 16) |
                      ((UInt32)request->commands[11].inOrOut <<  8) |
                      ((UInt32)request->commands[12].inOrOut);
    }

    _device->freeRequest(request);

    return returnValue;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

bool ApplePS2SynapticsTouchPad::setTouchPadModeByte( UInt8 modeByteValue,
                                                     bool  enableStreamMode )
{
    PS2Request * request = _device->allocateRequest();
    bool         success;

    if ( !request ) return false;

    // Disable stream mode before the command sequence.
    request->commands[0].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[0].inOrOut  = kDP_SetDefaultsAndDisable;

    // 4 set resolution commands, each encode 2 data bits.
    request->commands[1].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[1].inOrOut  = kDP_SetMouseResolution;
    request->commands[2].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[2].inOrOut  = (modeByteValue >> 6) & 0x3;

    request->commands[3].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[3].inOrOut  = kDP_SetMouseResolution;
    request->commands[4].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[4].inOrOut  = (modeByteValue >> 4) & 0x3;

    request->commands[5].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[5].inOrOut  = kDP_SetMouseResolution;
    request->commands[6].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[6].inOrOut  = (modeByteValue >> 2) & 0x3;

    request->commands[7].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[7].inOrOut  = kDP_SetMouseResolution;
    request->commands[8].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[8].inOrOut  = (modeByteValue >> 0) & 0x3;

    // Set sample rate 20 to set mode byte 2. Older pads have 4 mode
    // bytes (0,1,2,3), but only mode byte 2 remain in modern pads.
    request->commands[9].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[9].inOrOut  = kDP_SetMouseSampleRate;
    request->commands[10].command = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[10].inOrOut = 20;

    request->commands[11].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[11].inOrOut  = enableStreamMode ?
                                     kDP_Enable :
                                     kDP_SetMouseScaling1To1; /* Nop */

    request->commandsCount = 12;
    _device->submitRequestAndBlock(request);

    success = (request->commandsCount == 12);

    _device->freeRequest(request);
    
    return success;
}

bool ApplePS2SynapticsTouchPad::getModelID()
{
    PS2Request * request = _device->allocateRequest();
    bool         success;
	
    if ( !request ) return false;
	
    // Disable stream mode before the command sequence.
    request->commands[0].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[0].inOrOut  = kDP_SetDefaultsAndDisable;
	
    // 4 set resolution commands, each encode 2 data bits.
    request->commands[1].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[1].inOrOut  = kST_getModelID;
    request->commands[2].command  = kPS2C_ReadDataPort;
    request->commands[2].inOrOut  = 0;
    request->commands[3].command  = kPS2C_ReadDataPort;
    request->commands[3].inOrOut  = 0;
    request->commands[4].command  = kPS2C_ReadDataPort;
    request->commands[4].inOrOut  = 0;
	
    request->commandsCount = 5;
    _device->submitRequestAndBlock(request);
	
    success = (request->commandsCount == 5);
	_modelId = 0;
	_modelId = request->commands[2].inOrOut | (request->commands[3].inOrOut << 8)| (request->commands[4].inOrOut << 16);
	IOLog("ApplePS2Trackpad: Detected toucpad controller \"%s\"\n", model_names[(_modelId & 0x3F0000) >> 16]);
	
	_device->freeRequest(request);
    
    return success;
}

bool ApplePS2SynapticsTouchPad::getCapabilities()
{
    PS2Request * request = _device->allocateRequest();
    bool         success;
	
    if ( !request ) return false;
	
    // Disable stream mode before the command sequence.
    request->commands[0].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[0].inOrOut  = kDP_SetDefaultsAndDisable;
	
    // 4 set resolution commands, each encode 2 data bits.
    request->commands[1].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[1].inOrOut  = kST_getCapabilities;
    request->commands[2].command  = kPS2C_ReadDataPort;
    request->commands[2].inOrOut  = 0;
    request->commands[3].command  = kPS2C_ReadDataPort;
    request->commands[3].inOrOut  = 0;
    request->commands[4].command  = kPS2C_ReadDataPort;
    request->commands[4].inOrOut  = 0;
	
    request->commandsCount = 5;
    _device->submitRequestAndBlock(request);
	
    success = (request->commandsCount == 5);
	_capabilties = 0;
	_capabilties = request->commands[2].inOrOut | (request->commands[3].inOrOut << 8)| (request->commands[4].inOrOut << 16);
	IOLog("ApplePS2Trackpad: Capabilities 0x%X\n", _capabilties);
	
	_device->freeRequest(request);
    
    return success;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ApplePS2SynapticsTouchPad::setCommandByte( UInt8 setBits, UInt8 clearBits )
{
    //
    // Sets the bits setBits and clears the bits clearBits "atomically" in the
    // controller's Command Byte.   Since the controller does not provide such
    // a read-modify-write primitive, we resort to a test-and-set try loop.
    //
    // Do NOT issue this request from the interrupt/completion context.
    //

    UInt8        commandByte;
    UInt8        commandByteNew;
    PS2Request * request = _device->allocateRequest();

    if ( !request ) return;

    do
    {
        // (read command byte)
        request->commands[0].command = kPS2C_WriteCommandPort;
        request->commands[0].inOrOut = kCP_GetCommandByte;
        request->commands[1].command = kPS2C_ReadDataPort;
        request->commands[1].inOrOut = 0;
        request->commandsCount = 2;
        _device->submitRequestAndBlock(request);

        //
        // Modify the command byte as requested by caller.
        //

        commandByte    = request->commands[1].inOrOut;
        commandByteNew = (commandByte | setBits) & (~clearBits);

        // ("test-and-set" command byte)
        request->commands[0].command = kPS2C_WriteCommandPort;
        request->commands[0].inOrOut = kCP_GetCommandByte;
        request->commands[1].command = kPS2C_ReadDataPortAndCompare;
        request->commands[1].inOrOut = commandByte;
        request->commands[2].command = kPS2C_WriteCommandPort;
        request->commands[2].inOrOut = kCP_SetCommandByte;
        request->commands[3].command = kPS2C_WriteDataPort;
        request->commands[3].inOrOut = commandByteNew;
        request->commandsCount = 4;
        _device->submitRequestAndBlock(request);

        //
        // Repeat this loop if last command failed, that is, if the
        // old command byte was modified since we first read it.
        //

    } while (request->commandsCount != 4);  

    _device->freeRequest(request);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

IOReturn ApplePS2SynapticsTouchPad::setParamProperties( OSDictionary * dict )
{
    OSNumber * clicking = OSDynamicCast( OSNumber, dict->getObject("Clicking") );

    if ( clicking )
    {    
        UInt8  newModeByteValue = clicking->unsigned32BitValue() & 0x1 ?
                                  kModeByteValueGesturesEnabled :
                                  kModeByteValueGesturesDisabled;

        if (_touchPadModeByte != newModeByteValue)
        {
            _touchPadModeByte = newModeByteValue;

            //
            // Write the TouchPad mode byte value.
            //

            setTouchPadModeByte(_touchPadModeByte, true);

            //
            // Advertise the current state of the tapping feature.
            //

            setProperty("Clicking", clicking);
        }
    }

    return super::setParamProperties(dict);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ApplePS2SynapticsTouchPad::setDevicePowerState( UInt32 whatToDo )
{
    switch ( whatToDo )
    {
        case kPS2C_DisableDevice:
            
            //
            // Disable touchpad (synchronous).
            //

            setTouchPadEnable( false );
            break;

        case kPS2C_EnableDevice:

            //
            // Must not issue any commands before the device has
            // completed its power-on self-test and calibration.
            //

            IOSleep(1000);

            setTouchPadModeByte( _touchPadModeByte );

            //
            // Enable the mouse clock (should already be so) and the
            // mouse IRQ line.
            //

            setCommandByte( kCB_EnableMouseIRQ, kCB_DisableMouseClock );

            //
            // Clear packet buffer pointer to avoid issues caused by
            // stale packet fragments.
            //

            _packetByteCount = 0;

            //
            // Finally, we enable the trackpad itself, so that it may
            // start reporting asynchronous events.
            //

            setTouchPadEnable( true );
            break;
    }
}
