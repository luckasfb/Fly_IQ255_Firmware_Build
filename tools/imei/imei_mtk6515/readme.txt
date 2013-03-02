
This archive contains the utility for android devices based on MTK6515 chip.
Those devices do support 2 SIM cards with uniq IMEI codes which tend to be
lost while reflashing.

The IMEI info is stored in the file named MP0B_001, which can be placed in
one of two paths:
1) /nvram/md/NVRAM/NVD_IMEI/
2) /data/nvram/md/NVRAM/NVD_IMEI/
In the first case it must not be affected by resetting to factory defaults.
In second one it WOULD be erased.

So the utility helps to create the new file named MP0B_001 with both IMEI 
encoded into it.

The program is a result of http://forum.china-iphone.ru users discussion.
Algorythm autor: andryn
Ideas by f_mulder, pvsurkov
Compilled by hh85

Files: 
imei.arm   - executable for run directly on phone
imei.i386  - executable for linux PC
imei.exe   - executable for windows PC
compile.sh - Debian scrip to compile all above
imei.c     - source code

Usage: imei.xxx <15digit_IMEI_1> [<15digit_IMEI_2>] [output file]

If both IMEI you gave are in correct format the file named MP0B_001_NEW will 
be created. Now you can move it to /nvram/md/NVRAM/NVD_IMEI/MP0B_001 (path 
for new firmware) or to /data/nvram/md/NVRAM/NVD_IMEI/MP0B_001 (for old one).

Note: device must be rooted!

One more note: output file name must NOT be a 15 digit or it would be 
treated as IMEI.

Changes from original version.
1. Check IMEI(s) for a valid checksum (last digit).
   Thanks for idea, f_mulder!
2. Added parameter for output file name.
3. Check if output file exists. If so, exit with error.
4. Check if we can't write to output file because of invalid path or
   permissions restriction.
5. Default file name is 'MP0B_001' now.
6. Don't allow to use 2 same IMEI numbers at once. It can cause denial of
   service with some GSM operators.

Please post bug reports here: http://forum.china-iphone.ru/viewtopic.php?f=31&t=12628

