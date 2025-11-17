#!/bin/sh
# Make BREXX on CMS

# Exit if there is an error
set -e

# Show the commands
set -x

# Get the latest gccbrx.cckd disk image
herccontrol "detach 09F0"
wget -nv https://github.com/RossPatterson/CMS-370-GCCLIB/releases/download/v1.0.1/GCCLIB.zip
unzip GCCLIB.zip
cp GCCLIB/gccbrx.cckd ..
rm GCCLIB.zip
rm -r GCCLIB
herccontrol "attach 09F0 3350 gccbrx.cckd"

# IPL
herccontrol "ipl 6a1" -w "USER DSC LOGOFF AS AUTOLOG1"
herccontrol "/cp start c" -w "RDR"
herccontrol "/cp start d class a" -w "PUN"

# LOGON MAINTC
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon maintc maintc" -w "^VM Community Edition"
herccontrol "/ACCESS (NOPROF" -w "^Ready;"
herccontrol "/SET LDRTBLS 64" -w "^Ready;"
herccontrol "/PROFILE" -w "^Ready;"
herccontrol "/purge rdr" -w "^Ready;"

herccontrol "/ACCESS 393 B" -w "^Ready;"
herccontrol "/ERASE * * B1" -w "^Ready;"

# Get Source
yata -c -f archive.yata
herccontrol -m >tmp; read mark <tmp; rm tmp
echo "USERID MAINTC\n:READ  ARCHIVE  YATA    " > tmp
cat archive.yata >> tmp
netcat -q 0 localhost 3505 < tmp
rm tmp
herccontrol -w "HHCRD012I" -f $mark
herccontrol "/" -w "RDR FILE"
herccontrol "/yata -x -f READER -d b" -w "^Ready;"

# Get test suite
yata -c -d tests -f archive.yata
herccontrol -m >tmp; read mark <tmp; rm tmp
echo "USERID  MAINTC\n:READ  ARCHIVE  YATA    " > tmp
cat archive.yata >> tmp
netcat -q 0 localhost 3505 < tmp
rm tmp
herccontrol -w "HHCRD012I" -f $mark
herccontrol "/" -w "RDR FILE"
herccontrol "/yata -x -f READER -d b" -w "^Ready;"

# Get test tools
yata -c -d tools -f archive.yata
herccontrol -m >tmp; read mark <tmp; rm tmp
echo "USERID  MAINTC\n:READ  ARCHIVE  YATA    " > tmp
cat archive.yata >> tmp
netcat -q 0 localhost 3505 < tmp
rm tmp
herccontrol -w "HHCRD012I" -f $mark
herccontrol "/" -w "RDR FILE"
herccontrol "/yata -x -f READER -d b" -w "^Ready;"

# Get help files
yata -c -d help -f archive.yata
herccontrol -m >tmp; read mark <tmp; rm tmp
echo "USERID  MAINTC\n:READ  ARCHIVE  YATA    " > tmp
cat archive.yata >> tmp
netcat -q 0 localhost 3505 < tmp
rm tmp
herccontrol -w "HHCRD012I" -f $mark
herccontrol "/" -w "RDR FILE"
herccontrol "/yata -x -f READER -d b" -w "^Ready;"

# Fix Source Files
herccontrol "/COPYFILE * MACRO    B (RECFM F LRECL 80" -w "^Ready"
herccontrol "/COPYFILE * COPY     B (RECFM F LRECL 80" -w "^Ready"
herccontrol "/COPYFILE * ASSEMBLE B (RECFM F LRECL 80" -w "^Ready"
herccontrol "/RENAME NEWBREXX TMPFTYPE B NEWBREXX CONTROL B" -w "^Ready"

# TEMPORARY!  Build GCCCSECT MODULE, until a new VM/CE release ships our version.
herccontrol "/MKGCCCS" -w "^Ready;"

# Make the BRXHELP VMARC file
herccontrol "/EXPLOIT (BRXHELP) VMARC PACK &FN &FT B BRXHELP VMARC B (APPEND NOTRACE" -w "^Ready;"

# Make source tape and vmarc
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon operator operator" -w "RECONNECTED AT"
hetinit -n -d brexxsrc.aws
herccontrol "devinit 480 io/brexxsrc.aws" -w "^HHCPN098I"
herccontrol "/attach 480 to maintc as 181" -w "TAPE 480 ATTACH"
herccontrol "devinit 00d io/brexxsrc.vmarc" -w "^HHCPN098I"
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon maintc maintc" -w "RECONNECTED"
herccontrol "/begin"
herccontrol "/tape dump * * b (noprint" -w "^Ready;"
herccontrol "/detach 181" -w "^Ready;"
herccontrol "/vmarc pack * * b (pun notrace" -w "^Ready;"

# Close and remove extra record from VMARC file
herccontrol "devinit 00d dummy" -w "^HHCPN098I"
truncate -s-80 brexxsrc.vmarc

# Put tools on the T drive
herccontrol "/EXPLOIT (BRXTOOLS) COPYFILE &FN &FT &FM = = T (REPLACE OLDDATE" -w "^Ready"
herccontrol "/COPYFILE BRXTOOLS EXEC B = = A (REPLACE OLDDATE" -w "^Ready"
herccontrol "/EXPLOIT (BRXTOOLS) ERASE &FN &FT &FM" -w "^Ready"
herccontrol "/ERASE BRXTOOLS EXEC B" -w "^Ready"

# Put our deployment control file on the A drive
herccontrol "/COPYFILE NEWBREXX CONTROL B = = A (REPLACE OLDDATE" -w "^Ready"
herccontrol "/ERASE NEWBREXX CONTROL B" -w "^Ready"

