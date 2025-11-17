#!/bin/sh
# Test installing bREXX

# Exit if there is an error
set -e

# Show the commands
set -x

INST_MODE=$1
VERSION_STRING="$2"
BUILD_DATE="$3"
EXITRC=1

case "$INST_MODE" in
	"dasd" | "bin-tape" | "bin-vmarc" | "src-tape" | "src-vmarc")
		;;
	*)
		echo "Invalid installation mode: $INST_MODE"
		echo "Try 'dasd', 'bin-tape', 'bin-vmarc', 'src-tape', or 'src-vmarc'"
		exit 1
		;;
esac

# IPL
herccontrol "ipl 6a1" -w "USER DSC LOGOFF AS AUTOLOG1"
herccontrol "/CP START 00C" -w "RDR"
herccontrol "/CP PURGE CMSUSER RDR"
herccontrol "/CP PURGE MAINT RDR"
herccontrol "/CP PURGE MAINTC RDR"
herccontrol "/CP DISC" -w "^VM/370 Online"

# Install as per README.md section "Install pre-built via AWSTAPE"
if [ "$INST_MODE" = "bin-tape" ] ; then
	# 1. Download the pre-built release `BREXX.zip` file ...
	# ... done in GitHub workflow
	# 2. Unzip `BREXX.zip`.
	# ... done in GitHub workflow
	# 3. Log on to `MAINT` on VM.
	herccontrol "/LOGON MAINT CPCMS" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"
	# 4. At the Hercules console, attach the binary tape:
	cp BREXX/brexxbin.aws ..
	herccontrol "devinit 480 brexxbin.aws" -w "HHCPN098I"
	herccontrol "/ATTACH 480 TO * AS 181" -w "TAPE 480 ATTACH TO MAINT    181"
	# 5. At the Hercules console, attach the source tape:
	cp BREXX/brexxsrc.aws ..
	herccontrol "devinit 481 brexxsrc.aws" -w "HHCPN098I"
	herccontrol "/ATTACH 481 TO * AS 182" -w "TAPE 481 ATTACH TO MAINT    182"
	# 6. Load the pre-built bREXX file:
	herccontrol "/TAPE LOAD BREXX TEXT A" -w "^Ready;"
	# 7. Move the `BREXX TEXT` file to the Y-disk:
	herccontrol "/ACCESS 19E Y" -w "^Ready;"
	herccontrol "/COPYFILE BREXX TEXT A = = Y2 (OLDDATE REPLACE TYPE" -w "^Ready;"
	herccontrol "/ACCESS 19E Y/S" -w "^Ready;"
	herccontrol "/ERASE BREXX TEXT A" -w "^Ready;"
	# 8. Load the archive of `HELP` files:
	herccontrol "/TAPE LOAD BRXHELP VMARC A (TAP2" -w "^Ready;"
	# 9. Detach the tape drives:
	herccontrol "/DETACH 181-182" -w "^Ready;"
	# 10. Extract the `HELP` files:
	# 11. Move the `* HELPREXX` files to the `HELP` disk
	herccontrol "/ACCESS 19D U" -w "^Ready;"
	herccontrol "/VMARC UNPK BRXHELP VMARC A * * U (OLDDATE REPLACE" -w "^Ready;"
	herccontrol "/ERASE BRXHELP VMARC A" -w "^Ready;"
	# 12. Merge the `REXX HELPTASK` file
	herccontrol "/COPYFILE REXX HELPTASK U HELP = = (APPEND" -w "^Ready;"
	herccontrol "/ACCESS 19D U/S" -w "^Ready;"
	# 13. Re-save the CMS saved system
	herccontrol "/DEFINE STORAGE 16M" -w "CP ENTERED"
	herccontrol "/IPL 190 CLEAR" -w "^VM Community Edition"
	herccontrol "/SAVESYS CMS" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"

	# Prepare to test installation
	herccontrol "devinit 481 brexxsrc.aws" -w "HHCPN098I"
	herccontrol "/ATTACH 481 TO * AS 182" -w "TAPE 481 ATTACH TO MAINT    182"
	herccontrol "/TAPE REW (TAP2"  -w "^Ready;"
	herccontrol "/TAPE LOAD * EXEC A (TAP2"  -w "^Ready;"
	herccontrol "/TAPE REW (TAP2"  -w "^Ready;"
	herccontrol "/TAPE LOAD CMSEXCM ASSEMBLE A (TAP2"  -w "^Ready;"
	herccontrol "/DETACH 182"  -w "^Ready;"
	herccontrol "/SPOOL PUNCH CMSUSER CONT" -w "^Ready;"
	herccontrol "/EXPLOIT (BRXTESTS) DISK DUMP &FN &FT A" -w "^Ready;"
	herccontrol "/LOGOFF" -w "^VM/370 Online"
