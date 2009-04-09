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


#define ABS(a)	   (((a) < 0) ? -(a) : (a))


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
IOFixed     ApplePS2SynapticsTouchPad::scrollResolution()  { return _resolution; };


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
    _interruptHandlerInstalled = false;
	_newStream				   = true;
    _packetByteCount           = 0;
	// FIXME: I really need to look into _resolution and stuff (synaptics says the dpi is 2159 x 2388)
    //_resolution                = ((int)(UNKNOWN_RESOLUTION_X * 25.4)) << 16; // (100 dpi, 4 counts/mm) (this will be read later on from the device
	_resolution				   = (100) << 16; 
    _touchPadModeByte          = 0; //GESTURES_MODE_BIT; //| ABSOLUTE_MODE_BIT;	// We like absolute mode
	
	// Defaults...
	_prefEdgeScroll			= false;
	_prefHorizScroll		= false;
	_prefClicking			= false;
	_prefDragging			= false;
	_prefDragLock			= false;
	
	
	_prefSensitivity		= 350;
	_prefScrollArea			= .05;
	_prefScrollSpeed		= .5;
	_prefTrackSpeed			= .2;
	
	_prefAccelRate			= 1;
	

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


	getCapabilities();
	IOLog("ApplePS2Trackpad: Capabilities 0x%X\n", (unsigned int) (_capabilties));
	
	if(CAP_W_MODE) 	{
		_touchPadModeByte |= W_MODE_BIT | ABSOLUTE_MODE_BIT | RATE_MODE_BIT;
		
		IOLog("ApplePS2Trackpad: W Mode Supported :D\n");
		if(CAP_PALM_DETECT)	IOLog("ApplePS2Trackpad: Palm detection Supported :D\n");
		if(CAP_MULTIFINGER)	IOLog("ApplePS2Trackpad: Multiple finger detection Supported :D\n");
		else	IOLog("ApplePS2Trackpad: Multiple finger detection NOT Supported :(\n");

	} else {
		IOLog("ApplePS2Trackpad: W Mode not available, defaulting to Z mode :(\n");
		_touchPadModeByte |= ABSOLUTE_MODE_BIT | RATE_MODE_BIT;

	}
	
	IOLog("ApplePS2Trackpad: Initializing resolution to %d dpi\n", (int)(model_resolution[(INFO_SENSOR & 0x0f)][0] * 25.4));
	_resolution = (int)(model_resolution[(INFO_SENSOR & 0x0f)][0] * 25.4) << 16;
	

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
	// FIXME: If ABSOLUTE)MODE && (data & 0x08) is not 0, we need to reset the touchpad (brownout detections)
	if (_packetByteCount == 0){
		if(data == kSC_Acknowledge) return;
		else if(RELATIVE_MODE && !(data & 0x08)) return;
		else if(ABSOLUTE_MODE && (data & 0x08)) {
			setDevicePowerState(kPS2C_EnableDevice);	// The device had a brown out, reset it to absolute mode
			return;
		}
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
	UInt32	absX, absY;
	UInt32	pressureZ;
	UInt8	Wvalue;
	UInt8	prevEvent = _event;
	AbsoluteTime now;
	uint32_t currentTime, second;

	clock_get_uptime((uint64_t *)&now);
	clock_get_system_microtime(&second, &currentTime);
	
	
	// TODO: pssibly Reset the _prevX / _prevY to the curent IF the delta time is > 20ms (aka the finger was removed and now is
	
	
	
	// Swap buttons as requested by a user
#ifdef BUTTONS_SWAPED
    if ( (packet[0] & 0x1) ) buttons |= 0x2;  // left button   (bit 0 in packet)
    if ( (packet[0] & 0x2) ) buttons |= 0x1;  // right button  (bit 1 in packet)	
#else 
	buttons = (packet[0] & 0x3);	// buttons =  last two bits of byte one of packet
#endif
	
	pressureZ = packet[2];												//	  (max value is 255)
	absX = (packet[4] | ((packet[1] & 0x0F) << 8) | ((packet[3] & 0x10) << (12 - 4)));
	absY = (packet[5] | ((packet[1] & 0xF0) << 4) | ((packet[3] & 0x20) << (12 - 5)));
	
	long dx = absX - _prevX;
	long dy = absY - _prevY;	// Y is negative for ps2 (according to synaptics)
	
	if(_prevPacketSecond != second) _prevPacketTime += 1000000;	// Take into account when the microseconds wrap around
	long dt = currentTime - _prevPacketTime;
	
	
	if(W_MODE) Wvalue = ((packet[3] & 0x4) >> 2) | ((packet[0] & 0x30) >> 2) | ((packet[0] & 0x4) >> 1);	// (max value = 15)
	else Wvalue = W_FINGER_MIN;
	
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
	if((_event & (SCROLLING | VERTICAL_SCROLLING | HORIZONTAL_SCROLLING)) == 0) {
		dx *= _prefTrackSpeed;
		dy *= _prefTrackSpeed;
	} else {
		dx *= _prefScrollSpeed;
		dy *= _prefScrollSpeed;
	}

	
	dy *= ((double) (model_resolution[(INFO_SENSOR & 0x0f)][0])) / (model_resolution[(INFO_SENSOR & 0x0f)][0]);

	
	_streamdx += dx;
	_streamdy += dy;

	// TODO: calculate a drift so that we only add 1 when the DRIFT is over .5, not when dx is
	dx += (dx > 0) ? 0.5 : -0.5;
	dy += (dy > 0) ? 0.5 : -0.5;

	//	if(dx > 0) dx += .5;
//	else dx -= .5;
	
//	if(dy > 0) dy += .5;
//	else dy -= .5;
	
	
	
	// Lets calculate what event we want to handel
	
	
	if(_newStream) {
		// Rest the dx and dy values since prevX and prevY are both 0
		_newStream = false;
		dx = 0;		
		dy = 0;
		dt = 0;
		_streamdx = 0;
		_streamdy = 0;
		_streamStartSecond = second;
		_streamStartMicro = currentTime;

		
		// TODO: Only do this IF tapping is enabled
		if(_prefClicking &&
		   _prefDragging &&
		   _tapped &&
		   ((_streamStartSecond == _prevPacketSecond) && 
			//((_streamStartMicro + TAP_LENGTH_MIN) < _prevPacketTime) &&				// Above MIN and
			((_streamStartMicro + TAP_LENGTH_MAX) > _prevPacketTime)					// Less than MAX
			) ||
		   ((_streamStartSecond == _prevPacketSecond + 1) && // else If the second rolled over
			//((_streamStartMicro + TAP_LENGTH_MIN + 10000000) < _prevPacketTime) &&		// Between MIN
			((_streamStartMicro + TAP_LENGTH_MAX + 10000000) > _prevPacketTime)		// and MAX
			)
		   ) {
			_dragging = true;
		} else _dragging = false;
		
		_tapped = false;
		
		
		
		if(_prefEdgeScroll &&
		   (absX > (ABSOLUTE_X_MAX * (1 - _prefScrollArea)))) _event = VERTICAL_SCROLLING;
		else if(_prefEdgeScroll && _prefHorizScroll&&
		   (absY < (ABSOLUTE_Y_MIN * (1 + _prefScrollArea)))) _event = HORIZONTAL_SCROLLING;

		
		else if(_event == SCROLLING &&
				((_streamStartSecond == _prevPacketSecond) && ((_streamStartMicro + TAP_LENGTH_MAX) > _prevPacketTime)) ||
				((_streamStartSecond == _prevPacketSecond + 1) && ((_streamStartMicro + TAP_LENGTH_MAX + 10000000) > _prevPacketTime)		// and MAX
				 )
				) {
			_event =SCROLLING;
				
			
		}
		else if((W_MODE) && ((Wvalue * pressureZ) > _prefSensitivity))							 _event = SCROLLING;
		//else if((W_MODE) && (Wvalue >= W_FINGER_MAX))	_event = SCROLLING;

		else if((pressureZ < Z_LIGHT_FINGER))									 _event = DEFAULT_EVENT;	// We reset dx and dy untill it is a reliable number
		else																	 _event = MOVEMENT;
	} 
	else if(pressureZ < Z_LIGHT_FINGER)				_event = DEFAULT_EVENT;
	else if(_event & (VERTICAL_SCROLLING | HORIZONTAL_SCROLLING));	// This is jsut here so it is not reset MOVEMENT or SCROLLING (wouldnt matter as much)
	else if((_event == SCROLLING) && (Wvalue < W_RESERVED));	// our touchpad doesnt support anything below W_RESERVED so keep on scrolling
	
	else if((W_MODE) && ((Wvalue * pressureZ) > _prefSensitivity))							 _event = SCROLLING;
	//else if((W_MODE) && (Wvalue >= W_FINGER_MAX))	_event = SCROLLING;
	
	else if((pressureZ > Z_LIGHT_FINGER))			_event = MOVEMENT;
	
	
	if(prevEvent ^ _event) {
		dx = 0;		
		dy = 0;
		dt = 0;
		_streamdx = 0;
		_streamdy = 0;
	//	_streamStartSecond = second;
	//	_streamStartMicro = currentTime;
	}
		
	
	
	// Shouldn't be needed.... but hey, why not
	if(_prefDragging) buttons |= _dragging;

	
	
	switch(_event) {
		case HORIZONTAL_SCROLLING:
			dispatchScrollWheelEvent(0,  -1 * dx, 0, now);
			break;
		case VERTICAL_SCROLLING:
			dispatchScrollWheelEvent(dy, 0,		  0, now);
			break;
		case SCROLLING:
			dispatchScrollWheelEvent(dy, -1 * dx, 0, now);
			break;
			
		case MOVEMENT:
			dy *= -1;	// PS2 spec has the direction backwards from what the os wants (lower left corner is the orign, verses upper left)
			dispatchRelativePointerEvent((int) dx, (int) dy, buttons, now);
			break;
			
		// No mevement, just send button presses
		case DEFAULT_EVENT:
		default:

			if(!_newStream) _newStream = true;
			
			if(_prefClicking &&
			   !_dragging &&
			   !_tapped &&
			   (ABS(_streamdx) <= 4) && (ABS(_streamdy) <= 4) &&	// If very little movement, absX and absY would be better (more precision)
			   ((_streamStartSecond == second) && 
				  ((_streamStartMicro + TAP_LENGTH_MIN) < currentTime) &&				// Between MIN
				  ((_streamStartMicro + TAP_LENGTH_MAX) > currentTime)					// And MAX
			   ) ||
			   ((_streamStartSecond == second + 1) && // else If the second rolled over
				  ((_streamStartMicro + TAP_LENGTH_MIN + 10000000) < currentTime) &&		// Between MIN
			      ((_streamStartMicro + TAP_LENGTH_MAX + 10000000) > currentTime)		// and MAX
				)
			   ) {
				_tapped = true;
				buttons = 0x01;
			} 
			dispatchRelativePointerEvent(0, 0, buttons, now);

			break;
			
	}
	//IOLog("Buttons sent: 0x%X\n", buttons);

	//IOLog("Prev Time: %d \tCur Time: %d \t dt: %d\t Tap: %d\n", _prevPacketTime, currentTime, dt, _tapping);


	_prevX = absX;
	_prevY = absY;
	_prevButtons = buttons;
	_prevPacketSecond = second;
	_prevPacketTime = currentTime;
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
	uint32_t	 currentTime, second;


	clock_get_uptime((uint64_t *)&now);
	clock_get_system_microtime(&second, &currentTime);
    
	// Swap buttons as requested by a user
#ifdef BUTTONS_SWAPED
    if ( (packet[0] & 0x1) ) buttons |= 0x2;  // left button   (bit 0 in packet)
    if ( (packet[0] & 0x2) ) buttons |= 0x1;  // right button  (bit 1 in packet)	
	if ( (packet[0] & 0x4) ) buttons |= 0x4;  // middle button (bit 2 in packet)
#else 
	buttons = (packet[0] & 0x7);	// buttons =  last three bits of byte one of packet = middle, right, left
#endif

	
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
	_prevPacketSecond = second;
	_prevPacketTime = currentTime;

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
// This method is used by the prefrences pane to communicate with the driver (currently only supports clicking) called when IORegistryEntrySetCFProperties
IOReturn ApplePS2SynapticsTouchPad::setParamProperties( OSDictionary * dict )
{
    OSNumber *   clicking = OSDynamicCast( OSNumber, dict->getObject("Clicking") );
	

	

    if ( clicking )
    {    
	/*	// This now only resets the gesture bit, not everything else (yay, bug is fixed)
		UInt8  newModeByteValue = _touchPadModeByte & ~ GESTURES_MODE_BIT; // Clear out the gesture bit
		
        if(RELATIVE_MODE) clicking->unsigned32BitValue() & 0x1 ? newModeByteValue |= GESTURES_MODE_BIT : 0;	// set it if the user wants us to

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
        }*/
    } 
	else if(OSDynamicCast(OSBoolean, dict->getObject(kTPEdgeScrolling)))
	{
		_prefEdgeScroll		= ((OSBoolean * )OSDynamicCast(OSBoolean, dict->getObject(kTPEdgeScrolling)))->getValue();
		_prefHorizScroll	= ((OSBoolean * )OSDynamicCast(OSBoolean, dict->getObject(kTPHorizScroll)))->getValue();
		_prefClicking		= ((OSBoolean * )OSDynamicCast(OSBoolean, dict->getObject(kTPTapToClick)))->getValue();
		_prefDragging		= ((OSBoolean * )OSDynamicCast(OSBoolean, dict->getObject(kTPDraggin)))->getValue();
		_prefDragLock		= ((OSBoolean * )OSDynamicCast(OSBoolean, dict->getObject(kTPDragLock)))->getValue();
	
		_prefSensitivity	= (300 + 25 * ((OSNumber * )OSDynamicCast(OSNumber, dict->getObject(kTPSensitivity)))->unsigned32BitValue());
		_prefScrollArea		= (.01 * ((OSNumber * )OSDynamicCast(OSNumber, dict->getObject(kTPScrollArea)))->unsigned32BitValue());
		_prefScrollSpeed	= .15 + (.025 * ((OSNumber * )OSDynamicCast(OSNumber, dict->getObject(kTPScrollSpeed)))->unsigned32BitValue());
		_prefTrackSpeed		= (.2 * ((OSNumber * )OSDynamicCast(OSNumber, dict->getObject(kTPTrackSpeed)))->unsigned32BitValue());
		_prefAccelRate		= (.01 * ((OSNumber * )OSDynamicCast(OSNumber, dict->getObject(kTPAccelRate)))->unsigned32BitValue());
	}


    return super::setParamProperties(dict);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void ApplePS2SynapticsTouchPad::setDevicePowerState( UInt32 whatToDo )
{
    switch ( whatToDo )
    {
        case kPS2C_DisableDevice:
            setTouchPadEnable( false );
            break;

        case kPS2C_EnableDevice:
			// Reset vars for the absolute mode handler.
			_streamdx = 0;
			_streamdy = 0;
			_newStream = true;
			//_streamStartSecond = second;
			//_streamStartMicro = currentTime;
            
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
	
	setProperty("Serial Number", _serialNumber, sizeof(_serialNumber));
	return true;
}
bool   ApplePS2SynapticsTouchPad::getResolutions() {
	return false;
}