# Compile the code
herccontrol "/ipl cms" -w "^VM Community Edition"
herccontrol "/ACCESS (NOPROF" -w "^Ready;"
herccontrol "/SET LDRTBLS 64" -w "^Ready;"
herccontrol "/PROFILE" -w "^Ready;"
herccontrol "/set emsg on" -w "^Ready;"
herccontrol "/BRXBUILD" -w "^Ready;" -t 240

# Make debug binary file
herccontrol "/ipl cms" -w "^VM Community Edition"
herccontrol "/ACCESS (NOPROF" -w "^Ready;"
herccontrol "/SET LDRTBLS 64" -w "^Ready;"
herccontrol "/PROFILE" -w "^Ready;"
herccontrol "/set emsg on" -w "^Ready;"
herccontrol "/BRXSRCHD" -w "^Ready;"
herccontrol "/BRXGEN" -w "^Ready;"
herccontrol "/RENAME BREXX * A BREXXD = =" -w "^Ready;"

# Make normal binary file
herccontrol "/ipl cms" -w "^VM Community Edition"
herccontrol "/ACCESS (NOPROF" -w "^Ready;"
herccontrol "/SET LDRTBLS 64" -w "^Ready;"
herccontrol "/PROFILE" -w "^Ready;"
herccontrol "/set emsg on" -w "^Ready;"
herccontrol "/BRXSRCH" -w "^Ready;"
herccontrol "/BRXGEN" -w "^Ready;"

# Make binary tape and vmarc
herccontrol "/ipl cms" -w "^VM Community Edition"
herccontrol "/ACCESS (NOPROF" -w "^Ready;"
herccontrol "/SET LDRTBLS 64" -w "^Ready;"
herccontrol "/PROFILE" -w "^Ready;"
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon operator operator" -w "RECONNECTED AT"
hetinit -n -d brexxbin.aws
herccontrol "devinit 480 io/brexxbin.aws" -w "^HHCPN098I"
herccontrol "/attach 480 to maintc as 181" -w "TAPE 480 ATTACH"
herccontrol "devinit 00d io/brexxbin.vmarc" -w "^HHCPN098I"
herccontrol "/cp disc" -w "^VM/370 Online"
herccontrol "/logon maintc maintc" -w "RECONNECTED AT"
herccontrol "/begin"
herccontrol "/access 193 e" -w "^Ready"
herccontrol "/copyfile brexx * a = = e" -w "^Ready"
herccontrol "/copyfile brexxd * a = = e" -w "^Ready"
herccontrol "/access 393 f" -w "^Ready"
herccontrol "/copyfile maintc direct f = = e (oldd" -w "^Ready;"
herccontrol "/copyfile newbrexx exec t = = e (oldd" -w "^Ready;"
herccontrol "/copyfile newbrexx control a = = e (oldd" -w "^Ready;"
herccontrol "/tape dump * * e" -w "^Ready"
herccontrol "/tape dump brxhelp vmarc f" -w "^Ready"
herccontrol "/detach 181" -w "^Ready;"
herccontrol "/vmarc pack * * e (pun" -w "^Ready;"
herccontrol "/vmarc pack brxhelp vmarc f (pun" -w "^Ready;"

# Close and remove extra record from VMARC file
herccontrol "devinit 00d dummy" -w "^HHCPN098I"
truncate -s-80 brexxbin.vmarc

# LOGOFF
herccontrol "/logoff" -w "^VM/370 Online"

# REBUILD CMS
herccontrol "/logon maint cpcms" -w "^VM Community Edition"
herccontrol "/" -w "^Ready"
herccontrol "/NEWBREXX" -w "^Ready"
herccontrol "/define storage 16m"  -w "CP ENTERED"
herccontrol "/ipl 190 clear" -w "^VM Community Edition"
herccontrol "/savesys cms" -w "^VM Community Edition"
herccontrol "/" -w "^Ready;"
herccontrol "/logoff" -w "^VM/370 Online"

# Test suite
herccontrol "/logon maintc maintc" -w "^VM Community Edition"
herccontrol "/" -w "^Ready;"
herccontrol "/BRXSRCH" -w "^Ready;"
herccontrol "/runtest_" -w "^Ready;"
herccontrol "/logoff" -w "^VM/370 Online"

# Export version string
# For example:
#	VERSION_STRING=REXX-bREXX-2.1.9-CMS370-1.1.3
#	REXX_LEVEL=3.45
#	BUILD_DATE=Sep  2 2025
# Note: GITHUB_OUTPUT wants raw data (i.e. XXX, not "XXX").
herccontrol "/LOGON MAINT CPCMS" -w "^VM Community Edition"
herccontrol "/" -w "^Ready;"
herccontrol "/DMSREX VERSION" -w "^Ready;" \
	| sed -e "/Ready/ d" \
	| sed -n -e "2p" \
	| sed -e "s/^BREXX Version \+/ /" -e "s/ \+\(no\)\?debug$//" \
	| sed -e "s/^ \+\([^ ]*\) */VERSION_STRING=\1\n /" \
	| sed -e "s/^ \+\([^ ]*\) */REXX_LEVEL=\1\n /" \
	| sed -e "s/^ \+\(.*\) *$/BUILD_DATE=\1/" \
	>> "$GITHUB_OUTPUT"
herccontrol "/LOGOFF" -w "^VM/370 Online"

# SHUTDOWN
herccontrol "/logon operator operator" -w "RECONNECTED AT"
herccontrol "/shutdown" -w "^HHCCP011I"
herccontrol "detach 09F0"