fi

# Install as per README.md section "Install pre-built via VMARC"
if [ "$INST_MODE" = "bin-vmarc" ] ; then
	herccontrol "devinit 00c localhost:3505 sockdev ebcdic eof"
	# 1. Download the pre-built release `BREXX.zip` file ...
	# ... done in GitHub workflow
	# 2. Unzip `BREXX.zip`.
	# ... done in GitHub workflow
	# 3. Log on to `MAINT` on VM.
	herccontrol "/LOGON MAINT CPCMS" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"
	# 4. Upload `brexxbin.vmarc` to VM
	herccontrol -m >tmp; read MARKER <tmp; rm tmp
#            12345678901234567890123456789012345678901234567890123456789012345678901234567890
	echo -n "USERID MAINT                                                                    " | iconv -f cp819 -t cp1047 > tmp
	echo -n ":READ BREXXBIN VMARC                                                            " | iconv -f cp819 -t cp1047 >> tmp
	cat BREXX/brexxbin.vmarc >> tmp
	netcat -q 0 localhost 3505 < tmp
	rm tmp
	herccontrol "/" -w "RDR FILE"
	herccontrol "/READCARD * * A" -w "^Ready;"
	# 5. Extract the pre-built bREXX file:
	herccontrol "/VMARC UNPK BREXXBIN VMARC A BREXX TEXT A (OLDDATE REPLACE" -w "^Ready"	# Might get RC=8, sort of OK.
	# 6. Move the `BREXX TEXT` file to the Y-disk:
	herccontrol "/ACCESS 19E Y" -w "^Ready;"
	herccontrol "/COPYFILE BREXX TEXT A = = Y2 (OLDDATE REPLACE TYPE" -w "^Ready;"
	herccontrol "/ACCESS 19E Y/S" -w "^Ready;"
	# 7. Upload `brexxsrc.vmarc` to VM
	herccontrol -m >tmp; read MARKER <tmp; rm tmp
#            12345678901234567890123456789012345678901234567890123456789012345678901234567890
	echo -n "USERID MAINT NAME BREXXSRC VMARC                                                " | iconv -f cp819 -t cp1047 > tmp
	echo -n ":READ BREXXSRC VMARC                                                            " | iconv -f cp819 -t cp1047 >> tmp
	cat BREXX/brexxsrc.vmarc >> tmp
	netcat -q 0 localhost 3505 < tmp
	rm tmp
	herccontrol -w "HHCRD012I" -f $MARKER	# Wait for reader to process file
	herccontrol "/" -w "RDR FILE"
	herccontrol "/READCARD * * A" -w "^Ready;"
	# 8. Extract the archive of `HELP` files:
	herccontrol "/VMARC UNPK BREXXSRC VMARC A BRXHELP VMARC A (OLDDATE REPLACE" -w "^Ready;"
	# 9. Extract the `HELP` files:
	# 10. Move the `* HELPREXX` files to the `HELP` disk
	herccontrol "/ACCESS 19D U" -w "^Ready;"
	herccontrol "/VMARC UNPK BRXHELP VMARC A * * U (OLDDATE REPLACE" -w "^Ready;"
	# 11. Merge the `REXX HELPTASK` file
	herccontrol "/COPYFILE REXX HELPTASK U HELP = = (APPEND" -w "^Ready;"
	herccontrol "/ACCESS 19D U/S" -w "^Ready;"
	# 12. Re-save the CMS saved system
	herccontrol "/DEFINE STORAGE 16M" -w "CP ENTERED"
	herccontrol "/IPL 190 CLEAR" -w "^VM Community Edition"
	herccontrol "/SAVESYS CMS" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"

	# Prepare to test installation
	herccontrol "/VMARC UNPK BREXXSRC VMARC A * EXEC A (OLDDATE"  -w "^Ready;"
	herccontrol "/VMARC UNPK BREXXSRC VMARC A CMSEXCM ASSEMBLE A (OLDDATE"  -w "^Ready;"
	herccontrol "/SPOOL PUNCH CMSUSER CONT" -w "^Ready;"
	herccontrol "/EXPLOIT (BRXTESTS) DISK DUMP &FN &FT A" -w "^Ready;"
	herccontrol "/LOGOFF" -w "^VM/370 Online"
