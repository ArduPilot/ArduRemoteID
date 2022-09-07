#!/usr/bin/env python3

'''
script to create romfs.h from a set of files

Andrew Tridgell
May 2017
'''

import os, sys, tempfile, gzip

def write_encode(out, s):
    out.write(s.encode())

def embed_file(out, f, idx, embedded_name):
    '''embed one file'''
    try:
        contents = open(f,'rb').read()
    except Exception:
        raise Exception("Failed to embed %s" % f)

    pad = 0
    if embedded_name.endswith("bootloader.bin"):
        # round size to a multiple of 32 bytes for bootloader, this ensures
        # it can be flashed on a STM32H7 chip
        blen = len(contents)
        pad = (32 - (blen % 32)) % 32
        if pad != 0:
            if sys.version_info[0] >= 3:
                contents += bytes([0xff]*pad)
            else:
                for i in range(pad):
                    contents += bytes(chr(0xff))
            print("Padded %u bytes for %s to %u" % (pad, embedded_name, len(contents)))

    write_encode(out, 'static const uint8_t romfs_%u[] = {' % idx)

    outf = tempfile.NamedTemporaryFile()
    # ensure nul termination
    nul = bytearray([0])
    if contents[-1] != nul:
        contents += nul
    outf.write(contents)

    outf.seek(0)
    b = bytearray(outf.read())
    outf.close()

    for c in b:
        write_encode(out, '%u,' % c)
    write_encode(out, '};\n\n');

def create_embedded_h(filename, files):
    '''create a romfs_embedded.h file'''

    out = open(filename, "wb")
    write_encode(out, '''// generated embedded files\n\n''')

    # remove duplicates and sort
    files = sorted(list(set(files)))
    for i in range(len(files)):
        (name, filename) = files[i]
        try:
            embed_file(out, filename, i, name)
        except Exception as e:
            print(e)
            return False

    write_encode(out, '''const ROMFS::embedded_file ROMFS::files[] = {\n''')

    for i in range(len(files)):
        (name, filename) = files[i]
        print("Embedding file %s:%s" % (name, filename))
        write_encode(out, '{ "%s", sizeof(romfs_%u), romfs_%u },\n' % (name, i, i))
    write_encode(out, '};\n')
    out.close()
    return True

if __name__ == '__main__':
    import sys
    flist = []
    if len(sys.argv) < 2:
        print("Usage: make_romfs.py romfs_files.h FILES...")
        sys.exit(1)
    for i in range(2, len(sys.argv)):
        f = sys.argv[i]
        flist.append((f, f))
    create_embedded_h(sys.argv[1], flist)
