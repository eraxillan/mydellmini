-- radiobt.applescript
-- DellEFI

--  Created by Bernard Maltais on 31/01/09.
--  Copyright 2009 __MyCompanyName__. All rights reserved.

on changegui()
	tell matrix 1 of window "DellEFI Installer"
		-- display dialog (name of current cell) as string
		set guitype to (name of current cell) as string
	end tell
	
	if guitype is equal to "beginnerrb" then
		set guivalue to false
	else
		set guivalue to true
	end if
	
	tell button "dsdtcb" of box "optionspanel" of window "DellEFI Installer"
		set enabled to guivalue
	end tell
	
	tell button "keyboardpanecb" of box "optionspanel" of window "DellEFI Installer"
		set enabled to guivalue
	end tell
	
	tell button "enableremotecb" of box "optionspanel" of window "DellEFI Installer"
		set enabled to guivalue
	end tell
	
	tell button "disablehibernatecb" of box "optionspanel" of window "DellEFI Installer"
		set enabled to guivalue
	end tell
	
	tell button "eficb" of box "optionspanel" of window "DellEFI Installer"
		set enabled to guivalue
	end tell
	
	tell button "hidecb" of box "optionspanel" of window "DellEFI Installer"
		set enabled to guivalue
	end tell
	
	tell button "extensionscb" of box "optionspanel" of window "DellEFI Installer"
		set enabled to guivalue
	end tell
	
	tell button "quietbootcb" of box "optionspanel" of window "DellEFI Installer"
		set enabled to guivalue
	end tell
	
	tell button "2fingercb" of box "optionspanel" of window "DellEFI Installer"
		set enabled to guivalue
	end tell
	tell button "fixbluetoothcb" of box "optionspanel" of window "DellEFI Installer"
		set enabled to guivalue
	end tell
	tell button "oldgmacb" of box "optionspanel" of window "DellEFI Installer"
		set enabled to guivalue
	end tell
	
end changegui

on clicked theObject
	changegui()
end clicked

on awake from nib theObject
	changegui()
end awake from nib
