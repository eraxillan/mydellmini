-- MiniScript.applescript
-- MiniScript

--  Created by Type11 on 1/1/09.
--  V7.11 beta 6 by bmaltais on 26/1/09


on awake from nib theObject
	
	tell every window to center
	
	tell window "Initializing"
		set visible to true
	end tell
	
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
	
	tell window "Initializing"
		set visible to false
	end tell
	
	tell window "Mini9 Installer"
		set visible to true
	end tell
end awake from nib
