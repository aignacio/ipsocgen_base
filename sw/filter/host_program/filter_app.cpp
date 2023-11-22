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

#define STR_HELPER(x)   #x
#define STR(x)          STR_HELPER(x)
#define FPGA_IP         "192.168.1.130"
#define FPGA_PORT       1234
#define BLOCK_SIZE      1 // Multiples of 1KiB
#define FILTER_TYPE     0x1
#define CMD_SIZE        3
#define CAM_WIDTH       320 //640
#define CAM_HEIGHT      240 //480
#define KERNEL_SIZE     3

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
    cv::imshow("Webcam Stream " STR(CAM_WIDTH) "x" STR(CAM_HEIGHT), imageCopy);
    char key = (char) cv::waitKey(10);
    if (key == 27)
      break;
    }

    inputVideo.release();
    outputVideo.release();
}

cv::Mat createImageFromRows(const std::vector<std::vector<unsigned char>>& rows_split, int imageWidth, int imageHeight) {
  cv::Mat image(imageHeight, imageWidth, CV_8U);

  for (const auto& rowEntry : rows_split) {
    if (rowEntry.size() >= 4) {
      // Extract row number from the first 4 bytes
      int rowNumber = (rowEntry[0] << 24) | (rowEntry[1] << 16) | (rowEntry[2] << 8) | rowEntry[3];
      // Copy the pixel values to the corresponding row in the image
      if (rowNumber < imageHeight) {
        std::copy(rowEntry.begin() + 4, rowEntry.end(), image.ptr(rowNumber));
      }
      else {
        std::cerr << "Row number exceeds image height." << std::endl;
      }
    } 
    else {
      std::cerr << "Invalid row entry format." << std::endl;
    }
  }

  return image;
}

vector<unsigned char> sendViaUDP(std::vector<unsigned char> msg, bool wait_answer) {
  char buffer[CAM_WIDTH];
  struct sockaddr_in clientAddress;
  vector<unsigned char> charVector = {0};

  sendto(sockfd, msg.data(), msg.size(), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
  socklen_t clientAddressLength = sizeof(clientAddress);
  
  if (wait_answer == true) { 
    cout << "Waiting to receive something from FPGA...";
    int bytesRead = recvfrom(sockfd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddress, &clientAddressLength);
    cout << "Received" << std::endl;
    if (bytesRead == -1)
      std::cerr << "Failed to receive data!" << endl;

    vector<unsigned char> charVectorImg(buffer, buffer + sizeof(buffer) / sizeof(buffer[0]));
    return charVectorImg;
  }
  else {
    return charVector;
  }
}

Mat sendImgSegments(const Mat& image) {
  // We send each line of the image and readback the processed line
  // the only consideration is that the first processed line comes 
  // after we send two lines, once for a kernel of 3x3, we need at least
  // two lines.

  vector<vector<unsigned char>> rows_split;

  // Split image into vector of rows
  // Ensure that the image is single-channel (gray)
  if (image.channels() == 1) {
      // Iterate over each row in the image
      for (int i = 0; i < image.rows; ++i) {
          // Get the i-th row
          cv::Mat row = image.row(i);

          // Convert the row number to a vector of 4 bytes
          vector<unsigned char> rowNumberBytes(4);
          rowNumberBytes[0] = (i >> 24) & 0xFF;
          rowNumberBytes[1] = (i >> 16) & 0xFF;
          rowNumberBytes[2] = (i >> 8) & 0xFF;
          rowNumberBytes[3] = i & 0xFF;

          // Convert the row data to a vector
          vector<unsigned char> rowData(row.begin<unsigned char>(), row.end<unsigned char>());

          // Concatenate the row number bytes and row data
          rowNumberBytes.insert(rowNumberBytes.end(), rowData.begin(), rowData.end());

          // Add the result to the final array
          rows_split.push_back(rowNumberBytes);
      }
  }
  else {
    cout << "[ERROR] Image is not single channel" << endl;
  }

  // Send the initial required rows before sending all rows
  uint8_t bootstrap_rows = 0;
  for (size_t i=0; i<(KERNEL_SIZE-2); i++){
    bootstrap_rows += 1;
    cout << "Sending segment[" << i << "] of the image" << endl;
    sendViaUDP(rows_split[i], true);
  }
  
  std::vector<unsigned char> msg = {0xDE, 0xAD, 0xBE, 0xEF};
  vector<vector<unsigned char>> processed_image;

  for (size_t i=bootstrap_rows; i<(CAM_HEIGHT+bootstrap_rows); i++) {
    if (i < CAM_HEIGHT) {
      cout << "Sending segment[" << i << "] of the image" << endl;
      processed_image.push_back(sendViaUDP(rows_split[i], true));
    }
    else {
      cout << "Receiving remaining segment" << endl;
      processed_image.push_back(sendViaUDP(msg, true));
    }
  }
  cout << "Received all segments" << endl;
  return createImageFromRows(processed_image, CAM_WIDTH, CAM_HEIGHT);
}

void sendCmdFilter(){
  struct sockaddr_in clientAddress;
  const char* ackFPGA = "\xaa\xbb\xcc\xdd";  // Hexadecimal value to compare against
                              // Operation type | Width image | Height image | (x,y) | block size
  uint32_t filterReqHW[CMD_SIZE] = {FILTER_TYPE, CAM_WIDTH, CAM_HEIGHT};
  
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
    sendCmdFilter();
    Mat image_filtered = sendImgSegments(image);

    cout << "Frame counter: " << frame++ << " Size: " << image.size() << std::endl;
    // Display the histogram plot
    cv::imshow("Filtered Image - Nexys Video (FPGA)", image_filtered);
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
