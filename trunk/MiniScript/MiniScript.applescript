-- MiniScript.applescript
-- MiniScript

--  Created by Type11 on 1/1/09.
--  V7.11 beta 5 by bmaltais on 26/1/09


on awake from nib theObject
	set OSVer to do shell script "sw_vers | grep 'ProductVersion:' |awk '{print $2}'"
	set currdisk to do shell script "df -k / | grep dev | awk -F\" \" '{print $1}' | awk -F\"/\" '{print $3}'"
	set x to the length of the currdisk
	set currdisk to characters 1 thru (x - 2) of currdisk as string
	set contents of text field "booteddisk" of window "Mini9 Installer" to currdisk
	set contents of text field "diskname" of window "Mini9 Installer" to currdisk
	set testdisk to characters 1 thru 5 of currdisk as string
	
	set Type11_exist to do shell script "test -e /.Type11/.donoterase && echo 'file exists' || echo 'no file'"
	
	if Type11_exist is "no file" then
		tell button "eficb" of box "optionspanel" of window "Mini9 Installer"
			set enabled to false
		end tell
		
		tell button "dsdtcb" of box "optionspanel" of window "Mini9 Installer"
			set enabled to false
		end tell
		
		if OSVer is not equal to "10.5.6" then
			tell button "keyboardpanecb" of box "optionspanel" of window "Mini9 Installer"
				set enabled to false
			end tell
		end if
	else
		set dsdt_exists to do shell script "test -e /.Type11/DSDT.aml && echo 'file exists' || echo 'no file'"
		if dsdt_exists is "file exists" then
			tell button "dsdtcb" of box "optionspanel" of window "Mini9 Installer"
				set integer value to 0
			end tell
		else
			tell button "dsdtcb" of box "optionspanel" of window "Mini9 Installer"
				set enabled to false
			end tell
		end if
		
		if OSVer is not equal to "10.5.6" then
			tell button "keyboardpanecb" of box "optionspanel" of window "Mini9 Installer"
				set enabled to false
				set integer value to 0
			end tell
		else
			set prefpane_status to do shell script "test -e /.Type11/Keyboard.prefPane && echo 'file exists' || echo 'no file'"
			if prefpane_status is "file exists" then
				tell button "keyboardpanecb" of box "optionspanel" of window "Mini9 Installer"
					set enabled to false
					set integer value to 0
				end tell
			end if
		end if
		
		set remotecd_exists to do shell script "defaults read com.apple.NetworkBrowser | grep EnableODiskBrowsing; exit 0"
		if remotecd_exists is not "" then
			tell button "enableremotecb" of box "optionspanel" of window "Mini9 Installer"
				set enabled to false
				set integer value to 0
			end tell
		end if
		
		set hibernation_status to do shell script "pmset -g | grep hibernatemode | awk -F\" \" '{print $2}'"
		if hibernation_status is "0" then
			tell button "disablehibernatecb" of box "optionspanel" of window "Mini9 Installer"
				set enabled to false
				set integer value to 0
			end tell
		end if
		
		set scroll_status to do shell script "test -e /.Type11/ApplePS2Controller.kext && echo 'file exists' || echo 'no file'"
		if scroll_status is "file exists" then
			tell button "twofingercb" of box "optionspanel" of window "Mini9 Installer"
				set enabled to false
				set integer value to 0
			end tell
		end if
	end if
end awake from nib

