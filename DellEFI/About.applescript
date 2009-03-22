(* 
Changes:

-Fixed issues with version string handling
 */

/*****************************************************************************)
on choose menu item theObject
	tell window "About"
		set visible to true
	end tell
end choose menu item

on clicked theObject
	tell window "About"
		set visible to false
	end tell
end clicked
