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
#define BLOCK_SIZE      1 // Multiples of 1KiB
#define FILTER_TYPE     0x1
#define CMD_SIZE        5
#define CAM_WIDTH       320 //640
#define CAM_HEIGHT      240 //480

struct sockaddr_in serverAddress;
struct sockaddr_in localAddress;
int    sockfd;
bool   stop = false;

using namespace std;
using namespace cv;

// Function to measure the elapsed time
double measureElapsedTime(bool reset = false)
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    if (reset)
    {
        startTime = std::chrono::high_resolution_clock::now();
    }

    return elapsedTime;
}

template <typename T>
class TSQueue {
private:
  // Underlying queue
  queue<T> m_queue;

  // mutex for thread synchronization
  mutex m_mutex;

  // Condition variable for signaling
  condition_variable m_cond;

  size_t maxSize_;

public:
  // Constructor
  explicit TSQueue(size_t maxSize) : maxSize_(maxSize) {
  }

  // Pushes an element to the queue
  void push(T item) {
    // Acquire lock
    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_queue.size() >= maxSize_){
      m_queue.pop();
    }

    // Add item
    m_queue.push(item);

    // Notify one thread that
    // is waiting
    m_cond.notify_one();
  }

  // Pops an element off the queue
  T pop() {
    // acquire lock
    unique_lock<std::mutex> lock(m_mutex);

    // wait until queue is not empty
    m_cond.wait(lock, [this]() { return !m_queue.empty(); });

    //cout << "Queue Size: " << m_queue.size() << endl;
    // retrieve item
    T item = m_queue.front();
    m_queue.pop();

    // return item
    return item;
  }
};

TSQueue<cv::Mat> ThreadSafeQueue(100);

void streamVideo() {
  cv::VideoCapture inputVideo; //to open video flux
  inputVideo.open(0);
  int frame_width = CAM_WIDTH; //inputVideo.get(cv::CAP_PROP_FRAME_WIDTH);
  int frame_height = CAM_HEIGHT; //inputVideo.get(cv::CAP_PROP_FRAME_HEIGHT);
  int fps = inputVideo.get(cv::CAP_PROP_FPS);

  cv::VideoWriter outputVideo("appsrc ! videoconvert ! video/x-raw ! x264enc ! rtph264pay ! udpsink host=127.0.0.1 port=5000",cv::CAP_GSTREAMER,0,fps,cv::Size(frame_width,frame_height), true);

  while (inputVideo.grab()) {
    cv::Mat image, imageCopy;
    inputVideo.retrieve(image);
    cv::resize(image, imageCopy, cv::Size(frame_width, frame_height)); // Resize image to 320x240                                                                                
    outputVideo.write(imageCopy);
    ThreadSafeQueue.push(imageCopy);
    cv::imshow("Webcam Stream - 640x480 - 300KiB", imageCopy);
    char key = (char) cv::waitKey(10);
    if (key == 27)
      break;
    }

    inputVideo.release();
    outputVideo.release();
}

void convertToNetworkOrder(uint32_t* array, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    array[i] = htonl(array[i]);
  }
}

std::vector<std::vector<unsigned char>> splitImageIntoPackets(const cv::Mat& image) {
  std::vector<std::vector<unsigned char>> packets;
  int totalPackets = (image.rows * image.cols * image.channels()) / BLOCK_SIZE*1024;

  for (int packetIndex = 0; packetIndex < totalPackets; packetIndex++) {
    std::vector<unsigned char> packet(BLOCK_SIZE);
    int startIndex = packetIndex * BLOCK_SIZE;
    int endIndex = startIndex + BLOCK_SIZE;

    int packetOffset = 0;
    for (int i = startIndex; i < endIndex; i++) {
      int row = i / (image.cols * image.channels());
      int col = (i % (image.cols * image.channels())) / image.channels();
      int channel = i % image.channels();
      packet[packetOffset++] = image.at<cv::Vec3b>(row, col)[channel];
    }

    packets.push_back(packet);
  }

  return packets;
}

void sendCmdFilter(uint16_t x, uint16_t y, uint16_t block_size){
  struct sockaddr_in clientAddress;
  const char* ackFPGA = "\xaa\xbb\xcc\xdd";  // Hexadecimal value to compare against
                              // Operation type | Width image | Height image | (x,y) | block size
  uint32_t filterReqHW[CMD_SIZE] = {FILTER_TYPE, CAM_WIDTH, CAM_HEIGHT, (x<<16) | y, block_size};
  
  //convertToNetworkOrder(filterReqHW, 4);  
  sendto(sockfd, &filterReqHW, sizeof(filterReqHW), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
  socklen_t clientAddressLength = sizeof(clientAddress);

  char buffer[4];
  int bytesRead = recvfrom(sockfd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddress, &clientAddressLength);
  if (bytesRead == -1)
    std::cerr << "Failed to receive data!" << endl;
  if (memcmp(buffer, ackFPGA, sizeof(buffer)) != 0)
    std::cerr << "ACK not received!" << endl;
}

void plotFilter() {
  unsigned long frame = 0;

  //cout << "Frame Count, Hist. Correlation, Time per frame FPGA, Time per frame OpenCV " << endl;
  while (!stop) {
    Mat image = ThreadSafeQueue.pop();
    cvtColor(image, image, COLOR_BGR2GRAY);

    // Send the Histogram cmd to the FPGA
    //measureElapsedTime(true);
    //measureElapsedTime();
    sendCmdFilter(0, 0, frame);
    //for ()
    //vector<vector<unsigned char>> udpPkts = splitImageIntoPackets(image);

    cout << "Frame counter: " << frame++ << std::endl;
    cout << "Size: " << image.size()  << std::endl;
    // Display the histogram plot
    //cv::imshow("Histogram - Nexys Video (FPGA)", image);
  }
  destroyAllWindows();
}

int main() {
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
    thread streamThread(streamVideo);
    thread histogramThread(plotFilter);

    // Wait for the threads to finish
    streamThread.join();
    histogramThread.join();

    return 0;
}
