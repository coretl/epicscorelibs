#! /bin/sh
# createSoftLinks.sh
# share/src/libUnix $Id$

# Tailored script to create local softlinks releative to this directory
# the name createSoftLinks.sh is a key and must not be changed. If the
# release tool sccsGet finds a file by the name of (createSoftLinks.sh),
# after it has retrieved any out-of-date SCCS files, it invokes the
# script to create the defined softlinks

/bin/rm -f blderrSymTbl
/bin/rm -f errInc.c
/bin/rm -f makeStatTbl

ln -s ../misc/blderrSymTbl blderrSymTbl
ln -s ../misc/errInc.c errInc.c
ln -s ../../script/makeStatTbl makeStatTbl
