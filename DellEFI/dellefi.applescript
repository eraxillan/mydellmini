-- dellefi.applescript
-- dellefi

--  Created by Bernard Maltais on 28/01/09.

property QuietBootP : false
property HideEFIP : false
property RemoteCDP : false
property HibernationP : false

on awake from nib theObject
	
	tell every window to center
	
	tell window "Initializing"
		tell progress indicator "spinner"
			set uses threaded animation to true
			start
		end tell
		set visible to true
	end tell
	
	try
		set OSVer to do shell script "sw_vers | grep 'ProductVersion:' | awk '{print $2}'"
		
		if OSVer is not equal to "10.5.6" then
			display dialog "DellEFI is only meant for 10.5.6 system. Please upgrade to 10.5.6 before running again." buttons ["Quit"] default button "Quit" with icon caution
		end if
	on error errMsg number errorNumber
		display dialog "Could not detect OSX version. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
		quit
	end try
	
	try
		-- check if we are running on a Dell Mini 9... we check the mac address of the nic to be one of Dell's
		set disktype to do shell script "ifconfig en0 | grep ether | awk '{print $2}'"
		set disktype to characters 1 thru 8 of disktype as string
		
		if disktype is not equal to "00:21:70" then
			display dialog "It does not appear that you are running this application on a Dell Mini 9?  Are you sure you want to continue?" buttons ["No", "Yes"] default button "No" with icon caution
			if button returned of result is "No" then
				quit
			end if
		end if
	on error errMsg number errorNumber
		display dialog "Could not detect MAC address. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
		quit
	end try
	
	set prefpane_status to do shell script "test -e /.dellefi/Keyboard.prefPane && echo 'file exists' || echo 'no file'"
	if prefpane_status is "file exists" then
		tell button "keyboardpanecb" of box "optionspanel" of window "DellEFI Installer"
			-- set enabled to false
			set integer value to 0
		end tell
	end if
	
	set dsdt_exists to do shell script "test -e /dsdt.aml && echo 'file exists' || echo 'no file'"
	if dsdt_exists is "file exists" then
		tell button "dsdtcb" of box "optionspanel" of window "DellEFI Installer"
			set integer value to 0
			-- set enabled to false
		end tell
	end if
	
	set extexist to do shell script "test -e /Extra/Extensions.mkext && echo 'file exists' || echo 'no file'"
	if extexist is "file exists" then
		tell button "extensionscb" of box "optionspanel" of window "DellEFI Installer"
			set integer value to 0
			set title to "Reinstall Dell Mini 9 Extensions"
			-- set enabled to false
		end tell
	end if
	
	set bootexist to do shell script "test -e /boot && echo 'file exists' || echo 'no file'"
	if bootexist is "file exists" then
		tell button "eficb" of box "optionspanel" of window "DellEFI Installer"
			set integer value to 0
			set title to "Reinstall PCEFIV9 Bootloader"
			-- set enabled to false
		end tell
	end if
	
	set remotecd_exists to do shell script "defaults read com.apple.NetworkBrowser | grep EnableODiskBrowsing; exit 0"
	if remotecd_exists is not "" then
		set RemoteCDP to true
		tell button "enableremotecb" of box "optionspanel" of window "DellEFI Installer"
			-- set enabled to false
			set title to "Disable Remote CD"
			set integer value to 0
		end tell
	end if
	
	set quietstate to do shell script "grep Quiet /Library/Preferences/SystemConfiguration/com.apple.Boot.plist; exit 0"
	if quietstate is not "" then
		set QuietBootP to true
		tell button "quietbootcb" of box "optionspanel" of window "DellEFI Installer"
			set title to "Disable Quiet Boot"
		end tell
	end if
	
	set hideefistate to do shell script "ls -l / | grep dsdt.aml | grep @; exit 0"
	if hideefistate is not "" then
		set HideEFIP to true
		tell button "hidecb" of box "optionspanel" of window "DellEFI Installer"
			set title to "Show DellEFI files"
			set integer value to 0
		end tell
	end if
	
	set hibernation_status to do shell script "pmset -g | grep hibernatemode | awk -F\" \" '{print $2}'"
	if hibernation_status is "0" then
		set HibernationP to true
		tell button "disablehibernatecb" of box "optionspanel" of window "DellEFI Installer"
			-- set enabled to false
			set title to "Enable Hibernate (Not Recommended)"
			set integer value to 0
		end tell
	end if
	
	tell matrix 1 of window "DellEFI Installer"
		set current row to 1
	end tell
	
	tell window "Initializing"
		set visible to false
		tell progress indicator "spinner"
			stop
		end tell
	end tell
	
	tell window "DellEFI Installer"
		set visible to true
	end tell
