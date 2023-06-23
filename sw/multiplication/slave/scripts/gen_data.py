#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# File              : gen_data.py
# License           : MIT license <Check LICENSE>
# Author            : Anderson Ignacio da Silva (aignacio) <anderson@aignacio.com>
# Date              : 15.12.2022
# Last Modified Date: 15.12.2022
# Description       : Script to generate a pseudo image for the MPSoC - @aignacio,
#                     We breakdown the image into XxY chunks. Considering that the
#                     first segment [0] will be processed by the master tile, we
#                     generate NoC packets starting from the second segment [byte 1024].
import random

NoC_size_x  = 4
NoC_size_y  = 4
num_tiles   = NoC_size_x*NoC_size_y
histogram   = [0 for x in range(256)]
pkt_size    = 20 # Need to be multiple of 4x bytes

def gen_pkt(x, y):
    pkt = (x << 30) | (y << 28) | (0xFF << 20)
    return pkt.to_bytes(4, 'little')

def main():
    x = 0
    y = 0
    h_file = open("pseudo_image.h", "w")
    h_file.write("#ifndef PSEUDO_IMAGE_H")
    h_file.write("\n#define PSEUDO_IMAGE_H")
    h_file.write("\n\n#include <stdint.h>")
    h_file.write("\n\n__attribute__((section(\".image\"))) uint8_t pseudo_image ["+str(num_tiles)+"]["+str(pkt_size)+"] = {")
    for slave_id in range(0,num_tiles):
        pkt = gen_pkt(x,y)

        h_file.write("\n\t{"+hex(pkt[0])+", "+hex(pkt[1])+", "+hex(pkt[2])+", "+hex(pkt[3])+", ")
        for pixel_idx in range(4,pkt_size):
            val = random.randint(0,255)
            if pixel_idx%(pkt_size-1) == 0:
                h_file.write(str(val))
            else:
                h_file.write(str(val)+", ")
            if ((pixel_idx % 20) == 0):
                h_file.write("\n\t")
            histogram[val] += 1
        if slave_id != num_tiles-1:
            h_file.write("},")
        else:
            h_file.write("}")

        print("[Segment %d] \tx=%d / y=%d - Packet: 0x%s" % (slave_id, x, y,''.join('{:02x}'.format(x) for x in reversed(pkt))))
        y += 1
        if y == NoC_size_y:
            y = 0
            x += 1
            if x == NoC_size_x:
                x = 0

    h_file.write("\n};")

    h_file.write("\n\nuint8_t histogram_check [256] = {\n\t")
    for idx, i in enumerate(histogram):
        if idx == 255:
            h_file.write(str(i))
        else:
            h_file.write(str(i)+", ")

            if (((idx+1)%8) == 0):
                h_file.write("\n\t")

    h_file.write("\n};")
    h_file.write("\n\n#endif")
    h_file.close()

if __name__ == "__main__":
    main()
