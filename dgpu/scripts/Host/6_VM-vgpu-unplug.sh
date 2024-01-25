#!/bin/bash
if [ $# = 0 ]
then
    echo "Usage: $0 vm_name device_datafile_path"
fi

vmName=$1
deviceXML=$2
vmXMLName=$vmName.xml

virsh dumpxml "$vmName" > "$vmXMLName"
devLine=$(grep -n hostdev "$vmXMLName" | grep -oe "[0-9]\{1,3\}\:")
devARR=("${devLine// / }")
targetLine=${devARR[0]}
beginLine=${targetLine:0:3}
endLine=$(expr "$beginLine" + 7)

sed -n "$beginLine","$endLine"p "$vmXMLName" > "$deviceXML"
#echo "</hostdev>" >> $deviceXML

virsh detach-device "$vmName" "$deviceXML"

