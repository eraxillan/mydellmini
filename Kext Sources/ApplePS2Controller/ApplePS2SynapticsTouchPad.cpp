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
	_prevButtons			   = 0;
	//_prevPacketTime			   = 0;
    _interruptHandlerInstalled = false;
	_newStream				   = true;
    _packetByteCount           = 0;
    _resolution                = (100) << 16; // (100 dpi, 4 counts/mm) (this will be read later on from the device
    _touchPadModeByte          = GESTURES_MODE_BIT; //| ABSOLUTE_MODE_BIT;	// We like absolute mode

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
    request->commands[2].inOrOut  = (kST_IdentifyTouchpad >> 6) & 0x3;
    request->commands[3].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[3].inOrOut  = kDP_SetMouseResolution;
    request->commands[4].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[4].inOrOut  = (kST_IdentifyTouchpad >> 4) & 0x3;
    request->commands[5].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[5].inOrOut  = kDP_SetMouseResolution;
    request->commands[6].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[6].inOrOut  = (kST_IdentifyTouchpad >> 2) & 0x3;
    request->commands[7].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[7].inOrOut  = kDP_SetMouseResolution;
    request->commands[8].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[8].inOrOut  = kST_IdentifyTouchpad & 0x3;
    request->commands[9].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[9].inOrOut  = kDP_GetMouseInformation;	// 24bit data structure
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
	//IOLog("ApplePS2Trackpad: Mouse Information: 0x%X\n",  _touchpadIntormation);

	getCapabilities();
	IOLog("ApplePS2Trackpad: Capabilities 0x%X\n", (unsigned int) (_capabilties));
	
	if(CAP_W_MODE) 	{
		_touchPadModeByte |= W_MODE_BIT | ABSOLUTE_MODE_BIT | RATE_MODE_BIT;
		
		IOLog("ApplePS2Trackpad: W Mode Supported :D\n");
		if(CAP_PALM_DETECT)	IOLog("ApplePS2Trackpad: Palm detection Supported :D\n");
		if(CAP_MULTIFINGER)	IOLog("ApplePS2Trackpad: Multiple finger detection Supported :D\n");
	} else {
		IOLog("ApplePS2Trackpad: W Mode not available, defaulting to Z mode :(\n");
		_touchPadModeByte |= ABSOLUTE_MODE_BIT | RATE_MODE_BIT;

	}
	
	//IOLog("ApplePS2Touchpad: Sratchthat, I'm using relative mode (for all you no developers who dont want bugs...)");


	getModelID();
	IOLog("ApplePS2Trackpad: Detected toucpad controller \"%s\" (ModelID: 0x%X)\n", model_names[INFO_SENSOR & 0x0f], (unsigned int)_modelId);	// anding with 0x0f because we only have 16 versions stored in the char array

	//setSteamMode();
    //
    // Write the TouchPad mode byte value.
    //

    setTouchPadModeByte(_touchPadModeByte);

    //
    // Advertise the current state of the tapping feature.
    //

    setProperty("Clicking", GESTURES, 8);

    //
    // Must add this property to let our superclass know that it should handle
    // trackpad acceleration settings from user space.  Without this, tracking
    // speed adjustments from the mouse prefs panel have no effect.
    //

    setProperty(kIOHIDPointerAccelerationTypeKey, kIOHIDTrackpadAccelerationType);

    //
    // Install our driver's interrupt handler, for asynchronous data delivery.
    //

    _device->installInterruptAction(this, OSMemberFunctionCast(PS2InterruptAction, this, &ApplePS2SynapticsTouchPad::interruptOccurred));
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
	setStreamMode(true);

    //
	// Install our power control handler.
	//

	_device->installPowerControlAction( this, OSMemberFunctionCast(PS2PowerControlAction, this, &ApplePS2SynapticsTouchPad::setDevicePowerState));
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
	
	// In relative mode, 0x08 should be 1. In absolute mode, 0x08 should be 0
    if (_packetByteCount == 0){
		if(data == kSC_Acknowledge) return;
		else if(RELATIVE_MODE && !(data & 0x08)) return;
		else if(ABSOLUTE_MODE && (data & 0x08)) return;
    }

    //
    // Add this byte to the packet buffer. If the packet is complete, that is,
    // we have the three bytes, dispatch this packet for processing.
    //

    _packetBuffer[_packetByteCount++] = data;
    
	
    if (RELATIVE_MODE  && (_packetByteCount == RELATIVE_PACKET_SIZE))
    {
        dispatchRelativePointerEventWithPacket(_packetBuffer, RELATIVE_PACKET_SIZE);
        _packetByteCount = 0;
	} else if (ABSOLUTE_MODE && (_packetByteCount == ABSOLUTE_PACKET_SIZE)) {
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
	//UInt8 Wvalue = 0xFFFFFFFF;
	UInt32	absX, absY;
	UInt32	pressureZ;
	AbsoluteTime now;

	clock_get_uptime((uint64_t *)&now);
	//IOLog("Prev Time: %d \tCur Time: %d \t", _prevPacketTime, now);
	// Reset the _prevX / _prevY to the curent IF the delta time is > 20ms (aka the finger was removed and now is
	
	
	
	// Swap buttons as requested by a user
#ifdef BUTTONS_SWAPED
    if ( (packet[0] & 0x1) ) buttons |= 0x2;  // left button   (bit 0 in packet)
    if ( (packet[0] & 0x2) ) buttons |= 0x1;  // right button  (bit 1 in packet)	
#else 
	buttons = (packet[0] & 0x3);	// buttons =  last two bits of byte one of packet
#endif
	
	//IOLog("Button value: 0x%X\n", buttons);
	pressureZ = packet[2];												//	  (max value is 255)
	absX = (packet[4] | ((packet[1] & 0x0F) << 8) | ((packet[3] & 0x10) << (12 - 4)));
	absY = (packet[5] | ((packet[1] & 0xF0) << 4) | ((packet[3] & 0x20) << (12 - 5)));
	
	long dx = absX - _prevX;
	long dy = absY - _prevY;	// Y is negative for ps2 (according to synaptics)
	

	
	/*if(W_MODE) {
		Wvalue = ((packet[3] & 0x4) >> 2) | ((packet[0] & 0x8) >> 2);	// (max value = 15)
	} else {
		if(((packet[0] & 0x10) >> 4))	Wvalue = -1;		// No finger
		else							Wvalue = 6;	// finger (4 through 7 = finger)
	}*/
	
	
	// Emulate a middle button
	// TODO: add a short (a few ms pause) to each button press so that if they both are pressed within a timeframe, we get the middle button
	/*if ( (buttons & 0x3)  == 0x3)   {
		buttons = 0x4;		// Middle button
	} else if (_prevButtons == 0x4) {
		// Wait for the button states to stableize since we dont want to send unwanted button presses.
		if(buttons == 0) _prevButtons = 0;
		buttons = 0;
	}*/
	
	// Scale dx and dy so that they are within +/- 127
	dx /= ((ABSOLUTE_X_MAX - ABSOLUTE_X_MIN) / 127);
	dy /= ((ABSOLUTE_Y_MAX - ABSOLUTE_Y_MIN) / 127);
	
	// Lets calculate what event we want to handel
	
	// && pressureZ < (Z_FULL_FINGER)) 
	// Ignore when Z is small. According to teh specifications, X and Y are inacurate at Z < 25 (light finger contact)
	
	if(_newStream) {
		// Rest the dx and dy values since prevX and prevY are both 0
		_newStream = false;
		dx = 0;		
		dy = 0;
		
		if(absX > (ABSOLUTE_X_MAX * .9)) _event = VERTICAL_SCROLLING;
		else if((pressureZ < Z_LIGHT_FINGER)) _event = DEFAULT_EVENT;	// We reset dx and dy untill it is a reliable number
		else _event = MOVEMENT;
	} else if(pressureZ < Z_LIGHT_FINGER) {
		_event = DEFAULT_EVENT;
	} else if(_event == VERTICAL_SCROLLING) {
		//_event = VERTICAL_SCROLLING;	// no need to set it to somethign it already is... (although optimization may take care of it)
	} else if((pressureZ > Z_LIGHT_FINGER)) {
		_event = MOVEMENT;
	}
	
	
	//IOLog("Button switch 0x%X\n", buttons);

	switch(_event) {
		case HORIZONTAL_SCROLLING:
			dispatchScrollWheelEvent(0		, .2 * dx, 0, now);
			break;
		case VERTICAL_SCROLLING:
			dispatchScrollWheelEvent(.2 * dy, 0		 , 0, now);
			break;
		case SCROLLING:
			dispatchScrollWheelEvent(.2 * dy, .2 * dx, 0, now);
			break;
		case MOVEMENT:
			dy *= -1;	// PS2 spec has the direction bacwards from what the os wants
						
			dispatchRelativePointerEvent((int) dx, (int) dy, buttons, now);
			break;
		case DEFAULT_EVENT:
		default:
			// No mevement, just send button presses
			if(!_newStream) _newStream = true;
			dispatchRelativePointerEvent(0, 0, buttons, now);
			break;
			
	}
	//IOLog("Buttons sent: 0x%X\n", buttons);


	_prevX = absX;
	_prevY = absY;
	_prevButtons = buttons;
	_prevPacketTime = now;
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


	clock_get_uptime((uint64_t *)&now);

    
	// Swap buttons as requested by a user
#ifdef BUTTONS_SWAPED
    if ( (packet[0] & 0x1) ) buttons |= 0x2;  // left button   (bit 0 in packet)
    if ( (packet[0] & 0x2) ) buttons |= 0x1;  // right button  (bit 1 in packet)	
#else 
	if ( (packet[0] & 0x1) ) buttons |= 0x1;  // left button   (bit 0 in packet)
    if ( (packet[0] & 0x2) ) buttons |= 0x2;  // right button  (bit 1 in packet)
#endif

	if ( (packet[0] & 0x4) ) buttons |= 0x4;  // middle button (bit 2 in packet)
	
	// Emulate a middle button
	// TODO: add a short (a few ms pause) to each button press so that if they both are pressed within a timeframe, we get the middle button
	if ( (buttons & 0x3)  == 0x03)   {
		//buttons = 0x4;		// Middle button
		buttons = 0;	// This is for scrolling
		_event = MOVEMENT;
		_prevButtons = 0x4;
	} else if ((_prevButtons & 0x4) == 0x4) {
		// Wait for the button states to clean
		if(buttons == 0) {
			_prevEvent = _event;
			_prevButtons = 0;
		}
		buttons = 0;

	} else {
		_prevButtons = buttons;
	}
	
	
    dx = ((packet[0] & 0x10) ? 0xffffff00 : 0 ) | packet[1];
    dy = -(((packet[0] & 0x20) ? 0xffffff00 : 0 ) | packet[2]);
    
    //IOLog("Displatching event with dx: %d \tdy: %d\n", dx, dy);
	switch(_event) {
		case SCROLLING:
			// Send scroll event
			// TOD: mke the scaler work over multiple packets(dont drop decmals)
			dispatchScrollWheelEvent(dy * -.2, dx * -.2, 0, now);
			break;
		case MOVEMENT:
		default:
			dispatchRelativePointerEvent(dx, dy, buttons, now);
			break;
	}
	_prevPacketTime = now;

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

    // Disable stream mode before the command sequence. (This doesnt actualy dissable stream mode, according to teh specifications)
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
// This method is used by the prefrences pane to communicate with the driver (currently only supports clicking)
IOReturn ApplePS2SynapticsTouchPad::setParamProperties( OSDictionary * dict )
{
    OSNumber * clicking = OSDynamicCast( OSNumber, dict->getObject("Clicking") );

    if ( clicking )
    {    
		// This now only resets the gesture bit, not everything else (yay, bug is fixed)
		UInt8  newModeByteValue = _touchPadModeByte & ~GESTURES_MODE_BIT;
        clicking->unsigned32BitValue() & 0x1 ? newModeByteValue |= GESTURES_MODE_BIT : 0;

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

//-----------------------------------------------------------------------------//
bool   ApplePS2SynapticsTouchPad::setRelativeMode() {
	if((_touchPadModeByte & ABSOLUTE_MODE_BIT) != 0) { // if the bit is changed
		_touchPadModeByte &= ~(ABSOLUTE_MODE_BIT);
		setTouchPadModeByte(_touchPadModeByte, true);
	}
	return true;
}

// this is not needed (just use the setModeByte command)
bool   ApplePS2SynapticsTouchPad::setAbsoluteMode() {
	if((_touchPadModeByte & ABSOLUTE_MODE_BIT) == 0) {	// If the bit is changed
		_touchPadModeByte |= ABSOLUTE_MODE_BIT;
		setTouchPadModeByte(_touchPadModeByte, true);
	}
	return true;
}

bool   ApplePS2SynapticsTouchPad::setStreamMode( bool enable ) {
	PS2Request * request = _device->allocateRequest();
    bool         success;
	
    if ( !request ) return false;
	
    // Enable steaming mode
    request->commands[0].command  = kPS2C_SendMouseCommandAndCompareAck;
    request->commands[0].inOrOut  = kDP_SetMouseStreamMode;
	// We really should data mode as well
	
	request->commandsCount = 1;
    _device->submitRequestAndBlock(request);
	
	success = (request->commandsCount == 1);
	_device->freeRequest(request);
 	
	return success;
}

bool ApplePS2SynapticsTouchPad::getModelID()
{
	_modelId = getTouchPadData(kST_getModelID);
	if(_modelId != 0) return true;
	return false;
}

bool ApplePS2SynapticsTouchPad::getCapabilities()
{
	bool success = false;
	
	_capabilties = getTouchPadData(kST_getCapabilities);
	if((_capabilties & 0x00FF00 == 0x004700)) success = true;	
	
	
	_capabilties = ((_capabilties & 0xFF0000) >> 8) | (_capabilties & 0x0000FF);

	
	return success;
}


bool   ApplePS2SynapticsTouchPad::identifyTouchpad() {
	_touchpadIntormation = getTouchPadData(kST_IdentifyTouchpad);
	
	/*_touchpadIntormation = request->commands[10].inOrOut | (request->commands[11].inOrOut << 8) | (request->commands[12].inOrOut << 16);
	
    if ( request->commandsCount == 14 &&
		request->commands[11].inOrOut == 0x47 )
    {
        _touchPadVersion = (request->commands[12].inOrOut & 0x0f) << 8
		|  request->commands[10].inOrOut;
	*/
	return true;
}

bool   ApplePS2SynapticsTouchPad::getTouchpadModes() {
	return false;
}
bool   ApplePS2SynapticsTouchPad::getSerialNumber() {
	UInt32 prefix = 0;
	UInt32 serialNumber = 0;
	prefix =getTouchPadData(kST_getSerialNumberPrefix);
	serialNumber = getTouchPadData(kST_getSerialNumberSuffix);
	
	_serialNumber = (serialNumber | ((prefix & 0xFFF) << 24));
	return false;
}
bool   ApplePS2SynapticsTouchPad::getResolutions() {
	return false;
}