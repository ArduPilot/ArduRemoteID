#!/bin/bash
cat <<EOF > RemoteIDModule/git-version.h
#define GIT_VERSION 0x$(git describe --always)
EOF



