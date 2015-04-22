# DSDT Battery Refrences #

The following list of names are memory locations, the size in bytes of the location, and what I believe they are for
| EC0.NAME | size of | What its for | What we use it for |
|:---------|:--------|:-------------|:-------------------|
| BMF0 | 3 | Manufacturer Code,	My KR battery = 0x07 | Used in\_BIF method |
| BTY0 | 1 |  Battery type (1 = rechargeable) | (_BIF)_|
| BST0 | 8 | Battery state bitfield | (_BST)_|
| BRC0 | 16 | Remaining capacty | (_BST)_|
| BSN0 | 16 | Serial number | (_BIF) Handled incorrectly by kext_|
| BPV0 | 16 | Present Voltage | (_BST)_|
| BDV0 | 16 | Design Volatge | (_BIF)_|
| BDC0 | 16 | Design Capacity (watts / amps) | (_BIF)_|
| BFC0 | 16 | Last Full current | (_BIF)_|
| GAU0 | 8 | Percentage full | Not used, calculated from BRC and BFC |
| CYC0 | 8 | Cycle count |  |
| BPC0 | 16 | Batter current (Inst rate) | (_BST) , must be ones complimented_|
| BAC0 | 16 | Battery current (Avg Rate) | Not used, can be used for (_BST)_|
| BAT0 | 8 | Battery bitfield (may be similar to BST0), lsb is 0 when charging / AC and 1 when discharging / on battery |  |
| BTW0 | 16 |  |  |
| BDN0 | 8 | Display Number ??? (BAT1, BAT2 etc) |  |