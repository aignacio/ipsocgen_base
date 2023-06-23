#include <iostream>
#include <thread>
#include <vector>

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs/imgcodecs.hpp>
#include <opencv2/plot.hpp>

#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <chrono>

#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>

#define FPGA_IP         "192.168.1.130"
#define FPGA_PORT       1234
#define PACKET_SIZE     1024

struct sockaddr_in serverAddress;
struct sockaddr_in localAddress;
int    sockfd;

using namespace std;
using namespace cv;

typedef enum {
  CMD_NONE        = 0,
  CMD_RECV_ARRAY  = 1,
  CMD_MULT_FACTOR = 2,
  CMD_GET_RESULT  = 3
} CmdType_t;

typedef uint32_t arg_t;

typedef union {
  uint32_t word;
  struct {
    CmdType_t pkt_type:2;
    arg_t     arg1:15; // Array size in KiB
    arg_t     arg2:15; // Factor
  } st;
} Cmd_t;

double measureElapsedTime(bool reset = false) {
  static auto startTime = chrono::high_resolution_clock::now();

  auto endTime = chrono::high_resolution_clock::now();
  auto elapsedTime = chrono::duration<double, milli>(endTime - startTime).count();

  if (reset) {
    startTime = chrono::high_resolution_clock::now();
  }

  return elapsedTime;
}

void sendUDP (uint32_t payload);

void streamCmd (void) {
  using namespace std::this_thread; // sleep_for, sleep_until
  using namespace std::chrono; // nanoseconds, system_clock, seconds

  while(true) {
    Cmd_t test;

    test.st.pkt_type = CMD_MULT_FACTOR;
    test.st.arg1 = 22;
    test.st.arg2 = 13;
    sendUDP(test.word);

    sleep_for(seconds(1));
  }
}

int main(void) {
  // Create socket
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    std::cerr << "Error creating socket." << std::endl;
    return 1;
  }
  
  // Configure server address
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = inet_addr(FPGA_IP);
  serverAddress.sin_port = htons(1234);
  
  // Bind socket to local port 1234
  localAddress.sin_family = AF_INET;
  localAddress.sin_addr.s_addr = INADDR_ANY;
  localAddress.sin_port = htons(1234);
  bind(sockfd, (struct sockaddr *)&localAddress, sizeof(localAddress));
  
  // Create two threads for video streaming and histogram plotting
  thread streamThread(streamCmd);
  
  // Wait for the threads to finish
  streamThread.join();
  
  return 0;
}

// ------------------- AUX
void sendUDP (uint32_t data) {
  const char* ackFPGA = "\xaa\xbb\xcc\xdd";  // Hexadecimal value to compare against
  uint32_t payload = data;//htonl(data);
  struct sockaddr_in clientAddress;

  // Send
  sendto(sockfd, &payload, sizeof(payload), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
  socklen_t clientAddressLength = sizeof(clientAddress);

  // Receive
  char buffer[4];
  int bytesRead = recvfrom(sockfd, (char *)buffer, sizeof(buffer),
                           0, (struct sockaddr *)&clientAddress, 
                           &clientAddressLength);
  if (bytesRead == -1)
    std::cerr << "Failed to receive data!" << endl;
  if (memcmp(buffer, ackFPGA, sizeof(buffer)) != 0)
    std::cerr << "ACK not received!" << endl;
}