end awake from nib

on clicked theObject
	tell progress indicator "progress" of window "DellEFI Installer"
		set uses threaded animation to true
		set visible to true
		start
	end tell
	
	set keyboardpane to false
	set enableremote to false
	set disablehibernate to false
	set dsdt to false
	set efi to false
	set hideefi to false
	set extensionsfiles to false
	set quietboot to false
	set needreboot to false
	
	if state of button "keyboardpanecb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set keyboardpane to true
	end if
	if state of button "enableremotecb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set enableremote to true
	end if
	if state of button "disablehibernatecb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set disablehibernate to true
	end if
	if state of button "dsdtcb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set dsdt to true
	end if
	if state of button "eficb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set efi to true
	end if
	if state of button "hidecb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set hideefi to true
	end if
	if state of button "extensionscb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set extensionsfiles to true
	end if
	if state of button "quietbootcb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set quietboot to true
	end if
	
	tell application "Finder" to get folder of (path to me) as Unicode text
	set workingDir to POSIX path of result
	
	set x to the length of the workingDir
	set workingDir to characters 1 thru (x - 1) of workingDir as string
	set workingDir to "\"" & workingDir & "\"/"
	
	set disk to do shell script "df -k / | grep dev | awk -F\" \" '{print $1}' | awk -F\"/\" '{print $3}'"
	set x to the length of the disk
	set disk to characters 1 thru (x - 2) of disk as string
	
	try
		if efi is true then
			-- set currdisk to do shell script "ls -l /Volumes/ | grep \" /\" | grep root | awk '{print $9}'"
			
			set contents of text field "currentop" of window "DellEFI Installer" to "Installing bootloader"
			delay 1
			do shell script workingDir & "DellEFI.app/Contents/Resources/bootpcefiv9/fdisk -f " & workingDir & "DellEFI.app/Contents/Resources/bootpcefiv9/boot0 -u -y /dev/r" & disk & " > /dev/null &" with administrator privileges
			do shell script "dd if=" & workingDir & "DellEFI.app/Contents/Resources/bootpcefiv9/boot1h of=/dev/r" & disk & "s2 > /dev/null &" with administrator privileges
			do shell script "cp " & workingDir & "DellEFI.app/Contents/Resources/bootpcefiv9/boot /boot > /dev/null &" with administrator privileges
			
			set contents of text field "currentop" of window "DellEFI Installer" to "Installing com.apple.Boot"
			delay 1
			do shell script "cp " & workingDir & "DellEFI.app/Contents/Resources/Boot/com.apple.Boot.plist  /Library/Preferences/SystemConfiguration/com.apple.Boot.plist" with administrator privileges
		end if
	on error errMsg number errorNumber
		display dialog "Could not install the Bootloader. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
		quit
	end try
	
	try
		if quietboot then
			if QuietBootP then
				set contents of text field "currentop" of window "DellEFI Installer" to "Disabling Quiet Boot"
				delay 1
				do shell script "cp " & workingDir & "DellEFI.app/Contents/Resources/Boot/com.apple.Boot.plist  /Library/Preferences/SystemConfiguration/com.apple.Boot.plist" with administrator privileges
			else
				set contents of text field "currentop" of window "DellEFI Installer" to "Configuring Quiet Boot"
				delay 1
				do shell script "sed -e 's/disk0/" & disk & "/g' " & workingDir & "DellEFI.app/Contents/Resources/Boot/com.apple.Boot.Quiet.plist >  /Library/Preferences/SystemConfiguration/com.apple.Boot.plist" with administrator privileges
			end if
		end if
	on error errMsg number errorNumber
		display dialog "Could not disable quiet boot. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
		quit
	end try
	
	try
		if extensionsfiles is true then
			try
				do shell script "mkdir /.dellefi; touch /.dellefi/.donoterase" with administrator privileges
			end try
			
			-- move all extensions over to EFI ext dir.
			set contents of text field "currentop" of window "DellEFI Installer" to "Copy kexts to Extra folder"
			delay 1
			try
				do shell script "rf -rf /Extra.bak" with administrator privileges
			end try
			try
				do shell script "mv /Extra /Extra.bak" with administrator privileges
			end try
			try
				do shell script "mkdir /Extra > /dev/null &" with administrator privileges
				do shell script "mkdir /Extra/Extensions1" with administrator privileges
			end try
			do shell script "cp -R " & workingDir & "DellEFI.app/Contents/Resources/Extensions/*.kext /Extra/Extensions1" with administrator privileges
			do shell script "chown -R 0:0 /Extra/" with administrator privileges
			do shell script "chmod -R 755 /Extra/" with administrator privileges
			set contents of text field "currentop" of window "DellEFI Installer" to "Update EFI kext cache"
			delay 1
			do shell script "kextcache -a i386 -m /Extra/Extensions.mkext /Extra/Extensions1" with administrator privileges
			
			--clean up old audio installs
			set contents of text field "currentop" of window "DellEFI Installer" to "Remove old audio files"
			delay 1
			do shell script "rm -r /System/Library/Extensions/ALCinject.kext > /dev/null &" with administrator privileges
			do shell script "rm -r /System/Library/Extensions/HDAEnabler.kext > /dev/null &" with administrator privileges
			
			set contents of text field "currentop" of window "DellEFI Installer" to "Installing local files"
			delay 1
			-- move items that need to be local for audio and battery, hopefully this goes away someday
			do shell script "cp -R " & workingDir & "DellEFI.app/Contents/Resources/LocalExtensions/*.kext /System/Library/Extensions > /dev/null &" with administrator privileges
			do shell script "cp -R " & workingDir & "DellEFI.app/Contents/Resources/SystemConfiguration/*.bundle /System/Library/SystemConfiguration > /dev/null &" with administrator privileges
			-- remove mkext so it is rebuilt
			do shell script "rm -r /System/Library/Extensions.mkext > /dev/null &" with administrator privileges
			
			set needreboot to true
		end if
	on error errMsg number errorNumber
		display dialog "Could not install Extensions. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
		quit
	end try
	
	try
		--make custom aml file and copy to EFI part root
		if dsdt is true then
			set contents of text field "currentop" of window "DellEFI Installer" to "Creating dsdt.aml file"
			delay 1
			try
				do shell script "mkdir /.dellefi; touch /.dellefi/.donoterase" with administrator privileges
			end try
			
			try
				do shell script "cp -Rf " & workingDir & "DellEFI.app/Contents/Resources/DSDTPatcher /.dellefi/" with administrator privileges
			end try
			do shell script "cd /.dellefi/DSDTPatcher; ./DSDTPatcher > /dev/null 2>&1 &" with administrator privileges
			delay 6
			
			do shell script "cp /.dellefi/DSDTPatcher/dsdt.aml /dsdt.aml" with administrator privileges
			
			set needreboot to true
		end if
	on error errMsg number errorNumber
		display dialog "Could not create dsdt.aml file. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
		quit
	end try
	
	try
		--install old keyboard control panel as 10.5.6 does not see our trackpad
		if keyboardpane then
			set contents of text field "currentop" of window "DellEFI Installer" to "Installing 10.5.5 keyboard kext"
			delay 1
			try
				do shell script "mkdir /.dellefi; touch /.dellefi/.donoterase" with administrator privileges
			end try
			
			-- do shell script "mkdir /Backup > /dev/null &" with administrator privileges
			do shell script "mv /System/Library/PreferencePanes/Keyboard.prefPane /.dellefi > /dev/null &" with administrator privileges
			do shell script "cp -R " & workingDir & "DellEFI.app/Contents/Resources/PrefPanes/Keyboard.prefPane /System/Library/PreferencePanes/ > /dev/null &" with administrator privileges
			do shell script "chown -R root:wheel /System/Library/PreferencePanes/Keyboard.prefPane > /dev/null &" with administrator privileges
		end if
	on error errMsg number errorNumber
		display dialog "Could not install 10.5.5 keyboard kext. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
		quit
	end try
	
	try
		if enableremote then
			if RemoteCDP then
				set contents of text field "currentop" of window "DellEFI Installer" to "Disabling Remote CD"
				delay 1
				do shell script "defaults delete com.apple.NetworkBrowser EnableODiskBrowsing"
				do shell script "defaults delete com.apple.NetworkBrowser ODSSupported"
				
				set needreboot to true
			else
				--enable remote cd
				set contents of text field "currentop" of window "DellEFI Installer" to "Enabling Remote CD"
				delay 1
				do shell script "defaults write com.apple.NetworkBrowser EnableODiskBrowsing -bool true"
				do shell script "defaults write com.apple.NetworkBrowser ODSSupported -bool true"
				
				set needreboot to true
			end if
		end if
	on error errMsg number errorNumber
		display dialog "Could not configure Remote CD. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
		quit
	end try
	
	try
		if disablehibernate then
			if HibernationP then
				set contents of text field "currentop" of window "DellEFI Installer" to "Enabling hibernate to file"
				delay 1
				do shell script "pmset hibernatemode 3 > /dev/null &" with administrator privileges
			else
				set contents of text field "currentop" of window "DellEFI Installer" to "Disabling hibernate to file"
				delay 1
				do shell script "pmset hibernatemode 0 > /dev/null &" with administrator privileges
				do shell script "rm /var/vm/sleepimage > /dev/null &" with administrator privileges
			end if
		end if
	on error errMsg number errorNumber
		display dialog "Could not configure Hibernation. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
		quit
	end try
	
	try
		if hideefi then
			if HideEFIP then
				set contents of text field "currentop" of window "DellEFI Installer" to "Making DellEFI files visible"
				delay 1
				do shell script "chflags nohidden /dsdt.aml > /dev/null &" with administrator privileges
				do shell script "chflags nohidden /boot > /dev/null &" with administrator privileges
				do shell script "chflags nohidden /Extra > /dev/null &" with administrator privileges
			else
				set contents of text field "currentop" of window "DellEFI Installer" to "Hiding DellEFI files"
				delay 1
				do shell script "chflags hidden /dsdt.aml > /dev/null &" with administrator privileges
				do shell script "chflags hidden /boot > /dev/null &" with administrator privileges
				do shell script "chflags hidden /Extra > /dev/null &" with administrator privileges
			end if
		end if
	on error errMsg number errorNumber
		display dialog "Could not perform DellEFI file hiding. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
		quit
	end try
	
	--reboot
	tell progress indicator "progress" of window "DellEFI Installer"
		stop
		set visible to false
	end tell
	
	set contents of text field "currentop" of window "DellEFI Installer" to "Finished"
	if needreboot then
		display dialog "All done, ready for reboot" buttons ["No", "Yes"] default button "Yes" with icon caution
		if button returned of result is "No" then
			quit
		else
			tell application "System Events"
				restart
			end tell
		end if
	else
		display dialog "All done, no need to reboot!" buttons ["Close"] with icon caution
		quit
	end if
end clicked