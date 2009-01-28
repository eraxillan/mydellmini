-- type11bt.applescript
-- MiniScript

--  Created by Bernard Maltais on 27/01/09.
--  Copyright 2009 __MyCompanyName__. All rights reserved.
on clicked theObject
	if state of button "eficb" of box "optionspanel" of window "Mini9 Installer" is 0 then
		tell button "twofingercb" of box "optionspanel" of window "Mini9 Installer"
			set enabled to false
		end tell
		
		tell button "dsdtcb" of box "optionspanel" of window "Mini9 Installer"
			set enabled to false
		end tell
		
		tell button "quietbootcb" of box "optionspanel" of window "Mini9 Installer"
			set enabled to false
		end tell
	else
		tell button "twofingercb" of box "optionspanel" of window "Mini9 Installer"
			set enabled to true
		end tell
		
		set dsdt_exists to do shell script "test -e /.Type11/DSDT.aml && echo 'file exists' || echo 'no file'"
		if dsdt_exists is "file exists" then
			tell button "dsdtcb" of box "optionspanel" of window "Mini9 Installer"
				set enabled to false
			end tell
		end if
		
		tell button "quietbootcb" of box "optionspanel" of window "Mini9 Installer"
			set enabled to true
		end tell
	end if
	
end clicked