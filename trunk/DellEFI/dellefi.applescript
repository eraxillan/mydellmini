-- dellefi.applescript
-- dellefi

--  Created by Bernard Maltais on 28/01/09.

property QuietBootP : false
property HideEFIP : false
property RemoteCDP : false
property HibernationP : false
property FingerP : false
property dsdtP : false
property dellefiver : "1.1"
property needUpdate : false
property needreboot : false
property needtoremovedsdt : false
property workingDir : ""

on awake from nib theObject
	
	tell every window to center
	
	tell window "Initializing"
		tell progress indicator "spinner"
			set uses threaded animation to true
			start
		end tell
		set visible to true
	end tell
	
	set contents of text field "verlb" of window "DellEFI Installer" to "v" & dellefiver
	
	-- try
	-- 	set OSVer to do shell script "sw_vers | grep 'ProductVersion:' | awk '{print $2}'"
	
	-- 	if OSVer is not equal to "10.5.6" then
	-- display dialog "DellEFI is only meant for 10.5.6 system. Please upgrade to 10.5.6 before running again." buttons ["Quit"] default button "Quit" with icon caution
	-- 	end if
	-- on error errMsg number errorNumber
	-- 	display dialog "Could not detect OSX version. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
	-- 	quit
	-- end try
	
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
		display dialog "Could not detect MAC address. Do you want to continue? You are on a Dell Mini 9 right?" buttons ["No", "Yes"] default button "No" with icon caution
		if button returned of result is "No" then
			quit
		end if
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
		set dsdtP to true
		tell button "dsdtcb" of box "optionspanel" of window "DellEFI Installer"
			set integer value to 0
			set title to "Remove custom dsdt.aml file"
			-- set enabled to false
		end tell
	end if
	
	set extexist to do shell script "test -e /Extra/Extensions.mkext && echo 'file exists' || echo 'no file'"
	set extver to do shell script "test -e /.dellefi/.extv" & dellefiver & " && echo 'file exists' || echo 'no file'"
	if extexist is "file exists" then
		if extver is "file exists" then
			tell button "extensionscb" of box "optionspanel" of window "DellEFI Installer"
				set integer value to 0
				set title to "Reinstall Dell Mini 9 Extensions"
			end tell
			tell button "oldgmacb" of box "optionspanel" of window "DellEFI Installer"
				set enabled to false
			end tell
		else
			if dsdt_exists is "file exists" then
				tell button "dsdtcb" of box "optionspanel" of window "DellEFI Installer"
					set integer value to 1
					set title to "Remove custom dsdt.aml file"
					-- set enabled to false
				end tell
				tell button "extensionscb" of box "optionspanel" of window "DellEFI Installer"
					set integer value to 0
					set enabled to false
				end tell
				set needtoremovedsdt to true
			else
				set needUpdate to true
			end if
		end if
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
	
	set fingerexist to do shell script "test -e /.dellefi/.2finger && echo 'file exists' || echo 'no file'"
	if fingerexist is "file exists" then
		set FingerP to true
		tell button "2fingercb" of box "optionspanel" of window "DellEFI Installer"
			set title to "Remove 2 finger scroll"
			-- set enabled to false
		end tell
	end if
	
	set bluetoothexist to do shell script "test -e /Library/Preferences/com.apple.Bluetooth.plist && echo 'file exists' || echo 'no file'"
	if bluetoothexist is "no file" then
		tell button "fixbluetoothcb" of box "optionspanel" of window "DellEFI Installer"
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
	
	-- Need to remove old dsdt before upgrading.  Ensure a clean upgrade of the dsdt from a virgin one.
	if needtoremovedsdt is true then
		display dialog "DellEFI need to remove the existing dsdt file and then reboot before upgrading!" buttons ["Remove", "Keep it"] default button "Quit" with icon caution
		if button returned of result is "Remove" then
			do shell script "rm -f /dsdt.aml" with administrator privileges
			tell application "System Events"
				restart
			end tell
		end if
	end if
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
	set oldgma to false
	set quietboot to false
	set needreboot to false
	set twofinger to false
	set bluetooth to false
	
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
	if state of button "oldgmacb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set oldgma to true
	end if
	if state of button "quietbootcb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set quietboot to true
	end if
	if state of button "2fingercb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set twofinger to true
	end if
	if state of button "fixbluetoothcb" of box "optionspanel" of window "DellEFI Installer" is 1 then
		set bluetooth to true
	end if
	
	set workingDir to path to me
	set workingDir to POSIX path of workingDir
	
	set x to the length of the workingDir
	set workingDir to characters 1 thru (x - 1) of workingDir as string
	set workingDir to "\"" & workingDir & "\"/Contents/Resources"
	-- display dialog workingDir
	
	set disk to do shell script "df -k / | grep dev | awk -F\" \" '{print $1}' | awk -F\"/\" '{print $3}'"
	set x to the length of the disk
	set disk to characters 1 thru (x - 2) of disk as string
	
	try
		if efi is true then
			-- set currdisk to do shell script "ls -l /Volumes/ | grep \" /\" | grep root | awk '{print $9}'"
			
			set contents of text field "currentop" of window "DellEFI Installer" to "Installing bootloader"
			delay 1
			do shell script workingDir & "/bootpcefiv9/fdisk -f " & workingDir & "/bootpcefiv9/boot0 -u -y /dev/r" & disk & " > /dev/null &" with administrator privileges
			do shell script "dd if=" & workingDir & "/bootpcefiv9/boot1h of=/dev/r" & disk & "s2 > /dev/null &" with administrator privileges
			do shell script "cp " & workingDir & "/bootpcefiv9/boot /boot > /dev/null &" with administrator privileges
			
			set contents of text field "currentop" of window "DellEFI Installer" to "Installing com.apple.Boot"
			delay 1
			do shell script "cp " & workingDir & "/Boot/com.apple.Boot.plist  /Library/Preferences/SystemConfiguration/com.apple.Boot.plist" with administrator privileges
		end if
	on error errMsg number errorNumber
		display dialog "Could not install the Bootloader. Error " & errorNumber as text buttons ["Continue"] default button "Continue" with icon caution
	end try
	
	try
		if quietboot then
			if QuietBootP then
				set contents of text field "currentop" of window "DellEFI Installer" to "Disabling Quiet Boot"
				delay 1
				do shell script "cp " & workingDir & "/Boot/com.apple.Boot.plist  /Library/Preferences/SystemConfiguration/com.apple.Boot.plist" with administrator privileges
			else
				set contents of text field "currentop" of window "DellEFI Installer" to "Configuring Quiet Boot"
				delay 1
				do shell script "cp " & workingDir & "/Boot/com.apple.Boot.Quiet.plist  /Library/Preferences/SystemConfiguration/com.apple.Boot.plist" with administrator privileges
			end if
		else
			if needUpdate then
				if QuietBootP then
					set contents of text field "currentop" of window "DellEFI Installer" to "Updating Quiet Boot"
					delay 1
					do shell script "cp " & workingDir & "/Boot/com.apple.Boot.Quiet.plist  /Library/Preferences/SystemConfiguration/com.apple.Boot.plist" with administrator privileges
				else
					set contents of text field "currentop" of window "DellEFI Installer" to "Updating Boot"
					delay 1
					do shell script "cp " & workingDir & "/Boot/com.apple.Boot.plist  /Library/Preferences/SystemConfiguration/com.apple.Boot.plist" with administrator privileges
				end if
			end if
		end if
	on error errMsg number errorNumber
		display dialog "Could not disable quiet boot. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
	end try
	
	try
		if extensionsfiles is true then
			try
				do shell script "mkdir /.dellefi; touch" with administrator privileges
				do shell script "/.dellefi/.donoterase" with administrator privileges
			end try
			
			try
				do shell script "rm /.dellefi/.extv*" with administrator privileges
			end try
			try
				do shell script "touch /.dellefi/.extv" & dellefiver as text with administrator privileges
			end try
			
			-- move all extensions over to EFI ext dir.
			set contents of text field "currentop" of window "DellEFI Installer" to "Copy kexts to Extra folder"
			delay 1
			try
				do shell script "rf -r /Extra.bak" with administrator privileges
			end try
			try
				do shell script "mv /Extra /Extra.bak" with administrator privileges
			end try
			try
				do shell script "mkdir /Extra > /dev/null &" with administrator privileges
				do shell script "mkdir /Extra/Mini9Ext" with administrator privileges
			end try
			do shell script "cp -R " & workingDir & "/UpdateExtra.app /Extra" with administrator privileges
			
			do shell script "cp " & workingDir & "/bin/binmay /usr/bin" with administrator privileges
			do shell script "chmod -R 755 /usr/bin/binmay" with administrator privileges
			
			-- Patch AppleIntelGMA950.kext
			do shell script "cp -R /System/Library/Extensions/AppleIntelGMA950.kext /Extra/Mini9Ext/" with administrator privileges
			do shell script "/usr/bin/binmay -i /Extra/Mini9Ext/AppleIntelGMA950.kext/Contents/MacOS/AppleIntelGMA950 -o /Extra/Mini9Ext/AppleIntelGMA950.kext/Contents/MacOS/AppleIntelGMA950.hex -s 8680A227 -r 8680AE27" with administrator privileges
			do shell script "mv /Extra/Mini9Ext/AppleIntelGMA950.kext/Contents/MacOS/AppleIntelGMA950.hex /Extra/Mini9Ext/AppleIntelGMA950.kext/Contents/MacOS/AppleIntelGMA950" with administrator privileges
			do shell script "/usr/bin/perl -pe \"s/27A28086/27AE8086/g\" /Extra/Mini9Ext/AppleIntelGMA950.kext/Contents/Info.plist > /Extra/Mini9Ext/AppleIntelGMA950.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "mv /Extra/Mini9Ext/AppleIntelGMA950.kext/Contents/Info.plist.new /Extra/Mini9Ext/AppleIntelGMA950.kext/Contents/Info.plist" with administrator privileges
			
			-- Patch AppleIntelIntegratedFramebuffer.kext
			do shell script "cp -R /System/Library/Extensions/AppleIntelIntegratedFramebuffer.kext /Extra/Mini9Ext/" with administrator privileges
			do shell script "/usr/bin/binmay -i /Extra/Mini9Ext/AppleIntelIntegratedFramebuffer.kext/AppleIntelIntegratedFramebuffer -o /Extra/Mini9Ext/AppleIntelIntegratedFramebuffer.kext/AppleIntelIntegratedFramebuffer.hex -s 8680A227 -r 8680AE27" with administrator privileges
			do shell script "mv /Extra/Mini9Ext/AppleIntelIntegratedFramebuffer.kext/AppleIntelIntegratedFramebuffer.hex /Extra/Mini9Ext/AppleIntelIntegratedFramebuffer.kext/AppleIntelIntegratedFramebuffer" with administrator privileges
			do shell script "/usr/bin/perl -pe \"s/27A28086/27AE8086/g\" /Extra/Mini9Ext/AppleIntelIntegratedFramebuffer.kext/Info.plist > /Extra/Mini9Ext/AppleIntelIntegratedFramebuffer.kext/Info.plist.new" with administrator privileges
			do shell script "mv /Extra/Mini9Ext/AppleIntelIntegratedFramebuffer.kext/Info.plist.new /Extra/Mini9Ext/AppleIntelIntegratedFramebuffer.kext/Info.plist" with administrator privileges
			
			-- Copy IONetworkingFamily.kext
			do shell script "cp -R /System/Library/Extensions/IONetworkingFamily.kext /Extra/Mini9Ext/" with administrator privileges
			
			-- Copy IONDRVSupport.kext
			do shell script "cp -R /System/Library/Extensions/IONDRVSupport.kext /Extra/Mini9Ext/" with administrator privileges
			
			-- Copy IOGraphicsFamily.kext
			do shell script "cp -R /System/Library/Extensions/IOGraphicsFamily.kext /Extra/Mini9Ext/" with administrator privileges
			
			-- Patch IO80211Family.kext
			
			do shell script "cp -R /System/Library/Extensions/IO80211Family.kext /Extra/Mini9Ext/" with administrator privileges
			do shell script "cat /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist|grep -B 100 \"<array>\" > /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4306</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4309</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4328</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4329</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,432a</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4311</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4312</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4313</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4318</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4319</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,431a</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4320</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4324</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "echo \"				<string>pci14e4,4315</string>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "cat /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist|grep -A 100 \"</array>\" >> /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new" with administrator privileges
			do shell script "mv /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist.new /Extra/Mini9Ext/IO80211Family.kext/Contents/PlugIns/AppleAirPortBrcm4311.kext/Contents/Info.plist" with administrator privileges
			
			-- Copy other required opensource kext
			do shell script "cp -R " & workingDir & "/Extensions/*.kext /Extra/Mini9Ext" with administrator privileges
			-- do shell script "cp " & workingDir & "/bin/rmdellefi /usr/bin" with administrator privileges
			-- do shell script "chmod -R 755 /usr/bin/rmdellefi" with administrator privileges
			
			try
				--restore old GMA drivers with functional mirror mode
				if oldgma is true then
					set contents of text field "currentop" of window "DellEFI Installer" to "Installing old GMA kext"
					delay 1
					try
						do shell script "rm -r /Extra/Mini9Ext/AppleIntelIntegratedFramebuffer.kext" with administrator privileges
					end try
					do shell script "cp -R " & workingDir & "/oldgma/*.kext /Extra/Mini9Ext" with administrator privileges
				end if
			on error errMsg number errorNumber
				display dialog "Could not install old GMA kext. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
			end try
			
			do shell script "chown -R 0:0 /Extra/" with administrator privileges
			do shell script "chmod -R 755 /Extra/" with administrator privileges
			
			set contents of text field "currentop" of window "DellEFI Installer" to "Update Extra kext cache"
			delay 1
			do shell script "kextcache -a i386 -m /Extra/Extensions.mkext /Extra/Mini9Ext" with administrator privileges
			
			--clean up old audio installs just in case there are some bits left over
			set contents of text field "currentop" of window "DellEFI Installer" to "Remove old audio/sleep files"
			delay 1
			do shell script "rm -r /System/Library/Extensions/ALCinject.kext > /dev/null &" with administrator privileges
			do shell script "rm -r /System/Library/Extensions/HDAEnabler.kext > /dev/null &" with administrator privileges
			
			set contents of text field "currentop" of window "DellEFI Installer" to "Installing local files"
			delay 1
			-- move items that need to be local for audio and battery, hopefully this goes away someday
			do shell script "cp -R " & workingDir & "/LocalExtensions/*.kext /System/Library/Extensions > /dev/null &" with administrator privileges
			do shell script "chown -R 0:0 /System/Library/Extensions/AppleHDA.kext" with administrator privileges
			do shell script "chmod -R 755 /System/Library/Extensions/AppleHDA.kext" with administrator privileges
			do shell script "chown -R 0:0 /System/Library/Extensions/ClamshellDisplay.kext" with administrator privileges
			do shell script "chmod -R 755 /System/Library/Extensions/ClamshellDisplay.kext" with administrator privileges
			do shell script "chown -R 0:0 /System/Library/Extensions/IOAudioFamily.kext" with administrator privileges
			do shell script "chmod -R 755 /System/Library/Extensions/IOAudioFamily.kext" with administrator privileges
			-- remove mkext so it is rebuilt
			do shell script "rm -r /System/Library/Extensions.mkext > /dev/null &" with administrator privileges
			
			do shell script "cp -R " & workingDir & "/SystemConfiguration/*.bundle /System/Library/SystemConfiguration > /dev/null &" with administrator privileges
			
			set contents of text field "currentop" of window "DellEFI Installer" to "Installing Mirroring application"
			delay 1
			try
				do shell script "cp " & workingDir & "/bin/mirroring /usr/bin" with administrator privileges
				do shell script "chmod -R 755 /usr/bin/mirroring" with administrator privileges
			end try
			
			-- Install color profile
			set contents of text field "currentop" of window "DellEFI Installer" to "Installing Color Profile"
			delay 1
			try
				do shell script "cp " & workingDir & "/colorsync/* /Library/ColorSync/Profiles/" with administrator privileges
			end try
			
			-- Remove temporary binmay from system
			
			try
				do shell script "rm /usr/bin/binmay" with administrator privileges
			end try
			
			set needreboot to true
		end if
	on error errMsg number errorNumber
		display dialog "Could not install Extensions. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
	end try
	
	try
		--make custom aml file and copy to EFI part root
		if dsdt is true then
			if dsdtP then
				set contents of text field "currentop" of window "DellEFI Installer" to "Deleting dsdt.aml file"
				delay 1
				try
					do shell script "rm -f /dsdt.aml" with administrator privileges
					set needreboot to true
				end try
			else
				set contents of text field "currentop" of window "DellEFI Installer" to "Creating dsdt.aml file"
				delay 1
				
				makeDSDT()
				
				set needreboot to true
			end if
		else
			-- Check if we are doing an update and if an existing dsdt exist.
			if dsdtP then
				if needUpdate then
					set contents of text field "currentop" of window "DellEFI Installer" to "Updating dsdt.aml file"
					delay 1
					
					makeDSDT()
					
					set needreboot to true
				end if
			end if
		end if
	on error errMsg number errorNumber
		display dialog "Could not create dsdt.aml file. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
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
			do shell script "cp -R " & workingDir & "/PrefPanes/Keyboard.prefPane /System/Library/PreferencePanes/ > /dev/null &" with administrator privileges
			do shell script "chown -R root:wheel /System/Library/PreferencePanes/Keyboard.prefPane > /dev/null &" with administrator privileges
		end if
	on error errMsg number errorNumber
		display dialog "Could not install 10.5.5 keyboard kext. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
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
		else
			-- If user does not ask to hide or reveal files then check if files where hidden and then apply hiding again to hide updated files
			if HideEFIP then
				set contents of text field "currentop" of window "DellEFI Installer" to "Hiding updated DellEFI files"
				delay 1
				do shell script "chflags hidden /dsdt.aml > /dev/null &" with administrator privileges
				do shell script "chflags hidden /boot > /dev/null &" with administrator privileges
				do shell script "chflags hidden /Extra > /dev/null &" with administrator privileges
			end if
		end if
	on error errMsg number errorNumber
		display dialog "Could not perform DellEFI file hiding. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
	end try
	
	try
		if twofinger then
			if FingerP then
				set contents of text field "currentop" of window "DellEFI Installer" to "Removing two finger scrolling"
				delay 1
				try
					do shell script "rm /.dellefi/.2finger" with administrator privileges
				end try
				try
					do shell script "rm -r /Extra/Mini9Ext/ApplePS2Controller.kext" with administrator privileges
				end try
				try
					do shell script "cp -R " & workingDir & "/Extensions/ApplePS2Controller.kext /Extra/Mini9Ext" with administrator privileges
				end try
				try
					do shell script "rm /usr/local/bin/FFScrollDaemon" with administrator privileges
				end try
				try
					do shell script "rm /usr/local/bin/start_FFScrollDaemon" with administrator privileges
				end try
				try
					do shell script "rm /Library/Preferences/com.apple.driver.ApplePS2Trackpad.plist" with administrator privileges
				end try
				try
					do shell script "rm /Library/LaunchAgents/com.apple.FFScrollDaemon.plist" with administrator privileges
				end try
			else
				try
					do shell script "mkdir /.dellefi" with administrator privileges
				end try
				try
					do shell script "touch /.dellefi/.2finger" with administrator privileges
				end try
				
				set contents of text field "currentop" of window "DellEFI Installer" to "Installing two finger scrolling"
				delay 1
				do shell script "cp -R " & workingDir & "/2FingerScroll/ApplePS2Controller.kext /Extra/Mini9Ext" with administrator privileges
				do shell script "chown -R root:wheel /Extra/Mini9Ext/ApplePS2Controller.kext" with administrator privileges
				try
					do shell script "mkdir -p /usr/local/bin > /dev/null &" with administrator privileges
				end try
				do shell script "cp " & workingDir & "/2FingerScroll/FFScrollDaemon /usr/local/bin" with administrator privileges
				do shell script "chmod 755 /usr/local/bin/FFScrollDaemon" with administrator privileges
				do shell script "chown root:wheel /usr/local/bin/FFScrollDaemon" with administrator privileges
				do shell script "cp " & workingDir & "/2FingerScroll/start_FFScrollDaemon /usr/local/bin" with administrator privileges
				do shell script "chmod 755 /usr/local/bin/start_FFScrollDaemon" with administrator privileges
				do shell script "chown root:wheel /usr/local/bin/start_FFScrollDaemon" with administrator privileges
				do shell script "cp " & workingDir & "/2FingerScroll/com.apple.driver.ApplePS2Trackpad.plist /Library/Preferences/" with administrator privileges
				do shell script "chmod 644 /Library/Preferences/com.apple.driver.ApplePS2Trackpad.plist" with administrator privileges
				do shell script "chown root:admin /Library/Preferences/com.apple.driver.ApplePS2Trackpad.plist" with administrator privileges
				do shell script "cp " & workingDir & "/2FingerScroll/com.apple.FFScrollDaemon.plist /Library/LaunchAgents/" with administrator privileges
				do shell script "chmod 644 /Library/LaunchAgents/com.apple.FFScrollDaemon.plist" with administrator privileges
				do shell script "chown root:wheel /Library/LaunchAgents/com.apple.FFScrollDaemon.plist" with administrator privileges
			end if
			
			set contents of text field "currentop" of window "DellEFI Installer" to "Update Extra kext cache"
			delay 1
			do shell script "kextcache -a i386 -m /Extra/Extensions.mkext /Extra/Mini9Ext" with administrator privileges
		end if
	on error errMsg number errorNumber
		display dialog "Could not perform configuration of 2 finger scrolling. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
	end try
	
	try
		if bluetooth then
			set contents of text field "currentop" of window "DellEFI Installer" to "Fixing bluetooth"
			delay 1
			try
				do shell script "rm /Library/Preferences/com.apple.Bluetooth.plist" with administrator privileges
			end try
		end if
	on error errMsg number errorNumber
		display dialog "Could not fix bluetooth. Error " & errorNumber as text buttons ["Quit"] default button "Quit" with icon caution
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

on makeDSDT()
	try
		do shell script "mkdir /.dellefi; touch /.dellefi/.donoterase" with administrator privileges
	end try
	
	try
		do shell script "rm -rf /.dellefi/DSDTPatcher" with administrator privileges
	end try
	
	try
		do shell script "cp -Rf " & workingDir & "/DSDTPatcher /.dellefi/" with administrator privileges
	end try
	
	do shell script "cd /.dellefi/DSDTPatcher; ./DSDTPatcher > /dev/null 2>&1 &" with administrator privileges
	delay 10
	
	do shell script "cp /.dellefi/DSDTPatcher/dsdt.aml /dsdt.aml" with administrator privileges
	
	set needreboot to true
end makeDSDT