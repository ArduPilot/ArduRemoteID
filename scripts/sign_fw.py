#!/usr/bin/env python3
'''
sign an OTA bin
'''

import sys
import struct
import base64

try:
    import monocypher
except ImportError:
    print("Please install monocypher with: python3 -m pip install pymonocypher")
    sys.exit(1)

key_len = 32
sig_len = 64
descriptor = b'\x43\x2a\xf1\x37\x46\xe2\x75\x19'

if len(sys.argv) < 4:
    print("Usage: sign_fw OTA_FILE PRIVATE_KEYFILE BOARD_ID")
    sys.exit(1)

ota_file = sys.argv[1]
key_file = sys.argv[2]
board_id = int(sys.argv[3])

img = open(ota_file,'rb').read()
img_len = len(img)

def decode_key(ktype, key):
    ktype += "_KEYV1:"
    if not key.startswith(ktype):
        print("Invalid key type")
        sys.exit(1)
    return base64.b64decode(key[len(ktype):])

key = decode_key("PRIVATE", open(key_file, 'r').read())
if len(key) != key_len:
    print("Bad key length %u" % len(key))
    sys.exit(1)

desc_len = 80
ad_start = len(img)-desc_len
if img[ad_start:ad_start+8] == descriptor:
    print("Image is already signed")
    sys.exit(1)

signature = monocypher.signature_sign(key, img)
if len(signature) != sig_len:
    print("Bad signature length %u should be %u" % (len(signature), sig_len))
    sys.exit(1)

desc = struct.pack("<II64s", board_id, img_len, signature)
img = img + descriptor + desc

if len(img) != img_len + desc_len:
    print("Error: incorrect image length")
    sys.exit(1)

print("Applying signature")

open(ota_file, "wb").write(img)
print("Wrote %s" % ota_file)
