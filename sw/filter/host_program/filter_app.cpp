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
bool stop = false;

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
  VideoCapture cap(0); // Open the default camera
  cap.set(CAP_PROP_FRAME_WIDTH, 640);
  cap.set(CAP_PROP_FRAME_HEIGHT, 480);

  if (!cap.isOpened()) {
    cout << "Error opening video stream or file" << endl;
    return;
  }
  namedWindow("Webcam Stream - 640x480 - 300KiB", WINDOW_NORMAL);

  while (!stop) {
    Mat frame;
    cap >> frame; // Capture frame-by-frame

    if (frame.empty())
      break;

    imshow("Webcam Stream - 640x480 - 300KiB", frame); // Display the resulting frame
    ThreadSafeQueue.push(frame);
    if ((waitKey(1) == 27) || (waitKey(1) == 113)) // Press ESC to stop streaming
      break;
  }

  cap.release(); // Release the camera
  destroyAllWindows();
}

void plotFilter() {
  const char* ackFPGA = "\xaa\xbb\xcc\xdd";  // Hexadecimal value to compare against
  uint32_t histReqHW = htonl(0x1f0dd9f);
  struct sockaddr_in clientAddress;
  unsigned long frame = 0;

  cout << "Frame Count, Hist. Correlation, Time per frame FPGA, Time per frame OpenCV " << endl;
  while (!stop) {
    Mat image = ThreadSafeQueue.pop();
    cvtColor(image, image, COLOR_BGR2GRAY);

    char buffer[4];
    //measureElapsedTime();
    // Display the histogram plot
    cv::imshow("Histogram - Nexys Video (FPGA)", image);
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
