#!/bin/bash
# setup libraries links in $HOME/Arduino

DEST=~/Arduino/libraries/

mkdir -p "$DEST"
ln -sf $PWD/modules/uav_electronic_ids/id_open "$DEST"
ln -sf $PWD/modules/uav_electronic_ids/utm "$DEST"
ln -sf $PWD/modules/opendroneid-core-c/libopendroneid "$DEST"
ln -sf $PWD/libraries/mavlink2 ~/Arduino/libraries/ "$DEST"
