# Building from Sources under linux

### Step1: get prerequisites

 - sudo apt install arduino
 - pip install pymavlink

### Step2: get code

 - cd ~
 - git clone https://github.com/ardupilot/arduremoteid
 - cd arduremoteid/
 - git submodule init
 - git submodule update --recursive
 - ./scripts/regen_headers.sh
 - ./scripts/add_libraries.sh

### Step3: Open the 'arduino' software in your linux desktop

 - arduino

 - Arduino IDE -> File Menu -> Preferences -> "Additional Board Manager URLs:" cut-n-paste: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
tip:[if you have url/s already isted, you can add it to the end of them with a comma, and then the url.]
 - Arduino IDE -> File Menu -> Preferences -> Ensure 'Sketchbook location:' is set to : /home/xxxx/Arduino for your current user, it defaults to this, but check it.
 - Arduino IDE -> Tools Menu -> Board -> Boards Manager -> [search for 'esp32'] ->Select Version [drop-down]-> 2.0.4 -> Install   [2.0.3 or newer should work]
 - Arduino IDE -> Tools Menu -> Board -> "ESP32 Arduino" ->"ESP32S3 Dev Module" [choose it]-
 - Note that you must select the option with 'S3' in in here.

 - Open 'sketch - ie 'RemoteIDModule.ino 'from this repo
 - Arduino IDE -> File Menu -> Open ...

Plugin your ep32-s3 with usb cable using the port labeled "USB" on the pcb - this is for FLASHING it.

 - Arduino IDE -> Tools Menu -> Port:... -> /dev/ttyACM0
 - Press 'Upload' '"arrow" in IDE green bar.

If board does not flash, hold-down BOOT pushbutton on pcb while pressing RESET pushbutton briefly [to force it into bootloader mode] and retry.
done, ESP32-S3 is now running and emitting test/demo remote-id bluetooth

### Optional

Plugin your ep32-s3 with ANOTHER usb cable using the port labeled
"UART" on the pcb - this is where mavlink and debug is coming/going,
and you can connect to this with mavproxy, etc.

Plugin your ep32-s3 into a flight-controller UART using pins
RX(17)/TX(18)/GND on the pcb - this listens for mavlink from an
autopilot, and expects to find REMOTE_ID* packets in the mavlink
stream, and it broadcast/s this information from the drone as
bluetooth/wifi on 2.4ghz in a manner that can be received by Android
mobile phone App
[https://play.google.com/store/apps/details?id=org.opendroneid.android_osm]
and hopefully other open-drone-id compliant receivers.

Plugin your ep32-s3 into a flight-controller CAN port by wiring a standard CAN Tranciever (such as VP231 or similar) to pins 47(tx),38(rx),GND on the pcb.

Setup/Configuration of ArduPilot/Mavlink/CAN to communicate together is not documented here, please go to ArduPilot wiki for more, eg: https://ardupilot.org/copter/docs/common-remoteid.html