fi

# Install as per README.md section "Install pre-built via the GCCBRX DASD volume"
if [ "$INST_MODE" = "dasd" ] ; then
	# 1. Download the pre-built release `BREXX.zip` file ...
	# ... done in GitHub workflow
	# 2. Unzip `BREXX.zip`.
	# ... done in GitHub workflow
	# 3. Copy the `gccbrx.cckd` file ...
	# ... done pre-IPL
	# 3. Copy the `gccbrx.cckd` file ...
	herccontrol "/LOGON OPERATOR OPERATOR" -w "RECONNECTED AT"
	herccontrol "/DETACH 9F0 SYSTEM" -w "DASD 9F0 DETACHED"
	herccontrol "/VARY OFF 9F0" -w "9F0 VARIED OFFLINE"
	herccontrol "detach 09F0"
	cp BREXX/gccbrx.cckd ..
	herccontrol "attach 09F0 3350 gccbrx.cckd"
	herccontrol "/VARY ON 9F0" -w "9F0 VARIED ONLINE"
	herccontrol "/ATTACH 9F0 SYSTEM GCCBRX" -w "DASD 9F0 ATTACH TO SYSTEM GCCBRX"
	herccontrol "/CP DISC" -w "^VM/370 Online"
	# 4. Log on to `MAINT` on VM.
	herccontrol "/LOGON MAINT CPCMS" -w "^VM Community Edition"
	# 5. Upload the `maintc.direct` file ...
	# ... skipped, as VM/CE 1.1.2 has the MAINTC directory entry already.
	# 6. Upload the `newbrexx.exec` file ...
	herccontrol -m >tmp; read MARKER <tmp; rm tmp
	echo "USERID MAINT NAME NEWBREXX EXEC" > tmp
	echo ":READ NEWBREXX EXEC" >> tmp
	cat BREXX/newbrexx.exec >> tmp
	netcat -q 0 localhost 3505 < tmp
	rm tmp
	herccontrol -w "HHCRD012I" -f $MARKER	# Wait for reader to process file
	herccontrol "/" -w "RDR FILE"
	herccontrol "/ACCESS 5E5 B" -w "^Ready;"
	herccontrol "/READCARD * * B" -w "^Ready;"
	herccontrol "/ACCESS 5E5 B/B" -w "^Ready;"
	# 7. Re-save the `GCCLIB` saved segment
	herccontrol "/DEFINE STORAGE 16M" -w "CP ENTERED"
	herccontrol "/IPL CMS" -w "^VM Community Edition"
	herccontrol "/ACCESS (NOPROF" -w "^Ready;"
	herccontrol "/GCCSEG F20000 GCCLIB" -w "^Ready;"
	# 8. Install all the bREXX files ...
	herccontrol "/IPL CMS" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"
	herccontrol "/NEWBREXX" -w "^Ready;"
	# 9. Re-save the CMS saved system ...
	herccontrol "/IPL 190 CLEAR" -w "^VM Community Edition"
	herccontrol "/SAVESYS CMS" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"
	herccontrol "/CP LOGOFF" -w "^VM/370 Online"

	# Prepare to test installation
	herccontrol "/LOGON MAINTC MAINTC" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"
	herccontrol "/BRXSRCH" -w "^Ready;"
	herccontrol "/SPOOL PUNCH CMSUSER CONT" -w "^Ready;"
	herccontrol "/EXPLOIT (BRXTESTS) DISK DUMP &FN &FT F" -w "^Ready;"
	herccontrol "/LOGOFF" -w "^VM/370 Online"
fi