on clicked theObject
	tell progress indicator "progress" of window "Mini9 Installer"
		set uses threaded animation to true
		set visible to true
		start
	end tell
	
	set quietboot to false
	set twofinger to false
	set keyboardpane to false
	set enableremote to false
	set disablehibernate to false
	set dsdt to false
	set efi to false
	
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
	if state of button "dsdtcb" of box "optionspanel" of window "Mini9 Installer" is 1 then
		set dsdt to true
	end if
	if state of button "eficb" of box "optionspanel" of window "Mini9 Installer" is 1 then
		set efi to true
	end if
	
	tell application "Finder" to get folder of (path to me) as Unicode text
	set workingDir to POSIX path of result
	
	if efi is true then
		
		set disk to contents of text field "diskname" of window "Mini9 Installer"
		
		display dialog "Are you sure you want to erase EFI partition of disk " & disk & " and install Type11 files?" buttons ["No", "Yes"]
		if button returned of result is "No" then
			tell progress indicator "progress" of window "Mini9 Installer"
				stop
				set visible to false
			end tell
			return
		end if
		
		do shell script "mkdir /.Type11; touch /.Type11/.donoterase" with administrator privileges
		
		set contents of text field "currentop" of window "Mini9 Installer" to "Erasing EFI volume"
		delay 1
		--this does formatting of the boot loader and EFI partition
		do shell script "diskutil eraseVolume \"HFS+\" \"EFI\" /dev/" & disk & "s1 > /dev/null &" with administrator privileges
		set contents of text field "currentop" of window "Mini9 Installer" to "Copying boot loader"
		delay 1
		do shell script workingDir & "MiniScript.app/Contents/Resources/efi_boot_v6.1/fdisk -f " & workingDir & "MiniScript.app/Contents/Resources/efi_boot_v6.1/boot0 -u -y /dev/r" & disk & " > /dev/null &" with administrator privileges
		do shell script "dd if=" & workingDir & "MiniScript.app/Contents/Resources/efi_boot_v6.1/boot1h of=/dev/r" & disk & "s1 > /dev/null &" with administrator privileges
		
		--mount s1 to EFI mount
		set contents of text field "currentop" of window "Mini9 Installer" to "Mounting EFI volume"
		delay 1
		do shell script "mkdir /Volumes/EFI > /dev/null &" with administrator privileges
		do shell script "mount_hfs /dev/" & disk & "s1 /Volumes/EFI" with administrator privileges
		
		--Put in boot loader, copy update script, and DSDT.aml file
		set contents of text field "currentop" of window "Mini9 Installer" to "Copying boot binary"
		delay 1
		do shell script "cp " & workingDir & "MiniScript.app/Contents/Resources/efi_boot_v6.1/boot-turbo-munky.bin /Volumes/EFI/boot" with administrator privileges
		do shell script "cp " & workingDir & "MiniScript.app/Contents/Resources/efi_boot_v6.1/update.sh /Volumes/EFI/" with administrator privileges
		set contents of text field "currentop" of window "Mini9 Installer" to "Setup directory structure"
		delay 1
		do shell script "mkdir -p /Volumes/EFI/System/Booter > /dev/null &" with administrator privileges
		do shell script "mkdir /Volumes/EFI/Extensions > /dev/null &" with administrator privileges
		do shell script "touch /Volumes/EFI/.fseventsd/no_log" with administrator privileges
		
		--make custom aml file and copy to EFI part root
		if dsdt is false then
			set file_exists_aml to do shell script "test -e /.Type11/DSDT.aml && echo 'file exists' || echo 'no file'" with administrator privileges
			if file_exists_aml is "file exists" then
				set contents of text field "currentop" of window "Mini9 Installer" to "Copy existing DSDT.aml file"
				delay 1
				do shell script "cp /.Type11/DSDT.aml /Volumes/EFI/DSDT.aml > /dev/null 2>&1 &" with administrator privileges
			end if
		else
			set contents of text field "currentop" of window "Mini9 Installer" to "Create new DSDT.aml file"
			delay 1
			do shell script "mkdir /Volumes/EFI/DSDTPatcher" with administrator privileges
			do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/DSDT_Patcher_NO_INPUT/* /Volumes/EFI/DSDTPatcher/" with administrator privileges
			do shell script "cd /Volumes/EFI/DSDTPatcher; ./DSDTPatcher > /dev/null 2>&1 &" with administrator privileges
			delay 3
			do shell script "cp /Volumes/EFI/DSDTPatcher/dsdt.aml /Volumes/EFI/DSDT.aml" with administrator privileges
			do shell script "cp /Volumes/EFI/DSDTPatcher/dsdt.aml /.Type11/DSDT.aml" with administrator privileges
		end if
		
		if twofinger then
			set contents of text field "currentop" of window "Mini9 Installer" to "Installing two finger scrolling"
			delay 1
			do shell script "cp -Rp /Volumes/EFI/Extensions/ApplePS2Controller.kext /.Type11" with administrator privileges
			do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/2FingerScroll/ApplePS2Controller.kext /Volumes/EFI/Extensions" with administrator privileges
			do shell script "chown -R root:wheel /Volumes/EFI/Extensions/ApplePS2Controller.kext" with administrator privileges
			do shell script "mkdir -p /usr/local/bin > /dev/null &" with administrator privileges
			do shell script "cp " & workingDir & "MiniScript.app/Contents/Resources/2FingerScroll/FFScrollDaemon /usr/local/bin" with administrator privileges
			do shell script "chmod 755 /usr/local/bin/FFScrollDaemon" with administrator privileges
			do shell script "chown root:wheel /usr/local/bin/FFScrollDaemon" with administrator privileges
			do shell script "cp " & workingDir & "MiniScript.app/Contents/Resources/2FingerScroll/start_FFScrollDaemon /usr/local/bin" with administrator privileges
			do shell script "chmod 755 /usr/local/bin/start_FFScrollDaemon" with administrator privileges
			do shell script "chown root:wheel /usr/local/bin/start_FFScrollDaemon" with administrator privileges
			do shell script "cp " & workingDir & "MiniScript.app/Contents/Resources/2FingerScroll/com.apple.driver.ApplePS2Trackpad.plist /Library/Preferences/" with administrator privileges
			do shell script "chmod 644 /Library/Preferences/com.apple.driver.ApplePS2Trackpad.plist" with administrator privileges
			do shell script "chown root:admin /Library/Preferences/com.apple.driver.ApplePS2Trackpad.plist" with administrator privileges
			do shell script "cp " & workingDir & "MiniScript.app/Contents/Resources/2FingerScroll/com.apple.FFScrollDaemon.plist /Library/LaunchAgents/" with administrator privileges
			do shell script "chmod 644 /Library/LaunchAgents/com.apple.FFScrollDaemon.plist" with administrator privileges
			do shell script "chown root:wheel /Library/LaunchAgents/com.apple.FFScrollDaemon.plist" with administrator privileges
		end if
		
		-- move all extensions over to EFI ext dir.
		set contents of text field "currentop" of window "Mini9 Installer" to "Copy kexts to EFI partition"
		delay 1
		do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/Extensions/*.kext /Volumes/EFI/Extensions" with administrator privileges
		if quietboot then
			set contents of text field "currentop" of window "Mini9 Installer" to "Installing com.apple.Boot"
			delay 1
			do shell script "sed -e 's/disk0/" & disk & "/g' " & workingDir & "MiniScript.app/Contents/Resources/Boot/com.apple.Boot.Quiet.plist >  /Volumes/EFI/com.apple.Boot.plist" with administrator privileges
		else
			set contents of text field "currentop" of window "Mini9 Installer" to "Installing com.apple.Boot"
			delay 1
			do shell script "sed -e 's/disk0/" & disk & "/g' " & workingDir & "MiniScript.app/Contents/Resources/Boot/com.apple.Boot.plist >  /Volumes/EFI/com.apple.Boot.plist" with administrator privileges
		end if
		
		set contents of text field "currentop" of window "Mini9 Installer" to "Update EFI kext cache"
		delay 1
		--make sure permissions on update.sh are right and run it to make EFI kext cache
		do shell script "chmod +x /Volumes/EFI/update.sh" with administrator privileges
		do shell script "sudo /Volumes/EFI/update.sh" with administrator privileges
		--unmount the drive and delete mount folder
		set contents of text field "currentop" of window "Mini9 Installer" to "Unmount EFI partition"
		delay 1
		do shell script "umount -f /Volumes/EFI" with administrator privileges
		do shell script "rm -rf /Volumes/EFI" with administrator privileges
		--clean up old audio installs
		set contents of text field "currentop" of window "Mini9 Installer" to "Remove old audio files"
		delay 1
		do shell script "rm -r /System/Library/Extensions/ALCinject.kext > /dev/null &" with administrator privileges
		do shell script "rm -r /System/Library/Extensions/HDAEnabler.kext > /dev/null &" with administrator privileges
		
		set contents of text field "currentop" of window "Mini9 Installer" to "Installing local files"
		delay 1
		-- move items that need to be local for audio and battery, hopefully this goes away someday
		do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/LocalExtensions/*.kext /System/Library/Extensions > /dev/null &" with administrator privileges
		do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/SystemConfiguration/*.bundle /System/Library/SystemConfiguration > /dev/null &" with administrator privileges
		-- remove mkext so it is rebuilt
		do shell script "rm -r /System/Library/Extensions.mkext > /dev/null &" with administrator privileges
		
	end if
	
	--install old keyboard control panel as 10.5.6 does not see our trackpad
	if keyboardpane then
		set contents of text field "currentop" of window "Mini9 Installer" to "Installing 10.5.5 keyboard kext"
		delay 1
		-- do shell script "mkdir /Backup > /dev/null &" with administrator privileges
		do shell script "mv /System/Library/PreferencePanes/Keyboard.prefPane /.Type11 > /dev/null &" with administrator privileges
		do shell script "cp -R " & workingDir & "MiniScript.app/Contents/Resources/PrefPanes/Keyboard.prefPane /System/Library/PreferencePanes/ > /dev/null &" with administrator privileges
		do shell script "chown -R root:wheel /System/Library/PreferencePanes/Keyboard.prefPane > /dev/null &" with administrator privileges
	end if
	
	if enableremote then
		--enable remote cd
		set contents of text field "currentop" of window "Mini9 Installer" to "Enabling remote CD"
		delay 1
		do shell script "defaults write com.apple.NetworkBrowser EnableODiskBrowsing -bool true"
		do shell script "defaults write com.apple.NetworkBrowser ODSSupported -bool true"
	end if
	
	if disablehibernate then
		set contents of text field "currentop" of window "Mini9 Installer" to "Disabling hibernate"
		delay 1
		do shell script "pmset hibernatemode 0 > /dev/null &" with administrator privileges
		do shell script "rm /var/vm/sleepimage > /dev/null &" with administrator privileges
	end if
	-- delay 3
	
	--reboot
	tell progress indicator "progress" of window "Mini9 Installer"
		stop
		set visible to false
	end tell
	
	set contents of text field "currentop" of window "Mini9 Installer" to "Finished"
	display dialog "All done, ready for reboot" buttons ["No", "Yes"]
	if button returned of result is "No" then
		quit
	else
		tell application "System Events"
			restart
		end tell
	end if
end clicked
