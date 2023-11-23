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
#define SEGMENT_SIZE    (CAM_WIDTH*3+8)//480
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

TSQueue<cv::Mat> ThreadSafeQueue(1);

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

cv::Mat reassembleImage(const std::vector<std::vector<unsigned char>>& entries, int image_width, int image_height) {
    if (entries.empty()) {
        throw std::invalid_argument("Input vector of entries is empty.");
    }

    // Create a single-channel image with the specified width and height
    cv::Mat reassembledImage(image_height, image_width, CV_8UC1, cv::Scalar(0));

    // Iterate through entries and copy pixel values to the reassembled image
    for (const auto& entry : entries) {
        // Extract row index and number of rows from the entry
        int rowIndex, numRows;
        std::memcpy(&rowIndex, entry.data(), sizeof(int));
        std::memcpy(&numRows, entry.data() + sizeof(int), sizeof(int));

        // Copy pixel values to the reassembled image
        for (int j = 0; j < numRows; ++j) {
            std::memcpy(reassembledImage.ptr(rowIndex + j),
                        entry.data() + sizeof(int) * 2 + j * image_width, image_width);
        }
    }

    return reassembledImage;
}

std::vector<std::vector<unsigned char>> splitImageRows(const cv::Mat& inputImage) {
    std::vector<std::vector<unsigned char>> result;

    if (inputImage.channels() != 1) {
        // Assuming input is a single-channel (grayscale) image
        throw std::invalid_argument("Input image must be a single-channel image.");
    }

    int rows = inputImage.rows;
    int cols = inputImage.cols;

    for (int i = 0; i < rows; i += 3) {
        int numRowsInEntry = std::min(3, rows - i);

        // Create a vector to store the entry
        std::vector<unsigned char> entry(sizeof(int) * 2 + numRowsInEntry * cols);

        // Copy the row index and the number of rows into the entry
        std::memcpy(entry.data(), &i, sizeof(int));
        std::memcpy(entry.data() + sizeof(int), &numRowsInEntry, sizeof(int));

        // Copy the image data into the entry
        for (int j = 0; j < numRowsInEntry; ++j) {
            std::memcpy(entry.data() + sizeof(int) * 2 + j * cols,
                        inputImage.ptr(i + j), cols);
        }

        // Add the entry to the result vector
        result.push_back(entry);
    }

    return result;
}

vector<unsigned char> sendViaUDP(std::vector<unsigned char> msg, bool wait_answer) {
  char buffer[SEGMENT_SIZE];
  struct sockaddr_in clientAddress;
  vector<unsigned char> charVector = {0};

  sendto(sockfd, msg.data(), msg.size(), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
  socklen_t clientAddressLength = sizeof(clientAddress);
  
  if (wait_answer == true) { 
    //cout << "Waiting to receive something from FPGA...";
    int bytesRead = recvfrom(sockfd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddress, &clientAddressLength);
    //cout << "Received " << bytesRead << " bytes" << std::endl;
    if (bytesRead == -1)
      std::cerr << "Failed to receive data!" << endl;

    vector<unsigned char> charVectorImg(buffer, buffer + sizeof(buffer) / sizeof(buffer[0]));
    return charVectorImg;
  }
  else {
    return charVector;
  }
}

void compareAndPrint(const std::vector<std::vector<unsigned char>>& vec1, const std::vector<std::vector<unsigned char>>& vec2) {
    size_t size1 = vec1.size();
    size_t size2 = vec2.size();
    size_t minSize = std::min(size1, size2);

    // Iterate through the vectors of vectors and print differing elements
    for (size_t i = 0; i < minSize; ++i) {
        const auto& subVec1 = vec1[i];
        const auto& subVec2 = vec2[i];

        size_t subSize1 = subVec1.size();
        size_t subSize2 = subVec2.size();
        size_t minSubSize = std::min(subSize1, subSize2);

        // Iterate through the sub-vectors and print differing elements
        for (size_t j = 0; j < minSubSize; ++j) {
            if (subVec1[j] != subVec2[j]) {
                std::cout << "Vectors differ at index (" << i << ", " << j << "): "
                          << "vec1[" << i << "][" << j << "] = " << static_cast<int>(subVec1[j])
                          << ", vec2[" << i << "][" << j << "] = " << static_cast<int>(subVec2[j]) << std::endl;
            }
        }

        // If the sub-vectors are of different sizes, print the differing size
        if (subSize1 != subSize2) {
            std::cout << "Sub-vectors differ in size at index " << i << ": "
                      << "vec1[" << i << "] size = " << subSize1 << ", vec2[" << i << "] size = " << subSize2 << std::endl;
        }
    }

    // If the vectors are of different sizes, print the differing size
    if (size1 != size2) {
        std::cout << "Vectors of vectors differ in size: vec1 size = " << size1 << ", vec2 size = " << size2 << std::endl;
    }
}

cv::Mat sendImgSegments(const cv::Mat& image) {
  // We send each line of the image and readback the processed line
  // the only consideration is that the first processed line comes 
  // after we send two lines, once for a kernel of 3x3, we need at least
  // two lines.

  vector<vector<unsigned char>> rows_split = splitImageRows(image);
  vector<vector<unsigned char>> processed_image;

  for (size_t i=0; i<(CAM_HEIGHT/3); i++) {
    if (i < CAM_HEIGHT) {
      //cout << "Sending segments [" << i*3 << "," << ((i*3)+2) << "] of the image" << endl;
      processed_image.push_back(sendViaUDP(rows_split[i], true));
    }
  }
  //cout << "Received all segments of the image" << endl;

  compareAndPrint(rows_split, processed_image);

  return reassembleImage(processed_image, CAM_WIDTH, CAM_HEIGHT);
  //return reassembleImage(rows_split, CAM_WIDTH, CAM_HEIGHT);
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
    measureElapsedTime(true);
    measureElapsedTime();
    sendCmdFilter();
    Mat image_filtered = sendImgSegments(image);

    double timeMeas = measureElapsedTime();
    cout << "Frame counter: " << frame++ << " Size: " << image.size() <<" Time per frame: " << timeMeas << " ms" << std::endl;
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
