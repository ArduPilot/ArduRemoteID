# ArduPilot RemoteID Transmitter

This is an implementation of a MAVLink and DroneCAN OpenDroneID
transmitter. It aims to provide a transmitter solution for the FAA
standard RemoteID requrement.

## Hardware Supported

So far the only hardware that has been tested is the ESP32-S3. This
devboard is being used:
https://au.mouser.com/ProductDetail/356-ESP32S3DEVKTM1N8

Hardware from https://wurzbachelectronics.com/ and
https://bluemark.io/ is expected to work and will be tested soon.

The pins assumed in this firmware are:

 - UART TX on pin 17
 - UART RX on pin 18
 - CAN TX on pin 47
 - CAN RX on pin 38

For CAN a suitable 1MBit bxCAN transceiver needs to be connected to
the CAN TX/RX pins.

## Protocols

This firmware supports communication with an ArduPilot flight
controller either using MAVLink or DroneCAN.

For MAVLink the following service is used:
https://mavlink.io/en/services/opendroneid.html

For DroneCAN the following messages are used:
https://github.com/dronecan/DSDL/tree/master/dronecan/remoteid

The DroneCAN messages are an exact mirror of the MAVLink messages to
make a dual-transport implementation easy.

## Releases

Pre-built releases are in the releases folder on github

## Flashing

To flash to an ESP32-S3 board use the espressif FlashTool from
https://www.espressif.com/en/support/download/other-tools

and use the pre-built binary in the releases folder then flash using
the following options:

![Flashing with FlashTool](FlashTool.jpg "Flashing")

## ArduPilot Support

A pull-request with support for both MAVLink and DroneCAN is here:

https://github.com/ArduPilot/ardupilot/pull/21075

## Credit

Many thanks to the great work by:

 - OpenDroneID: https://github.com/opendroneid/
 - Steve Jack: https://github.com/sxjack/uav_electronic_ids

This firmware builds on their work.
