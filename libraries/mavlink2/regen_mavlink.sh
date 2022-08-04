#!/bin/bash
# re-generate mavlink headers, assumes pymavlink is installed

mavgen.py --wire-protocol 2.0 --lang C ../../modules/mavlink/message_definitions/v1.0/all.xml -o generated


