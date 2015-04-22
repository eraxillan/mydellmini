# DellEFI 1.2 - Subject to change, currently in Alpha #
  * Updated bootloader from PC EFI V9 to Chameleon 2.0 RC1.
  * **PLANNED** Includes new touchpad driver with two finger scrolling support.
  * **PLANNED** Patch BroadcomUSBBluetoothHCIController to recognize our module as an Apple suppled / internal module (thanks to **tetany** for the fix).
  * **PLANNED** Update IOAudioFamily so that it is the same as the 10.5.7 version


# DellEFI 1.1 #
  * Updated AppleACPIBatteryManager which reads battery information correctly.
  * Updated DSDTPatcher, fixes dsdt to report battery charge / discharge rate. Also reports cycle count, manufacturer and more information.
  * Autopatching of Apple's GMA950, Framebuffer, and IO80211Familly kexts
  * Copying of Apple's IOGraphicsFamily, IONDRVSupport, and IONEtworkingFamily kexts
  * Added EFI string to remove the need for the HDAEnabler and Natit kexts.
  * Replace patched AppleRTL8196 driver with modified Realtek1000 driver, impreved throughput and better support for the 8139 based card.
  * Updated mirroring mode drivers to only replace the Framebuffer kext
  * Updated ApplePS2Controler to include the new ApplePS2Keyboard kext instead of installing separately.
  * Updated IntelPIIXATA kext (recompiled with optimizations, latest codebase from apple)
  * Moved /Extra/Extensions1 to /Extra/Mini9Ext/
  * Logic changes to force dsdt regeneration and efi string placement

# DellEFI 1.07.1 #
  * Added UpdateExtensions.app to /Extra so that you may run it instead of the terminal's kextcache
  * New ApplePS2Keyboard.kext that fixes the stuck key on wake up bug. Also includes new keymap so that the menu button maps as the command key. **Thanks go to kenp for the fix.**
  * New SDHC driver which uses interrupts to increase speed and decrease cpu usage. **Thanks to km9 for writing the kext.**

# DellEFI 1.06 #
  * New GMA 950 video drivers based on 10.5.6 kernel Extensions
  * Owner and File Access Rights properly set for new LocalExtensions
  * Verification of currently installed Extensions to properly flag required installation from release to r

# DellEFI 1.05 #
  * New 10.5.6 GMA 950 kext and 10.5.6 FrameBuffer kext. with no artifacts

# DellEFI 1.04 #
  * Ability to delete dsdt.aml if you suspect it is corrupted
  * Changed url of dellefiappcast.xml because google code won't let mu update it anymore ;-(

# DellEFI 1.03 #
  * You can now rename DellEFI to whatever your heart tell you
  * Added a "Fix bluetooth" option to resolve no bluetooth on boot. **Thanks to Zeke for the tip.**
  * Added back 2 finger scroll option for those who really want it
  * some code cleanup

# DellEFI 1.02 #

# DellEFI 1.01 #
  * Added the Sparkle framework so when I release a new version you will be warned by the application itself
  * Added various error handling dialogs to help troubleshoot AppleScript errors.

# DellEFI 1.0 #
  * Initial release