# Install as per README.md section "Install from source via AWSTAPE"
if [ "$INST_MODE" = "src-tape" ] ; then
	# 1. Download the pre-built release `BREXX.zip` file ...
	# ... done in GitHub workflow
	# 2. Unzip `BREXX.zip`.
	# ... done in GitHub workflow
	# 3. Log on to `MAINTC` on VM.
	herccontrol "/LOGON MAINTC MAINTC" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"
	# 4. At the Hercules console, mount and attach the source tape:
	herccontrol "/DISCONN" -w "^VM/370 Online"
	herccontrol "/LOGON OPERATOR OPERATOR" -w "RECONNECTED AT"
	cp BREXX/brexxsrc.aws ..
	herccontrol "devinit 480 brexxsrc.aws" -w "HHCPN098I"
	herccontrol "/ATTACH 480 TO MAINTC AS 181" -w "TAPE 480 ATTACH TO MAINTC   181"
	herccontrol "/DISCONN" -w "^VM/370 Online"
	herccontrol "/LOGON MAINTC MAINTC" -w "RECONNECTED AT"
	herccontrol "/BEGIN"
	# 5. Extract the loader exec:
	herccontrol "/TAPE LOAD BRXLOAD EXEC A" -w "^Ready;"
	# 6. Rewind the tape:
	herccontrol "/TAPE REW" -w "^Ready;"
	# 7. Run the loader exec:
	herccontrol "/BRXLOAD TAPE" -w "^Mount source tape on 181 and press ENTER"
	herccontrol "/" -w "^Mount binary tape on 181 and press ENTER"
	# 8. When prompted to, at the Hercules console, mount the binary tape:
	cp BREXX/brexxbin.aws ..
	herccontrol "devinit 480 brexxbin.aws" -w "HHCPN098I"
	herccontrol "/" -w "^Ready;"
	# 9. Detach the tape drive:
	herccontrol "/DETACH 181" -w "^Ready;"
	# 10. Compile bREXX from source:
	herccontrol "/BRXBUILD" -w "^Ready;"
	# 11. Build the bREXX file:
	herccontrol "/BRXGEN" -w "^Ready;"
	# 12. Send the bREXX deployment EXEC to MAINT:
	herccontrol "/SPOOL PUNCH MAINT" -w "^Ready;"
	herccontrol "/DISK DUMP NEWBREXX EXEC" -w "^Ready"
	# 12. Log on to `MAINT` on VM.
	herccontrol "/LOGOFF" -w "^VM/370 Online"
	herccontrol "/LOGON MAINT CPCMS" -w "^VM Community Edition"
	# 13. Install all the bREXX files (including some on the Y-disk):
	herccontrol "/DISK LOAD" -w "^Ready"
	herccontrol "/NEWBREXX" -w "^Ready;"
	# 14. Re-save the CMS saved system ...
	herccontrol "/DEFINE STORAGE 16M" -w "CP ENTERED"
	herccontrol "/IPL 190 CLEAR" -w "^VM Community Edition"
	herccontrol "/SAVESYS CMS" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"
	herccontrol "/LOGOFF" -w "^VM/370 Online"

	# Prepare to test installation
	herccontrol "/LOGON MAINTC MAINTC" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"
	herccontrol "/BRXSRCH" -w "^Ready;"
	herccontrol "/SPOOL PUNCH CMSUSER CONT" -w "^Ready;"
	herccontrol "/EXPLOIT (BRXTESTS) DISK DUMP &FN &FT F" -w "^Ready;"
	herccontrol "/LOGOFF" -w "^VM/370 Online"
fi

# Install as per README.md section "Install from source via VMARC"
if [ "$INST_MODE" = "src-vmarc" ] ; then
	herccontrol "devinit 00c localhost:3505 sockdev ebcdic eof"
	# 1. Download the pre-built release `BREXX.zip` file ...
	# ... done in GitHub workflow
	# 2. Unzip `BREXX.zip`.
	# ... done in GitHub workflow
	# 3. Log on to `MAINTC` on VM.
	herccontrol "/LOGON MAINTC MAINTC" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"
	# 4. Upload `brexxbin.vmarc` to VM
	herccontrol -m >tmp; read MARKER <tmp; rm tmp
#            12345678901234567890123456789012345678901234567890123456789012345678901234567890
	echo -n "USERID MAINTC NAME BREXXBIN VMARC                                               " | iconv -f cp819 -t cp1047 > tmp
	echo -n ":READ BREXXBIN VMARC                                                            " | iconv -f cp819 -t cp1047 >> tmp
	cat BREXX/brexxbin.vmarc >> tmp
	netcat -q 0 localhost 3505 < tmp
	rm tmp
	herccontrol "/" -w "RDR FILE"
	herccontrol "/READCARD * * A" -w "^Ready;"
	# 5. Upload `brexxsrc.vmarc` to VM
	herccontrol -m >tmp; read MARKER <tmp; rm tmp
