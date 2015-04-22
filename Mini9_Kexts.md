# Dell Mini 9 Custom Kexts and their uses #

| **Kext** | **Use** | **Reason for inclusion** | **Future Plans** |
|:---------|:--------|:-------------------------|:-----------------|
| AppleACPIPS2Nub.kext | Provides an interface to the PS2Controller kext to enable and dissable interrupts | <font color='red'>Required</font>**for PS/2 Support (enables / disables ps2 interrupts)**| New version from meklort in v1.1 |
| AppleDecrypt.kext | Enables use of apple encrypted kexts | <font color='red'>Required</font>**so that apple encrypted kexts will load (example: enables GUI)**|  |
| AppleIntelGMA950.kext | Driver for the GMA950 display | Only change is that it has been patched (read: hex edited) so that it recognizes 0x27AE as a supported device | This is created on the fly in v1.1 |
| AppleIntelIntegratedFramebuffer.kext | framebuffer stuff for the intel cards | Only change is that it has been patched (read: hex edited) so that it recognizes 0x27AE as a supported device | This is created on the fly in v1.1 |
| AppleIntelPIIXATA.kext | PATA driver | This driver allows sleep / resume to work correctly on STEC SSD |  |
| ApplePS2Controller.kext | Driver for PS2 Devices, includes keyboard and trackpad kexts | <font color='red'>Required</font>**for PS/2 support (Apple does not include ps2 drivers in os x)**| New version from meklort in v1.1. A new touchpad driver is in in development which include two finger and side scrolling support |
| Realtek1000.kext  | Enables network card |  | Update kext to notify OS of duplex mode and network speed |
| IO80211Family.kext | Enable WiFi |  | This is created on the fly in v1.1+ |
| IOGraphicsFamily.kext |  | Driver needed so that IONDRVSupport will load | This is copied directly from the os without patching |
| IONDRVSupport.kext | Framebuffer stuff... |  | This is copied directly from the os without patching |
| IONetworkingFamily.kext |  | Needed to load Ethernet card kext | This is copied directly from the os without patching |
| IOSDHCIBlockDevice.kext | Jmicron SDHC driver | Mini 9 includes a card reader, this driver enables it |  |
| SMBIOSResolver.kext |  |  |  |

# Dell Mini 9 Custom LocalExtensions kexts and their uses #
Quick qestions... Why are these not in the /Extra/Extensions1 folder?
| AppleHDA.kext | HD Audio kext | | Why is this a "LocalExtensions" and not installed in the Extra/ directory? Currently won't load if in /Extra... maybe could be fixed with custom compile of code?  There is apparently a way to patch the source to make it load from /Extra. It may be possible to include this in /Extra if we include a DSP (I think) kext, there is instructions with the VoodooHDA kext on how to do this. |
|:--------------|:--------------|:|:------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| ClamshellDisplay.kext | Enables sleep when display is closed | Add glue so that sleep works properly | A dsdt patch is available and included with DellEFI to negate the need for this, however it is not enabled by defualt |
| IOAudioFamily.kext | Provides an interface for audio drivers | The kext has been patched so that audio works on resume. | Why is this a "LocalExtensions" and not installed in the Extra/ director? |