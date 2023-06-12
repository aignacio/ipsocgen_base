from PyQt5 import QtGui
from PyQt5.QtWidgets import QWidget, QApplication, QLabel, QVBoxLayout
from PyQt5.QtGui import QPixmap
import sys
import cv2
from PyQt5.QtCore import pyqtSignal, pyqtSlot, Qt, QThread
import numpy as np
import socket
import matplotlib.pyplot as plt
from enum import Enum
import time

UDP_IP     = "192.168.1.130"
UDP_PORT   = 1234
IMG_WIDTH  = 640
IMG_HEIGHT = 480

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

class VideoThread(QThread):
    change_pixmap_signal = pyqtSignal(np.ndarray)

    def __init__(self):
        super().__init__()
        self._run_flag = True

    def run(self):
        # capture from web cam
        cap = cv2.VideoCapture(0)
        while self._run_flag:
            ret, cv_img = cap.read()

            if ret:
                self.change_pixmap_signal.emit(cv_img)
            #time.sleep(0.5)
        # shut down capture system
        cap.release()

    def stop(self):
        """Sets run flag to False and waits for thread to finish"""
        self._run_flag = False
        self.wait()

class App(QWidget):
    bins = 256
    fig, ax = plt.subplots()
    ax.set_title('Histogram (grayscale)')
    ax.set_xlabel('Bin')
    ax.set_ylabel('Frequency')
    g1, = ax.plot(np.arange(bins), np.zeros((bins,1)), c='r', lw=3)
    g2, = ax.plot(np.arange(bins), np.zeros((bins,1)), c='c', lw=3, ls='dashed')

    ax.set_xlim(0, bins-1)
    ax.set_ylim(0, 1)
    plt.ion()
    plt.show()

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Webcam image capture by aignacio")
        self.disply_width = IMG_WIDTH
        self.display_height = IMG_HEIGHT
        # create the label that holds the image
        self.image_label = QLabel(self)
        self.image_label.resize(self.disply_width, self.display_height)
        # create a text label
        self.textLabel = QLabel('Webcam')

        # create a vertical box layout and add the two labels
        vbox = QVBoxLayout()
        vbox.addWidget(self.image_label)
        vbox.addWidget(self.textLabel)
        # set the vbox layout as the widgets layout
        self.setLayout(vbox)

        # Start the UDP socket
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind(("", UDP_PORT))

        # create the video capture thread
        self.thread = VideoThread()
        # connect its signal to the update_image slot
        self.thread.change_pixmap_signal.connect(self.update_image)
        self.thread.change_pixmap_signal.connect(self.update_hist)

        # start the thread
        self.thread.start()

    def closeEvent(self, event):
        self.thread.stop()
        event.accept()

    @pyqtSlot(np.ndarray)
    def update_hist(self, cv_img):
        frame = cv2.cvtColor(cv_img, cv2.COLOR_BGR2GRAY)
        # frame_1ch = frame[:,:,0]  # Need when using imread -> Convert from 3x ch to 1x ch
        frame_array = frame.ravel()  # Convert from 2D matrix to 1D

        histogram_check = cv2.calcHist([frame], [0], None, [self.bins], [0, self.bins])
        histogram_check = histogram_check/histogram_check.max()

        data = IrisMPSoCPkt(0,IrisMPSoCEnc.CMD_HISTOGRAM, (IMG_WIDTH-1,IMG_HEIGHT-1)).get_header()
        self.sock.sendto(data, (UDP_IP, UDP_PORT))
        rx_eth = self.sock.recvfrom(4)
        message = rx_eth[0]
        if message != bytes.fromhex("aabbccdd"):
            sys.exit("[ERROR] Issue with received response!")

        for r in range((IMG_WIDTH*IMG_HEIGHT)//1024):
            msg = frame_array[r*1024:(r*1024+1024)].tobytes()
            self.sock.sendto(msg, (UDP_IP, UDP_PORT))

            rx_eth = self.sock.recvfrom(4)
            message = rx_eth[0]
            if message != bytes.fromhex("aabbccdd"):
                sys.exit("[ERROR] Issue with received response!")
        rx_eth = self.sock.recvfrom(1024)
        histogram = rx_eth[0]

        hist_array = np.array(np.frombuffer(histogram, dtype=np.uint32));
        hist_array = hist_array/hist_array.max()

        self.g2.set_ydata(histogram_check)
        self.g1.set_ydata(hist_array)
        self.fig.canvas.draw()

    @pyqtSlot(np.ndarray)
    def update_image(self, cv_img):
        """Updates the image_label with a new opencv image"""
        qt_img = self.convert_cv_qt(cv_img)
        self.image_label.setPixmap(qt_img)

    def convert_cv_qt(self, cv_img):
        """Convert from an opencv image to QPixmap"""
        rgb_image = cv2.cvtColor(cv_img, cv2.COLOR_BGR2RGB)
        h, w, ch = rgb_image.shape
        bytes_per_line = ch * w
        convert_to_Qt_format = QtGui.QImage(rgb_image.data, w, h, bytes_per_line, QtGui.QImage.Format_RGB888)
        p = convert_to_Qt_format.scaled(self.disply_width, self.display_height, Qt.KeepAspectRatio)
        return QPixmap.fromImage(p)

if __name__=="__main__":
    app = QApplication(sys.argv)
    a = App()
    a.show()
    sys.exit(app.exec_())
