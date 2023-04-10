import cv2
import matplotlib.pyplot as plt
import numpy as np
import sys
import time
import socket
from enum import Enum
from datetime import datetime
import matplotlib.pyplot as plt

# Configuration variables
UDP_IP     = "192.168.1.130"
UDP_PORT   = 1234
IMG_WIDTH  = 640
IMG_HEIGHT = 480

# class syntax
class IrisMPSoCEnc(Enum):
    CMD_NONE        = 0
    CMD_HISTOGRAM   = 1
    CMD_GET_RESULT  = 2

class IrisMPSoCPkt:
    # Encoding
    # 31----------22----------13-----------0
    # |   dim.x   |   dim.y   |  pkt_type  |
    # |___________|___________|____________|
    def __init__(self, pkt_id=0, pkt_type=IrisMPSoCEnc.CMD_NONE, dim=(639,479)):
        self.pkt_id     = pkt_id
        self.pkt_type   = pkt_type
        self.dim        = dim

    def get_header(self):
        val = (((self.dim[0] & 0x3FF) << 22) | ((self.dim[1] & 0x3FF) << 12) | (self.pkt_type.value & 0xFFF))
        return val.to_bytes(4,'little')

# cap = cv2.VideoCapture(0)
# cap.set(cv2.CAP_PROP_FRAME_WIDTH,IMG_WIDTH)
# cap.set(cv2.CAP_PROP_FRAME_HEIGHT,IMG_HEIGHT)
# cap.set(cv2.CAP_PROP_FPS, 30)
# font = cv2.FONT_HERSHEY_SIMPLEX
# ret, frame = cap.read()
# frame = cv2.flip(frame,1)
# frame = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
# cv2.imwrite('/home/aignacio/projects/mpsoc_sw_master/scripts/image.png', frame)

bins = 256
start_proc = time.time()
frame = cv2.imread('/home/aignacio/projects/mpsoc_sw_master/scripts/image.png')
frame_1ch = frame[:,:,0]  # Convert from 3x ch to 1x ch
frame_array = frame_1ch.ravel()  # Convert from 2D matrix to 1D
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("", UDP_PORT))
numPixels = np.prod(frame.shape[:2])//1024
# cap = cv2.VideoCapture(0)

print("Number of pixels: %d" % (numPixels))
print("Sending image via UDP...")

# fig, ax = plt.subplots()
# ax.set_title('Histogram (grayscale)')
# ax.set_xlabel('Bin')
# ax.set_ylabel('Frequency')
# check, = ax.plot(np.arange(bins), np.zeros((bins,1)), c='k', lw=3, linestyle='dashed')
# # hw,    = ax.plot(np.arange(bins), np.zeros((bins,1)), c='c', lw=3)
# ax.set_xlim(0, bins-1)
# ax.set_ylim(0, 1)
# plt.ion()
# plt.show()

img_cnt=0

# plt.ion()
# fig, ax = plt.subplots()
# fruits = ['apple', 'blueberry', 'cherry', 'orange']
# counts = [40, 100, 30, 55]
# bar_labels = ['red', 'blue', '_red', 'orange']
# bar_colors = ['tab:red', 'tab:blue', 'tab:red', 'tab:orange']
# ax.bar(fruits, counts, label=bar_labels, color=bar_colors)
# ax.set_ylabel('fruit supply')
# ax.set_title('Fruit supply by kind and color')
# ax.legend(title='Fruit color')


while (1):
    # ret, cv_img = cap.read()
    # frame = cv2.cvtColor(cv_img, cv2.COLOR_BGR2GRAY)
    # frame_array = frame.ravel()  # Convert from 2D matrix to 1D

    # histogram_check = cv2.calcHist([frame], [0], None, [bins], [0, bins])
    # histogram_check = histogram_check/histogram_check.max()
    # check.set_ydata(histogram_check)
    # fig.canvas.draw()

    data = IrisMPSoCPkt(0,IrisMPSoCEnc.CMD_HISTOGRAM, (IMG_WIDTH-1,IMG_HEIGHT-1)).get_header()
    sock.sendto(data, (UDP_IP, UDP_PORT))

    rx_eth = sock.recvfrom(4)
    message = rx_eth[0]
    if message != bytes.fromhex("aabbccdd"):
        sys.exit("[ERROR] Issue with received response!")

    for r in range(numPixels):
        # msg = bytes([i%256 for i in range(1*1024)])
        msg = frame_array[r*1024:(r*1024+1024)].tobytes()
        # print("Sending row %d - size %d"%(r,len(msg)))
        sock.sendto(msg, (UDP_IP, UDP_PORT))

        rx_eth = sock.recvfrom(4)
        message = rx_eth[0]
        if message != bytes.fromhex("aabbccdd"):
            sys.exit("[ERROR] Issue with received response!")
        # print("%d"%r)
        #time.sleep(0.001)
        # time.sleep(1)
    print("%s - %d"%(datetime.now(),img_cnt))
    # data = IrisMPSoCPkt(0,IrisMPSoCEnc.CMD_GET_RESULT).get_header()
    # sock.sendto(data, (UDP_IP, UDP_PORT))
    rx_eth = sock.recvfrom(1024)
    message = rx_eth[0]
    img_cnt += 1
