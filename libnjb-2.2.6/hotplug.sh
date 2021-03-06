#!/bin/sh

INSTALL="/usr/bin/install -c"
HOTPLUGPATH=/etc/hotplug
SCRIPTNAME=nomadjukebox
USERMAP=nomad.usermap

# See if the parameter script ($2), device ($3) and productid ($4)
# are already defined in the usermap ($1)
function inmap {
    while read LINE; do
	if [ "x${LINE}" != "x" ]; then
	    firstword=`echo ${LINE} | awk '{ print $1 }'`
	    if [ ${firstword} != "#" ]; then
	        # This is a device line entry
		script=${firstword}
		manid=`echo ${LINE} | awk '{ print $3 }'`
		productid=`echo ${LINE} | awk '{ print $4 }'`
		# Skip blank products...
		if [ "x${script}" = "x$2" ]; then 
		    if [ "x${manid}" = "x$3" ]; then 
			if [ "x${productid}" = "x$4" ]; then
			    echo "yes"
			    return 0
			fi
		    fi
		fi
	    fi
	fi
    done < $1
    echo "no"
    return 0
}

# Scan the usermap $2 for all devices in $1 to see if they are already
# there, else patch the usermap.
function patchusermap {
    # Nullify comment
    comment=""
    while read LINE; do
	if [ "x$LINE" != "x" ]; then
	    firstword=`echo ${LINE} | awk '{ print $1 }'`
	    if [ ${firstword} = "#" ]; then
	    # This is a comment line, save it.
		comment=${LINE}
	    else
	        # This is a device line entry
		script=${firstword}
		manid=`echo ${LINE} | awk '{ print $3 }'`
		productid=`echo ${LINE} | awk '{ print $4 }'`
		# Skip blank products...
		if [ "x${manid}" != "x" ]; then
	            # See if this product is already in the usermap
		    echo "Checking for product ${productid} in $2..."
		    if [ `inmap $2 ${script} ${manid} ${productid}` = "no" ]; then
			echo "Not present, adding to hotplug map."
			echo ${comment} >> $2
			echo ${LINE} >> $2
			comment=""
		    else
			echo "Already installed."
		    fi
		fi
	    fi
	fi
    done < $1
}

# Check prerequisites
COMMAND=`which grep 2> /dev/null`
if [ "x${COMMAND}" = "x" ];
then
    echo "Missing grep program. Fatal error."
    exit 1
fi
COMMAND=`which awk 2> /dev/null`
if [ "x${COMMAND}" = "x" ];
then
    echo "Missing awk program. Fatal error."
    exit 1
fi

# This script locates the hotplug distribution on a certain host
# and sets up userland hotplugging scripts according to rules. 
# The in-parameters are the hotplug directory and the name of a
# file of hotplug device entries and a script to be executed for 
# these deviced.

if test -d ${HOTPLUGPATH}
then
    echo "Hotplug in ${HOTPLUGPATH}"
else
    echo "Hotplug missing on this system. Cannot install."
    exit 1
fi

if test -d ${HOTPLUGPATH}/usb 
then
    echo "Has usb subdirectory."
else
    mkdir ${HOTPLUGPATH}/usb
fi

echo "Installing script."
${INSTALL} nomadjukebox ${HOTPLUGPATH}/usb
echo "Installing usermap."
${INSTALL} -m 644 ${USERMAP} ${HOTPLUGPATH}/usb
# If we find a usb.usermap file, and we see that this distribution
# of hotplug does not support private usermaps, then we need to
# patch the usb.usermap file.
#
# Create a merged file, diff the files to each other, and if there
# were mismatches, then patch the installed file.
echo "Checking hotplugging CVS version..."
echo "/etc/hotplug/usb/*.usermap support was added in august 2002"
EDITMAP="yes"
CVSTAG=`grep '\$Id:' /etc/hotplug/usb.agent`
if [ "x${CVSTAG}" != "x" ]; then
    DATE=`echo ${CVSTAG} | awk '{ print $5 }'`
    echo "Hotplug version seems to be from ${DATE}"
    YEAR=`echo ${DATE} | awk 'BEGIN { FS="/"} {print $1; }'`
    MONTH=`echo ${DATE} | awk 'BEGIN { FS="/"} {print $2; }'`
    DAY=`echo ${DATE} | awk 'BEGIN { FS="/"} {print $3; }'`
    if [ "${YEAR}" -gt "2002" ]; then
	EDITMAP="no"
    else
	if [ "${YEAR}" -eq "2002" ]; then
	    if [ "${MONTH}" -ge "08" ]; then
		EDITMAP="no"
	    fi
	fi
    fi
fi
if [ "x${EDITMAP}" == "xyes" ]; then
    echo "We need to edit the ${HOTPLUGPATH}/usb.usermap if it exists..."
    if test -f ${HOTPLUGPATH}/usb.usermap
    then
	echo "We have a usb.usermap..."
	patchusermap ${USERMAP} /etc/hotplug/usb.usermap
    fi
fi

echo "Hotplugging successfully installed."

exit 0

