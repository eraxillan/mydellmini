## Keyboard Enhancements ##
The keyboard kext currently does not support international buttons. Also, some of the buttons that we map either do nothing, or do something not traditionaly to os x. We can use a preference pane to allow the user to customize certain behaviors (such as what the context key does, the wireless, or battery key)

## Trackpad / Keyboard wake support ##
According to the Acpi specification, any device that is capable of waking the sistem form slepp sounld have a dsdt method "_PRW". This method tells the os / bios waht the lowest power state the device can go to. In our case, we want to disable sleeping for the PS2{K,M} devices so that they can provide wake events. The ApplePS2Controller already puts the touchpad into its own sleep power state that allows to button clicks, so it will not use too much more power._

## DSDT Exit code fix ##
Currently the dsdt patcher uses a lot of exec() calls when they are not needed. We can fix this so that the patcher will instead use built in c functions (if available) and read teh exit codes. If there is an error, we can then report that in the main()'s exit code so that dellEFI does not have to guess as to when the patcher is done.
This may have been fixed with bios A05

## Networking (Realtek1000) kext ##
The Realtek1000 kext does not report the network speed or duplex mode to the os. This should be a somewhat easy fix.

## Shutdown / Restart Hang ##
This is probably due to a bug in one of the drivers / dsdt patch. An easy fix may be modifying the DSDT's KILL method so that it will force a shutdown after an elapsed time (say 5 seconds).
A fix has been found for this, albeit not a very good one.
If you have USB Legacy support ENABLED and Bluetooth DISSABLED in the bios, the restart bug is not there.

## Brightness Control ##
The bios currently handles brightness control. I have found a way to get the dsdt file to pass the keypresses to the os, however OS X doesn't know how to change the brightness.

## Touchpad Scrolling support (In Progress / Working) ##
The current driver uses the touchpad in relative mode. If we put the touchpad in absolute mode we can then allow for side scrolling and some other features (hot corners, etc).

## Startup Chime ##
Just beacause some people have requested it, A kext could be made that loads initialy to play a startup sound (after the audio driver of course).

## Fn + Ctrl key swap ##
On macbooks, the fn key is the leftmost key, where as on the mini, the ctrl key is. It may be possible to swap these keys in the dsdt to have a similar layout as macbook. I have been looking at the code, and have not found an easy way to do this as it looks to be even lower than the dsdt (aka the bios). The dsdt does have a variable that reports the FN button state, however I do not know of anyway to specifically swap it with the ctrl key (but I do have an idea...)

## Temperature reporting ##
The dsdt maps out the embedded controller's memory locations. Some of the variable look to be associated with temperature. It is possible to create a new kext that reads those values from the EC and reports it to the os.
The CTMP variable is read by a method in the thermal zone device, Apple's ACPI driver should automatically read it (if not I may make my own sometime).

## Fn-2: Wireless ##
I believe I have located the method that handles the wireless button and may be able to modify it to enable or disable the wireless hardware. Another alternative is that the OS can handle the keypress (as it is getting one currently) to enable and disable the wireless network.

## Fn-3: Battery ##
This button currently send an unhandeled keypress to the os. Any ideas on waht to use it for?

## Fn-8: Display ##
This button has been disabled due to display corruption when pressed. It can be remanned to something else (pass keycode to os, turn off display, etc) Please post any ideas on what it should do.

## USB disconnected after sleep ##
Currently (at least on my mini) the usb hub / controller is reset after each sleep. This is probably due to the dsdt file cutting power to these devices. It is possible to make it so the usb devices never go to sleep (or only sleep when on battery) so that they are not reset when the computer is woken up. An alternative is to modify the USB kext (say one form superhai) to wait for the device to reappear after sleep.
Some more info:
When usb wake in the bios is set to true, the dsdt's USBW memory location is also set to true. When it is, it tells the os that it cannot change power states. When false, it tells it that it can be turned off. THe change that should be made is that we could make it change from off to a sleep state, hopefully it helps.
  * I was reading the IOUSBFamily source code and located code fix for a usb card attached to and express port. The fix enables makes it so the usb driver does not assume the device was disconnected. If I can apply the fix to all the usb ports on the mini, we should have a (temporary / hackish) solution to the problem.