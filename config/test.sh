#!/usr/bin/csh

setenv dbroot configdb/04/lab2
rm -rf $dbroot

#setenv dbexe pdsapp/config/CfgExpt.py
setenv dbexe build/pdsapp/bin/i386-linux-dbg/configdb

$dbexe --create $dbroot
$dbexe --create-device $dbroot IMS 01000000.07cc0031 01000000.07cc0030
$dbexe --create-device-alias $dbroot IMS CALIB1
$dbexe --copy-device-alias $dbroot IMS CALIB2 CALIB1
$dbexe --import-device-data $dbroot IMS Opal1kConfig configdb/02/lab1/data/opal1k_12b_20g.dat "Opal config BL=12 GN=20"
$dbexe --assign-device-alias $dbroot IMS CALIB1 Opal1kConfig opal1k_12b_20g.dat
$dbexe --create-expt-alias $dbroot GCAL
$dbexe --copy-expt-alias $dbroot GCAL_COPY GCAL
$dbexe --assign-expt-alias $dbroot GCAL IMS CALIB1
$dbexe --update-keys $dbroot
#$dbexe --clear $dbroot

