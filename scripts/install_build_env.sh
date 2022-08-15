#!/bin/bash

python3 -m pip install empy
python3 -m pip install pymavlink
python3 -m pip install dronecan
wget https://downloads.arduino.cc/arduino-cli/arduino-cli_0.26.0_Linux_64bit.tar.gz
mkdir -p bin
(cd bin && tar xvzf ../arduino-cli_0.26.0_Linux_64bit.tar.gz)
