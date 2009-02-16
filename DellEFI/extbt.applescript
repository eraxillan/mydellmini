--  Created by Bernard Maltais on 27/01/09.
--  Copyright 2009 __MyCompanyName__. All rights reserved.
on clicked theObject
	if state of button "extensionscb" of box "optionspanel" of window "DellEFI Installer" is 0 then
		tell button "oldgmacb" of box "optionspanel" of window "DellEFI Installer"
			set enabled to false
		end tell
	else
		tell button "oldgmacb" of box "optionspanel" of window "DellEFI Installer"
			set enabled to true
		end tell
	end if
	
end clicked