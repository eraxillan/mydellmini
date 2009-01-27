-- MiniScript.applescript
-- MiniScript

--  Created by Type11 on 1/1/09.


on awake from nib theObject
	set image of image view "logo" of window "Mini9 Installer" to load image "DellMini9"
	set OSVer to do shell script "sw_vers | grep 'ProductVersion:' |awk '{print $2}'" with administrator privileges
	if OSVer is not equal to "10.5.6" then
		tell button "keyboardpanecb" of box "optionspanel" of window "Mini9 Installer"
			set enabled to false
		end tell
	end if
	set currdisk to do shell script "diskutil info / | awk '/Identifier/ {print $3}'" with administrator privileges
	set x to the length of the currdisk
	set currdisk to characters 1 thru (x - 2) of currdisk as string
	set contents of text field "booteddisk" of window "Mini9 Installer" to currdisk
	set contents of text field "diskname" of window "Mini9 Installer" to currdisk
	set testdisk to characters 1 thru 5 of currdisk as string
	--try and mount EFI part if this script has never been run will fail but quietly which is fine.  only reason we are doing this here is top look for existing dsdt.aml file and ask if they want it repalced as it needs a restart
	(*
	do shell script "mkdir /Volumes/EFI > /dev/null &" with administrator privileges
	do shell script "mount_hfs /dev/" & testdisk & "s1 /Volumes/EFI > /dev/null 2>&1 &" with administrator privileges
	set file_exists to do shell script "test -e /Volumes/EFI/DSDT.aml && echo 'file exists' || echo 'no file'" with administrator privileges
	if file_exists is "file exists" then
		display dialog "You currently have a DSDT.aml file on EFI partition.  Do you want to maintain this after install or create a new one?  This requires a reboot to create a new one and will happen immediately if you hit New.  After reboot re-run MiniSctipt" buttons ["Keep", "New", "Quit"] default button "New"
		if button returned of result is "Keep" then
			do shell script "mkdir /tempaml; cp /Volumes/EFI/DSDT.aml /tempaml > /dev/null &" with administrator privileges
		else if button returned of result is "New" then
			do shell script "rm /Volumes/EFI/DSDT.aml > /dev/null &" with administrator privileges
			do shell script "umount -f /Volumes/EFI > /dev/null &" with administrator privileges
			tell application "System Events"
				restart
				quit
			end tell
		else if button returned of result is "Quit" then
			do shell script "umount -f /Volumes/EFI > /dev/null &" with administrator privileges
			quit
		end if
	end if
	--umount for now (if mounted) DSDT check done
	do shell script "umount -f /Volumes/EFI > /dev/null &" with administrator privileges
	*)
end awake from nib

