1. Make a directory:

mkdir newiso

2. Inside the folder, copy cdboot from lastest chameleon binairies, and create folder called Extra.


3. Inside Extra folder, copy your com.apple.boot.plist, and add those line to the .plist:

> 

&lt;key&gt;

Timeout

&lt;/key&gt;


> 

&lt;string&gt;

5

&lt;/string&gt;


> 

&lt;key&gt;

Rescan Prompt

&lt;/key&gt;


> 

&lt;string&gt;

yes

&lt;/string&gt;



4.. Extra folder is also the place for dsdt.aml, others .plists, themes...etc.

5. Still inside Extra folder, create a dmg, and name it Preboot.dmg (it's the RAM disk)
Inside the dmg, copy you extensions (Extra/Extensions/) or Extensions.mkext (Extra/).

6. Now open Terminal, then type:

hdiutil makehybrid -o new.iso newiso/ -iso -hfs -joliet -eltorito-boot newiso/cdboot -no-emul-boot -hfs-volume-name "My Boot CD" -joliet-volume-name "My Boot CD"

7. Burn the ISO. You're done.


---


Boot the Retail DVD:
When the bootloader ask you for rescan, press ENTER.
Go to boot options (f8), Swap disks, then press ESC to rescan drive. When the name of the DVD appears, press ENTER.

Boot Partitions:
When the bootloader ask you for rescan, press any key (not ENTER).
Go to Boot options (f8), then choose your partition.

If you get an instant reboot, turn your controller in AHCI mode.

Thanks to Zef for help, and all the team for this great booloader.