#include <iostream>
#include <thread>

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

#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>

#define FPGA_IP         "192.168.1.130"
#define FPGA_PORT       1234
#define MAX_BUFFER_SIZE 1024

struct sockaddr_in serverAddress;
struct sockaddr_in localAddress;
int    sockfd;
bool stop = false;

using namespace std;
using namespace cv;

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

    cout << "Queue Size: " << m_queue.size() << endl;
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

void plotHistogram() {
  while (!stop) {
    Mat image = ThreadSafeQueue.pop();
    cvtColor(image, image, COLOR_BGR2GRAY);

    // Send histogram request to the FPGA
    uint32_t histReqHW = htonl(0x1f0dd9f);
    char     buffer[5];

    // Send the message to the FPGA
    sendto(sockfd, &histReqHW, sizeof(histReqHW), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

    // Receive data
    struct sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    int bytesRead = recvfrom(sockfd, buffer, sizeof(buffer)-1, 0, (struct sockaddr *)&clientAddress, &clientAddressLength);
    if (bytesRead == -1) {
        std::cerr << "Failed to receive data.\n";
        break;
    }

    std::cout << "Received value from server: " << bytesRead << std::endl;

    // ------------------------------------------------------------------------------------------------------
    // Compute the histogram
    int histSize = 256; // Number of bins
    float range[] = { 0, 256 }; // Pixel range
    const float* histRange = { range };
    bool uniform = true;
    bool accumulate = false;
    Mat hist;

    // Convert from 3CH to 1CH
    cv::calcHist(&image, 1, nullptr, cv::Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);
    // Normalize the histogram values
    cv::normalize(hist, hist, 0, hist.rows, cv::NORM_MINMAX, -1, cv::Mat());

    // Create a blank white image to draw the histogram
    int histWidth = 512;
    int histHeight = 400;
    cv::Mat histImage(histHeight, histWidth, CV_8UC3, cv::Scalar(255, 255, 255));

    // Find the maximum histogram value
    double maxVal = 0;
    cv::minMaxLoc(hist, nullptr, &maxVal);

    // Scale the histogram values to fit the image height
    for (int i = 0; i < histSize; i++) {
        float binVal = hist.at<float>(i);
        int height = cvRound(binVal * histHeight / maxVal);

        // Draw a black line for each histogram bin
        cv::line(histImage, cv::Point(2*i, histHeight), cv::Point(2*i, histHeight - height), cv::Scalar(0, 0, 0), 1);
    }


    // Add axis titles
    cv::putText(histImage, "Frequency - Ref", cv::Point(10, histHeight / 2), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);

    // Display the histogram plot
    cv::imshow("Histogram", histImage);
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
    thread histogramThread(plotHistogram);

    // Wait for the threads to finish
    streamThread.join();
    histogramThread.join();

    return 0;
}