on clicked theObject
	tell progress indicator "progress" of window "Mini9 Installer"
		set visible to true
		start
	end tell
	set quietboot to false
	set twofinger to false
	set keyboardpane to false
	set enableremote to false
	set disablehibernate to false
	set removereggie to false
	tell application "Finder" to get folder of (path to me) as Unicode text
	set workingDir to POSIX path of result
	set disk to contents of text field "diskname" of window "Mini9 Installer"
	if state of button "quietbootcb" of box "optionspanel" of window "Mini9 Installer" is 1 then
		set quietboot to true
	end if
	if state of button "twofingercb" of box "optionspanel" of window "Mini9 Installer" is 1 then
		set twofinger to true
	end if
	if state of button "keyboardpanecb" of box "optionspanel" of window "Mini9 Installer" is 1 then
		set keyboardpane to true
	end if
	if state of button "enableremotecb" of box "optionspanel" of window "Mini9 Installer" is 1 then
		set enableremote to true
	end if
	if state of button "disablehibernatecb" of box "optionspanel" of window "Mini9 Installer" is 1 then
		set disablehibernate to true
	end if
	if state of button "removereggiecb" of box "optionspanel" of window "Mini9 Installer" is 1 then
		set removereggie to true
	end if
	set contents of text field "currentop" of window "Mini9 Installer" to "erasing EFI volume"
	--this does formatting of the boot loader and EFU partition
	do shell script "diskutil eraseVolume \"HFS+\" \"EFI\" /dev/" & disk & "s1 > /dev/null &" with administrator privileges
	set contents of text field "currentop" of window "Mini9 Installer" to "copying boot loader"
	do shell script workingDir & "MiniScript.app/Contents/Resources/efi_boot_v6.1/fdisk -f " & workingDir & "MiniScript.app/Contents/Resources/efi_boot_v6.1/boot0 -u -y /dev/r" & disk & " > /dev/null &" with administrator privileges
	do shell script "dd if=" & workingDir & "MiniScript.app/Contents/Resources/efi_boot_v6.1/boot1h of=/dev/r" & disk & "s1 > /dev/null &" with administrator privileges
	
	--mount s1 to EFI mount
	set contents of text field "currentop" of window "Mini9 Installer" to "mounting EFI volume"
	do shell script "mkdir /Volumes/EFI > /dev/null &" with administrator privileges
	do shell script "mount_hfs /dev/" & disk & "s1 /Volumes/EFI" with administrator privileges
	
	--If it is a new install then put in boot laoder, copy update script, and DSDT.aml file
	set contents of text field "currentop" of window "Mini9 Installer" to "copying boot bin"
	do shell script "cp " & workingDir & "MiniScript.app/Contents/Resources/efi_boot_v6.1/boot-turbo-munky.bin /Volumes/EFI/boot" with administrator privileges
	do shell script "cp " & workingDir & "MiniScript.app/Contents/Resources/efi_boot_v6.1/update.sh /Volumes/EFI/" with administrator privileges
	set contents of text field "currentop" of window "Mini9 Installer" to "setup dir structure"
	do shell script "mkdir -p /Volumes/EFI/System/Booter > /dev/null &" with administrator privileges
	do shell script "mkdir /Volumes/EFI/Extensions > /dev/null &" with administrator privileges
	do shell script "touch /Volumes/EFI/.fseventsd/no_log" with administrator privileges
	--make custom aml file and copy to EFI part root
	set contents of text field "currentop" of window "Mini9 Installer" to "create or copy DSDT.aml file"
	(*
	set file_exists_aml to do shell script "test -e /tempaml/DSDT.aml && echo 'file exists' || echo 'no file'" with administrator privileges
	if file_exists_aml is "file exists" then
		do shell script "cp /tempaml/DSDT.aml /Volumes/EFI/DSDT.aml; rm -r /tempaml > /dev/null &" with administrator privileges
	else
	*)
	do shell script "mkdir /Volumes/EFI/DSDTPatcher" with administrator privileges
	do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/DSDT_Patcher_NO_INPUT/* /Volumes/EFI/DSDTPatcher/" with administrator privileges
	do shell script "cd /Volumes/EFI/DSDTPatcher; ./DSDTPatcher > /dev/null &" with administrator privileges
	delay 3
	do shell script "cp /Volumes/EFI/DSDTPatcher/dsdt.aml /Volumes/EFI/DSDT.aml" with administrator privileges
	(*
	end if
	*)
	-- move all extensions over to EFI ext dir.
	set contents of text field "currentop" of window "Mini9 Installer" to "copy kexts to EFI partition"
	do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/Extensions/*.kext /Volumes/EFI/Extensions" with administrator privileges
	delay 3
	if quietboot then
		set contents of text field "currentop" of window "Mini9 Installer" to "installing com.apple.Boot"
		do shell script "sed -e 's/disk0/" & disk & "/g' " & workingDir & "MiniScript.app/Contents/Resources/Boot/com.apple.Boot.Quiet.plist >  /Volumes/EFI/com.apple.Boot.plist" with administrator privileges
	else
		set contents of text field "currentop" of window "Mini9 Installer" to "installing com.apple.Boot"
		do shell script "sed -e 's/disk0/" & disk & "/g' " & workingDir & "MiniScript.app/Contents/Resources/Boot/com.apple.Boot.plist >  /Volumes/EFI/com.apple.Boot.plist" with administrator privileges
	end if
	
	if twofinger then
		set contents of text field "currentop" of window "Mini9 Installer" to "installing two finger scrolling"
		do shell script "mkdir /Backup > /dev/null &"
		do shell script "cp -Rp /Volumes/EFI/Extensions/ApplePS2Controller.kext /Backup" with administrator privileges
		do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/2FingerScroll/ApplePS2Controller.kext /Volumes/EFI/Extensions;chown -R root:wheel /Volumes/EFI/Extensions/ApplePS2Controller.kext" with administrator privileges
		do shell script "mkdir -p /usr/local/bin > /dev/null &; cp " & workingDir & "MiniScript.app/Contents/Resources/2FingerScroll/FFScrollDaemon /usr/local/bin;chmod 755 /usr/local/bin/FFScrollDaemon;chown root:wheel /usr/local/bin/FFScrollDaemon" with administrator privileges
		do shell script "cp " & workingDir & "MiniScript.app/Contents/Resources/2FingerScroll/start_FFScrollDaemon /usr/local/bin;chmod 755 /usr/local/bin/start_FFScrollDaemon;chown root:wheel /usr/local/bin/start_FFScrollDaemon" with administrator privileges
		do shell script "cp " & workingDir & "MiniScript.app/Contents/Resources/2FingerScroll/com.apple.driver.ApplePS2Trackpad.plist /Library/Preferences/;chmod 644 /Library/Preferences/com.apple.driver.ApplePS2Trackpad.plist;chown root:admin /Library/Preferences/com.apple.driver.ApplePS2Trackpad.plist" with administrator privileges
		do shell script "cp " & workingDir & "MiniScript.app/Contents/Resources/2FingerScroll/com.apple.FFScrollDaemon.plist /Library/LaunchAgents/;chmod 644 /Library/LaunchAgents/com.apple.FFScrollDaemon.plist;chown root:wheel /Library/LaunchAgents/com.apple.FFScrollDaemon.plist" with administrator privileges
	end if
	
	--check and see if we are at 10.5.6 and if so offer to install old keyboard control panel as 10.5.6 does not see our trackpad
	if keyboardpane then
		set contents of text field "currentop" of window "Mini9 Installer" to "installing 10.5.5 keyboard kext"
		do shell script "mkdir /Backup > /dev/null &" with administrator privileges
		do shell script "mv /System/Library/PreferencePanes/Keyboard.prefPane /Backup > /dev/null &" with administrator privileges
		do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/PrefPanes/Keyboard.prefPane /System/Library/PreferencePanes/ > /dev/null &" with administrator privileges
		do shell script "chown -R root:wheel /System/Library/PreferencePanes/Keyboard.prefPane > /dev/null &" with administrator privileges
	end if
	set contents of text field "currentop" of window "Mini9 Installer" to "update EFI kext cache"
	--make sure permissions on update.sh are right and run it to make EFI kext cache
	do shell script "chmod +x /Volumes/EFI/update.sh" with administrator privileges
	do shell script "sudo /Volumes/EFI/update.sh" with administrator privileges
	--unmount the drive and delete mount folder
	set contents of text field "currentop" of window "Mini9 Installer" to "unmount EFI part"
	do shell script "umount -f /Volumes/EFI" with administrator privileges
	do shell script "rm -rf /Volumes/EFI" with administrator privileges
	--clean up old audio installs
	set contents of text field "currentop" of window "Mini9 Installer" to "remove old audio files"
	do shell script "rm -r /System/Library/Extensions/ALCinject.kext > /dev/null &" with administrator privileges
	do shell script "rm -r /System/Library/Extensions/HDAEnabler.kext > /dev/null &" with administrator privileges
	-- remove mkext so it is rebuilt
	set contents of text field "currentop" of window "Mini9 Installer" to "installing local files"
	do shell script "rm -r /System/Library/Extensions.mkext > /dev/null &" with administrator privileges
	-- move items that need to be local for audio and battery, hopefully this goes away someday
	do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/LocalExtensions/*.kext /System/Library/Extensions > /dev/null &" with administrator privileges
	do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/SystemConfiguration/*.bundle /System/Library/SystemConfiguration > /dev/null &" with administrator privileges
	
	-- setup sleepwatcher to make audio wake after sleep
	if removereggie then
		set contents of text field "currentop" of window "Mini9 Installer" to "removing reggie_se and sleepwatcher"
		do shell script "rm -r /System/Library/PrivateFrameworks/ARMDisassembler.framework > /dev/null &" with administrator privileges
		do shell script "rm -r /System/Library/PrivateFrameworks/CHUD.framework > /dev/null &" with administrator privileges
		do shell script "rm -r /System/Library/PrivateFrameworks/diStorm.framework > /dev/null &" with administrator privileges
		do shell script "rm -r /System/Library/PrivateFrameworks/PPCDisasm.framework > /dev/null &" with administrator privileges
		do shell script "rm -r /usr/bin/reggie_se > /dev/null &" with administrator privileges
		do shell script "rm -r /private/etc/rc.sleep > /dev/null &" with administrator privileges
		do shell script "rm -r /private/etc/rc.wakeup > /dev/null &" with administrator privileges
		do shell script "rm -r /Library/StartupItems/SleepWatcher > /dev/null &" with administrator privileges
		do shell script "rm  -r /usr/local/sbin/sleepwatcher > /dev/null &" with administrator privileges
		do shell script "rm  -r /System/Library/Extensions/CHUD* > /dev/null &" with administrator privileges
	end if
	
	if enableremote then
		--enable remote cd
		set contents of text field "currentop" of window "Mini9 Installer" to "enabling remote CD"
		do shell script "defaults write com.apple.NetworkBrowser EnableODiskBrowsing -bool true"
		do shell script "defaults write com.apple.NetworkBrowser ODSSupported -bool true"
	end if
	if disablehibernate then
		set contents of text field "currentop" of window "Mini9 Installer" to "disabling hibernate"
		do shell script "pmset hibernatemode 0 > /dev/null &" with administrator privileges
		do shell script "rm /var/vm/sleepimage > /dev/null &" with administrator privileges
	end if
	delay 3
	--reboot
	tell progress indicator "progress" of window "Mini9 Installer"
		stop
		set visible to false
	end tell
	set contents of text field "currentop" of window "Mini9 Installer" to "Finished"
	display dialog "Ready for reboot" buttons ["Nope", "Yep"]
	if button returned of result is "Nope" then
		quit
	else
		tell application "System Events"
			restart
		end tell
	end if
	
end clicked