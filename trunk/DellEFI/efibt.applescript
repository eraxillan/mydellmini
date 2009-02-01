--  Created by Bernard Maltais on 27/01/09.
--  Copyright 2009 __MyCompanyName__. All rights reserved.
on clicked theObject
	if state of button "eficb" of box "optionspanel" of window "DellEFI Installer" is 0 then
		tell button "dsdtcb" of box "optionspanel" of window "DellEFI Installer"
			set enabled to false
		end tell
	else
		set dsdt_exists to do shell script "test -e /dsdt.aml && echo 'file exists' || echo 'no file'"
		if dsdt_exists is "file exists" then
			tell button "dsdtcb" of box "optionspanel" of window "DellEFI Installer"
				set enabled to false
			end tell
		end if
	end if
	
end clicked