#            12345678901234567890123456789012345678901234567890123456789012345678901234567890
	echo -n "USERID MAINTC NAME BREXXSRC VMARC                                               " | iconv -f cp819 -t cp1047 > tmp
	echo -n ":READ BREXXSRC VMARC                                                            " | iconv -f cp819 -t cp1047 >> tmp
	cat BREXX/brexxsrc.vmarc >> tmp
	netcat -q 0 localhost 3505 < tmp
	rm tmp
	herccontrol "/" -w "RDR FILE"
	herccontrol "/READCARD * * A" -w "^Ready;"
	# 6. Extract the loader exec:
	herccontrol "/VMARC UNPK BREXXSRC VMARC A BRXLOAD EXEC A (OLDDATE REPLACE" -w "^Ready;"
	# 7. Run the loader exec:
	herccontrol "/BRXLOAD VMARC" -w "^Ready;"
	# 8. Compile bREXX from source:
	herccontrol "/BRXBUILD" -w "^Ready;"
	# 9. Build the bREXX file:
	herccontrol "/BRXGEN" -w "^Ready;"
	# 10. Send the bREXX deployment EXEC to MAINT:
	herccontrol "/SPOOL PUNCH MAINT" -w "^Ready;"
	herccontrol "/DISK DUMP NEWBREXX EXEC T" -w "^Ready;"
	# 11. Log on to `MAINT` on VM.
	herccontrol "/LOGOFF" -w "^VM/370 Online"
	herccontrol "/LOGON MAINT CPCMS" -w "^VM Community Edition"
	# 12. Install all the bREXX files (including some on the Y-disk):
	herccontrol "/DISK LOAD" -w "^Ready"
	herccontrol "/NEWBREXX" -w "^Ready;"
	# 13. Re-save the CMS saved system
	herccontrol "/DEFINE STORAGE 16M" -w "CP ENTERED"
	herccontrol "/IPL 190 CLEAR" -w "^VM Community Edition"
	herccontrol "/SAVESYS CMS" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"
	herccontrol "/LOGOFF" -w "^VM/370 Online"

	# Prepare to test installation
	herccontrol "/LOGON MAINTC MAINTC" -w "^VM Community Edition"
	herccontrol "/" -w "^Ready;"
	herccontrol "/BRXSRCH" -w "^Ready;"
	herccontrol "/SPOOL PUNCH CMSUSER CONT" -w "^Ready;"
	#   Hercules has a very old bug in /cgi-bin/tasks/syslog that makes
	#   herccontrol fail its search for the Ready message.  So we can't do it
	#   normally here, which would look like this:
	#      herccontrol "/EXPLOIT (BRXTESTS) DISK DUMP &FN &FT F" -w "^Ready;"
	#   Instead, we start EXPLOIT, which runs very quickly, then sleep for
	#   5 seconds, then re-sync on the LOGOFF.
	#
	#   NOTE: This problem doesn't occur on the other installation tests,
	#   just this one.  It appears to be related to the volume of console log
	#   messages, but it's highly repeatable.
	herccontrol "/EXPLOIT (BRXTESTS) DISK DUMP &FN &FT F"
	sleep 5
	herccontrol "/LOGOFF" -w "^VM/370 Online"
fi

# Test installation
herccontrol "/LOGON CMSUSER CMSUSER" -w "^VM Community Edition"
herccontrol "/" -w "^Ready;"
herccontrol "/DISK LOAD"  -w "^Ready;"
herccontrol "/DMSREX VERSION" -w "^Ready;" \
	| sed -e "/Ready/ d" \
	| sed -n -e "2p" \
	| sed -e "s/^BREXX Version \+/ /" -e "s/ \+\(no\)\?debug$//" \
	| sed -e "s/^ \+\([^ ]*\) */MY_VERSION_STRING=\"\1\"\n /" \
	| sed -e "s/^ \+\([^ ]*\) */MY_REXX_LEVEL=\"\1\"\n /" \
	| sed -e "s/^ \+\(.*\) *$/MY_BUILD_DATE=\"\1\"/" \
	> tmp
. ./tmp
if [ "$VERSION_STRING" != "$MY_VERSION_STRING" ] ; then
	echo Wrong version found: $MY_VERSION_STRING
	exit 1
fi
if [ "$INST_MODE" = "bin-tape" -o "$INST_MODE" = "bin-vmarc" -o "$INST_MODE" = "dasd" ] ; then
	# Date may be different for source installations, so only check for binary and DASD cases.
	if [ "$BUILD_DATE" != "$MY_BUILD_DATE" ] ; then
		echo Wrong build date found: $MY_BUILD_DATE
		exit 1
	fi
fi
herccontrol "/RUNTEST_" -w "^Ready;"
herccontrol "/LOGOFF" -w "^VM/370 Online"
EXITRC=0

# All done
herccontrol "/LOGON OPERATOR OPERATOR" -w "RECONNECTED AT"
herccontrol "/SHUTDOWN" -w "^HHCCP011I"
exit $EXITRC
