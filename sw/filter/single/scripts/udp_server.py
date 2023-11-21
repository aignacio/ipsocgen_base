import socket
from datetime import datetime

num_pkt     = 0
localIP     = "127.0.0.1"
localPort   = 1234
bufferSize  = 1024
test = 0
msgFromServer = "Hello Dragon {0:3d}".format(test)
bytesToSend   = str.encode(msgFromServer)
# Create a datagram socket
UDPServerSocket = socket.socket(family=socket.AF_INET, type=socket.SOCK_DGRAM)
# Bind to address and ip
UDPServerSocket.bind(("", localPort))
print("UDP server up and listening")

# Listen for incoming datagrams
while(True):
    if test < 1000:
        test = test+1
    else:
        test = 0
    msgFromServer = "Hello Dragon {0:3d}".format(test)
    bytesToSend   = str.encode(msgFromServer)
    dt = datetime.now()
    bytesAddressPair = UDPServerSocket.recvfrom(bufferSize)
    message = bytesAddressPair[0]
    address = bytesAddressPair[1]
    print("[%s] Packet Counter %d" % (dt, num_pkt))
    clientMsg = "[{}] Message from Client:{}".format(dt, message)
    clientIP  = "[{}] Client IP Address:{}".format(dt, address)
    print(clientMsg)
    print(clientIP)
    print("Sending message in return %s"%msgFromServer)
    # Sending a reply to client
    UDPServerSocket.sendto(bytesToSend, address)
    num_pkt += 1
