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
#define LOOP_SAMPLES    10
#define TIMES           13
#define BASE            2

struct sockaddr_in serverAddress;
struct sockaddr_in localAddress;
int    sockfd;

using namespace std;
using namespace cv;

typedef union {
  uint64_t dword;
  struct {
    uint32_t factor;
    uint32_t times;
  } st;
} Oper_t;

double measureElapsedTime(bool reset = false) {
  static auto startTime = chrono::high_resolution_clock::now();

  auto endTime = chrono::high_resolution_clock::now();
  auto elapsedTime = chrono::duration<double, milli>(endTime - startTime).count();

  if (reset) {
    startTime = chrono::high_resolution_clock::now();
  }

  return elapsedTime;
}

void sendUDP (Oper_t op);

void streamCmd (void) {
  using namespace std::this_thread; // sleep_for, sleep_until
  using namespace std::chrono; // nanoseconds, system_clock, seconds

  Oper_t op;
  double elapsed_time_vec [TIMES][LOOP_SAMPLES];

  for (size_t i=0; i<TIMES; i++) {
    for (size_t sample=0; sample<LOOP_SAMPLES; sample++) {
      op.st.times = pow(BASE,i);
      op.st.factor = 1;
      auto startTime = chrono::high_resolution_clock::now();
      sendUDP(op);
      auto endTime = chrono::high_resolution_clock::now();
      auto elapsedTime = chrono::duration<double, milli>(endTime - startTime).count();
      elapsed_time_vec[i][sample] = elapsedTime;
      cout << "Sample: " << sample << " Factor: " << op.st.factor << " Times: " << op.st.times << " - Elapse time: " << elapsedTime << endl;
    }
  }

  cout << endl;
  cout << "Results:" << endl;

  for (size_t i=0; i<TIMES; i++) {
    double average = 0;
    for (size_t j=0; j<LOOP_SAMPLES; j++) {
      average += elapsed_time_vec[i][j];
    }
    average /= LOOP_SAMPLES;
    cout << "Number of Loops: " << pow(BASE,i) << "\t Average times " << LOOP_SAMPLES << " sample: " << average << endl;
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
void sendUDP (Oper_t op) {
  const char* ackFPGA = "\xaa\xbb\xcc\xdd";  // Hexadecimal value to compare against
  uint64_t payload = op.dword;//htonl(data);
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
