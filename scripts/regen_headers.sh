#!/bin/bash
# re-generate mavlink headers, assumes pymavlink is installed

echo "Generating mavlink2 headers"
rm -rf libraries/mavlink2/generated
mavgen.py --wire-protocol 2.0 --lang C modules/mavlink/message_definitions/v1.0/all.xml -o libraries/mavlink2/generated

echo "Generating DroneCAN headers for libcanard"
rm -rf libraries/DroneCAN_generated
python3 modules/dronecan_dsdlc/dronecan_dsdlc.py -O libraries/DroneCAN_generated modules/DSDL/uavcan modules/DSDL/dronecan modules/DSDL/com

# cope with horrible Arduino library handling
PACKETS="NodeStatus GetNodeInfo HardwareVersion SoftwareVersion RestartNode dynamic_node_id remoteid"
for p in $PACKETS; do
    (
        cd libraries/DroneCAN_generated
        ln -s include/*"$p"*.h .
        ln -s src/*"$p"*.c .
    )
done
