# CMS-370-BREXX
This is a port of Vasilis Vlachoudis's bREXX for the CMS system of VM/370.

See Vlachoudis's original [README.md](READMEVV.md) for his explanation of
bREXX, as it was originally written for MS-DOS, and as it grew to support Unix,
Linux, and MS-Windows.

# Download

The source code for VM/370 CMS bREXX is available from
[RossPatterson/CMS-370-BREXX](https://github.com/RossPatterson/CMS-370-BREXX)
on GitHub.  You can download that and build it yourself, on CMS, using the
[tools](https://github.com/RossPatterson/CMS-370-BREXX/tree/master/tools/tooldisk.memo)
it includes.

Pre-built releases of VM/370 CMS bREXX are available at the
[Releases](https://github.com/RossPatterson/CMS-370-releases) page at that same
GitHub repository.  You can download the release ZIP file and install it without
having to compile anything.

# Installation

There are several different ways you can install bREXX for VM/370 CMS.  They
all produce the same results, it's just a matter of your preference as to how
you do it.  For turnkey users (_e.g._,
[VM/370 Community Edition](http://www.vm370.org/vm) or
[VM/370 SixPack 1.3](https://web.archive.org/web/20221216071632/http://www.smrcc.org.uk/members/g4ugm/vm-370/vm370sixpack-1_3.zip)),
the recommended process is to replace the `GCCBRX` DASD volume.

## Install pre-built via the GCCBRX DASD volume.

If you're running VM/370 under the Hercules S/370 emulator, you can add the
`gccbrx.cckd` emulated disk file to your configuration, add the statements in
`MAINTC DIRECT` to your CP directory source (typically `USER DIRECT` on the
`MAINT 191` minidisk), and update your Hercules configuration to make it
available to CP.

Note that if you already have GCCLIB for CMS and bREXX installed on your VM/370
system (_e.g._, you're running a turnkey system, or you've previously installed
bREXX this way), you probably already have a `GCCBRX` DASD volume and the
associated directory and configuration statements in place.  Assuming you
haven't modified any minidisks on the `GCCBRX` volume, you can replace it with
the one from the new release.

1. Download the pre-built release `BREXX.zip` file from the location above to
   the machine where you run Hercules.
2. Unzip `BREXX.zip`.
3. Copy the `gccbrx.cckd` file to the folder where you keep your emulated DASD
   files.
4. Log on to `MAINT` on VM.
5. Upload the `maintc.direct` file to VM, add it to your `USER DIRECT` file,
   and install the updated directory (but see the VM/CE and SixPack note above).
6. Upload the `newbrexx.exec` file to `MAINT` on VM (typically to the
   `MAINT 5E5` disk) as `NEWBREXX EXEC`.
7. Re-save the `GCCLIB` saved segment:
   1. `DEFINE STORAGE 16M`
   2. `IPL CMS`
   3. `ACCESS (NOPROF`
   4. `GCCSEG F20000 GCCLIB`
   5. Results: `GCCSEG COMPLETE`.
   6. `IPL CMS`
8. Install all the bREXX files (including some on the Y-disk): `NEWBREXX`
9. Re-save the CMS saved system to update the shared Y-stat:
   1. `IPL 190 CLEAR`
   2. `SAVESYS CMS`
   3. Results: `SYSTEM SAVED`.

## Install pre-built via VMARC.

1. Download the pre-built release `BREXX.zip` file from the location above to
   the machine where you run Hercules.
2. Unzip `BREXX.zip`.
3. Log on to `MAINT` on VM.
4. Upload `brexxbin.vmarc` to VM in binary, fixed format, record length 80 as
   `BREXXBIN VMARC`.
5. Extract the pre-built bREXX file: `VMARC UNPK BREXXBIN VMARC A BREXX TEXT A (OLDDATE`.
6. Move the `BREXX TEXT` file to the Y-disk:
   1. `ACCESS 19E Y`
   2. `COPY BREXX TEXT A = = Y (OLDDATE REPLACE`
   3. `ACCESS 19E Y/S`
7. Upload `brexxsrc.vmarc` to VM in binary, fixed format, record length 80 as
   `BREXXSRC VMARC`.
8. Extract the archive of `HELP` files: `VMARC UNPK BREXXSRC VMARC A BRXHELP VMARC A (OLDDATE`.
9. Extract the `HELP` files: `VMARC UNPK BRXHELP VMARC A * * A (OLDDATE`.
10. Move the `* HELPREXX` files to the `HELP` disk (typically `MAINT 19D`).
11. Merge the `REXX HELPTASK` file into the `HELP HELPTASK` file on the `HELP`
    disk.
12. Re-save the CMS saved system to update the shared Y-stat:
   1. `DEFINE STORAGE 16M`
   2. `IPL 190 CLEAR`
   3. `SAVESYS CMS`
   4. Results: `SYSTEM SAVED`.

## Install pre-built via AWSTAPE.

1. Download the pre-built release `BREXX.zip` file from the location above to
   the machine where you run Hercules.
2. Unzip `BREXX.zip`.
3. Log on to `MAINT` on VM.
4. At the Hercules console, attach the binary tape:
   1. `devinit 480` _unzip_dir_`/brexxbin.aws`.
   2. `/ATTACH 480 TO MAINT AS 181`
5. At the Hercules console, attach the source tape:
   1. `devinit 481` _unzip_dir_`/brexxsrc.aws`.
   2. `/ATTACH 481 TO MAINT AS 182`
6. Load the pre-built bREXX file: `TAPE LOAD BREXX TEXT A`.
7. Move the `BREXX TEXT` file to the Y-disk:
   1. `ACCESS 19E Y`
   2. `COPY BREXX TEXT A = = Y (OLDDATE REPLACE`
   3. `ACCESS 19E Y/S`
8. Load the archive of `HELP` files: `TAPE LOAD BRXHELP VMARC A (TAP2`.
9. Detach the tape drives: `DETACH 181-182`.
10. Extract the `HELP` files: `VMARC UNPK BRXHELP VMARC A * * A (OLDDATE`.
11. Move the `* HELPREXX` files to the `HELP` disk (typically `MAINT 19D`).
12. Merge the `REXX HELPTASK` file into the `HELP HELPTASK` file on the `HELP`
    disk.
13. Re-save the CMS saved system to update the shared Y-stat:
   1. `DEFINE STORAGE 16M`
   2. `IPL 190 CLEAR`
   3. `SAVESYS CMS`
   4. Results: `SYSTEM SAVED`.

## Install from source via VMARC.

1. Download the pre-built release `BREXX.zip` file from the location above to
   the machine where you run Hercules.
2. Unzip `BREXX.zip`.
3. Log on to `MAINTC` on VM.
4. Upload `brexxbin.vmarc` to `MAINTC 191` in binary, fixed format, record
   length 80 as `BREXXBIN VMARC`.
5. Upload `brexxsrc.vmarc` to `MAINTC 191` in binary, fixed format, record
   length 80 as `BREXXSRC` VMARC`.
6. Extract the loader exec: `VMARC UNPK BREXXSRC VMARC A BRXLOAD EXEC A (OLDDATE`.
7. Run the loader exec: `BRXLOAD VMARC`
8. Compile bREXX from source: `BRXBUILD`
9. Build the bREXX file: `BRXGEN`
10. Send the bREXX deployment EXEC to MAINT:
   1. `SPOOL PUNCH MAINT`
   2. `DISK DUMP NEWBREXX EXEC`
11. Log on to `MAINT` on VM.
12. Install all the bREXX files (including some on the Y-disk):
   1. `DISK LOAD`
   2. `NEWBREXX`
13. Re-save the CMS saved system to update the shared Y-stat:
   1. `DEFINE STORAGE 16M`
   2. `IPL 190 CLEAR`
   3. `SAVESYS CMS`
   4. Results: `SYSTEM SAVED`.

## Install from source via AWSTAPE.

1. Download the pre-built release `BREXX.zip` file from the location above to
   the machine where you run Hercules.
2. Unzip `BREXX.zip`.
3. Log on to `MAINTC` on VM.
4. At the Hercules console, mount and attach the source tape:
   1. `devinit 480` _unzip_dir_`/brexxsrc.aws`.
   2. `/ATTACH 480 TO MAINTC AS 181`
5. Extract the loader exec: `TAPE LOAD BRXLOAD EXEC A`.
6. Rewind the tape: `TAPE REW`.
7. Run the loader exec: `BRXLOAD TAPE`
8. When prompted to, at the Hercules console, mount the binary tape:
   `devinit 480` _unzip_dir_`/brexxbin.aws`.
9. Detach the tape drive: `DETACH 181`.
10. Compile bREXX from source: `BRXBUILD`
11. Build the bREXX file: `BRXGEN`
12. Send the bREXX deployment EXEC to MAINT:
   1. `SPOOL PUNCH MAINT`
   2. `DISK DUMP NEWBREXX EXEC`
12. Log on to `MAINT` on VM.
13. Install all the bREXX files (including some on the Y-disk):
   1. `DISK LOAD`
   2. `NEWBREXX`
14. Re-save the CMS saved system to update the shared Y-stat:
   1. `DEFINE STORAGE 16M`
   2. `IPL 190 CLEAR`
   3. `SAVESYS CMS`
   4. Results: `SYSTEM SAVED`.

# License

Vasilis Vlachoudis's
[original license for bREXX](ftp://ftp.gwdg.de/pub/languages/rexx/brexx/README)
said:

>     How much does it cost
>     ~~~~~~~~~~~~~~~~~~~~~
>       This is a FREEWARE program, as long as it is used for NON COMMERCIAL
>     purpose. But any generous contribution is well accepted :) to help
>     keeping this project alive. It is still FREEWARE if it is included
>     as a part (macro language) for another FREEWARE product.
>
>       For commercial use the registration fee is $50 (for version 2.0
>     and above). Furthermore, if you want to include it as a macro
>     language in one of your shareware/commercial program you have to
>     contact the author Vasilis.Vlachoudis@cern.ch. For more
>     informations please contact me by e-mail on one of the above addresses.

Vlachoudis subsequently re-released it under version 2 of the GNU Public
License (see [vlachoudis/brexx on GitHub](https://github.com/vlachoudis/brexx)).

Several other programmers ported Vlachoudis's GPL'ed version of bREXX to
VM/370, and dedicated their work to the public domain (see [LICENSE](LICENSE)
and [waiverform.txt](waiverform.txt).
