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

std::vector<std::vector<unsigned char>> splitImageIntoPackets(const cv::Mat& image) {
  std::vector<std::vector<unsigned char>> packets;
  int totalPackets = (image.rows * image.cols * image.channels()) / PACKET_SIZE;

  for (int packetIndex = 0; packetIndex < totalPackets; packetIndex++) {
    std::vector<unsigned char> packet(PACKET_SIZE);
    int startIndex = packetIndex * PACKET_SIZE;
    int endIndex = startIndex + PACKET_SIZE;

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

std::vector<unsigned int> convertCharToInt(const std::vector<unsigned char>& charVector) {
  std::vector<unsigned int> intVector;
  intVector.reserve(charVector.size() / 4); // Reserve memory for efficiency

  for (int i=0; i<(int)charVector.size(); i+=4) {
    unsigned int intValue = (static_cast<unsigned char>(charVector[i+3]) << 24)     |
                            (static_cast<unsigned char>(charVector[i+2]) << 16) |
                            (static_cast<unsigned char>(charVector[i+1]) << 8)  |
                             static_cast<unsigned char>(charVector[i+0]);

    intVector.push_back(intValue);
  }

  return intVector;
}

std::vector<unsigned int> normalizeVector(const std::vector<unsigned int>& inputVector, int newMinValue, int newMaxValue) {
    std::vector<unsigned int> normalizedVector;
    normalizedVector.reserve(inputVector.size());

    // Find the minimum and maximum values in the input vector
    auto minMax = std::minmax_element(inputVector.begin(), inputVector.end());
    unsigned int currentMinValue = *minMax.first;
    unsigned int currentMaxValue = *minMax.second;

    // Calculate the normalization range
    double inputRange = currentMaxValue - currentMinValue;
    double outputRange = newMaxValue - newMinValue;

    // Normalize each element in the input vector
    for (const auto& element : inputVector)
    {
        // Normalize the element to the new range
        unsigned int normalizedValue = static_cast<unsigned int>((element - currentMinValue) / inputRange * outputRange) + newMinValue;

        // Add the normalized value to the output vector
        normalizedVector.push_back(normalizedValue);
    }

    return normalizedVector;
}

// Function to send UDP packets
vector<unsigned int> sendPackets(const std::vector<std::vector<unsigned char>>& packets) {
  char buffer[4];
  const char* ackFPGA = "\xaa\xbb\xcc\xdd";  // Hexadecimal value to compare against
  struct sockaddr_in clientAddress;

  // Send the message to the FPGA
  for (const auto& packet : packets) {
    sendto(sockfd, packet.data(), packet.size(), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    socklen_t clientAddressLength = sizeof(clientAddress);
    int bytesRead = recvfrom(sockfd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddress, &clientAddressLength);
    if (bytesRead == -1)
      std::cerr << "Failed to receive data!" << endl;
    if (memcmp(buffer, ackFPGA, sizeof(buffer)) != 0)
      std::cerr << "ACK not received!" << endl;
  }

  // Receive the 1024 bytes histogram (each of the 256 values is 4-bytes long = 256x4 = 1KiB)
  array<unsigned char, PACKET_SIZE> receivedPacket;
  vector<unsigned char> receivedData;

  socklen_t clientAddressLength = sizeof(clientAddress);
  int bytesRead = recvfrom(sockfd, receivedPacket.data(), receivedPacket.size(), 0, (struct sockaddr *)&clientAddress, &clientAddressLength);
  if (bytesRead == -1)
    std::cerr << "Failed to receive data!" << endl;
  if (memcmp(buffer, ackFPGA, sizeof(buffer)) != 0)
    std::cerr << "ACK not received!" << endl;

  receivedData.insert(receivedData.end(), receivedPacket.begin(), receivedPacket.begin() + bytesRead);
  return convertCharToInt(receivedData);
}

void printVectorDigits(const std::vector<unsigned int>& inputVector) {
  int count = 0;

  for (const auto& element : inputVector) {
    std::cout << element << " ";
    count++;

    if (count == 10){
      std::cout << std::endl;
      count = 0;
    }
  }

  // Print a newline if the last line doesn't contain 10 digits
  if (count != 0) {
    std::cout << std::endl;
  }
}

double computeHistogramCorrelation(const std::vector<unsigned int>& hist1, const std::vector<unsigned int>& hist2) {
    if (hist1.size() != hist2.size()) {
        std::cerr << "Error: Histogram sizes do not match." << std::endl;
        return 0.0;
    }

    cv::Mat hist1f(hist1.size(), 1, CV_32F);
    cv::Mat hist2f(hist2.size(), 1, CV_32F);

    for (size_t i = 0; i < hist1.size(); ++i) {
        hist1f.at<float>(i, 0) = static_cast<float>(hist1[i]);
        hist2f.at<float>(i, 0) = static_cast<float>(hist2[i]);
    }

    double correlation = cv::compareHist(hist1f, hist2f, cv::HISTCMP_CORREL);
    double correlationPercentage = (correlation + 1.0) * 50.0; // Scale correlation to [0, 100]

    return correlationPercentage;
}

void plotHistogram() {
  const char* ackFPGA = "\xaa\xbb\xcc\xdd";  // Hexadecimal value to compare against
  uint32_t histReqHW = htonl(0x1f0dd9f);
  struct sockaddr_in clientAddress;
  unsigned long frame = 0;

  while (!stop) {
    Mat image = ThreadSafeQueue.pop();
    cvtColor(image, image, COLOR_BGR2GRAY);

    char buffer[4];
    // Send the Histogram cmd to the FPGA
    measureElapsedTime();
    sendto(sockfd, &histReqHW, sizeof(histReqHW), 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    socklen_t clientAddressLength = sizeof(clientAddress);
    int bytesRead = recvfrom(sockfd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddress, &clientAddressLength);
    if (bytesRead == -1)
      std::cerr << "Failed to receive data!" << endl;
    if (memcmp(buffer, ackFPGA, sizeof(buffer)) != 0)
      std::cerr << "ACK not received!" << endl;

    vector<vector<unsigned char>> udpPkts = splitImageIntoPackets(image);
    vector<unsigned int> histogramHW = sendPackets(udpPkts);

    double elapsedFPGAProc = measureElapsedTime();
    measureElapsedTime(true);

    measureElapsedTime();
    vector<unsigned int> normhistogramHW = normalizeVector(histogramHW, 0, 480);

    //cout << histogramHW.size() << endl;
    //printVectorDigits(normhistogramHW);
    //while(1);

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
    cv::normalize(hist, hist, 0, 480, cv::NORM_MINMAX, -1, cv::Mat());

    // Create a blank white image to draw the histogram
    int width = 3;
    int start_left = 100;
    int histWidth = ((256*width)*2)+start_left+60;
    int histHeight = 512+start_left;
    int start_bottom = histHeight-start_left;
    cv::Mat histImage(histHeight, histWidth, CV_8UC3, cv::Scalar(255, 255, 255));

    // Find the maximum histogram value
    double maxVal = 0;
    cv::minMaxLoc(hist, nullptr, &maxVal);
    vector<unsigned int> histOpenCV;

    double elapsedOpenCVProc = measureElapsedTime();
    // Scale the histogram values to fit the image height
    for (int i=0; i<histSize; i++) {
      float binVal = hist.at<float>(i);
      unsigned int bin_val = cvRound(binVal);
      //int height = cvRound(binVal * histHeight / maxVal);
      // Draw a black line for each histogram bin
      cv::line(histImage, cv::Point(start_left+(i*(width*2)), start_bottom), cv::Point(start_left+(i*(width*2)), start_bottom - bin_val), cv::Scalar(0, 0, 0), 2);

      histOpenCV.push_back(bin_val);
    }

    for (int i=1; i<(int)normhistogramHW.size(); i++) {
      // Draw a black line for each histogram bin
      cv::line(histImage, cv::Point(start_left+((i*(width*2))+width), start_bottom), cv::Point(start_left+((i*(width*2))+width), start_bottom - normhistogramHW[i]), cv::Scalar(0, 0, 255), 2);
    }

    // Add axis titles
    cv::line(histImage, cv::Point(start_left-2, start_bottom+2), cv::Point(start_left-2, 10), cv::Scalar(0, 0, 0), 2);
    cv::line(histImage, cv::Point(start_left-2, start_bottom+2), cv::Point(histWidth-10, start_bottom+2), cv::Scalar(0, 0, 0), 2);
    cv::putText(histImage, "Frequency", cv::Point(10, histHeight / 2), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::putText(histImage, "Bins", cv::Point(histWidth/2, start_bottom+40), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);

    cv::putText(histImage, "0", cv::Point(start_left-10, start_bottom+20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::putText(histImage, "480", cv::Point(start_left-40, start_bottom-480), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::putText(histImage, "255", cv::Point(start_left+(256*(3*2))+3-20, start_bottom+20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);

    cv::putText(histImage, "Frequency - OpenCV", cv::Point(histWidth/2, 20), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);
    cv::putText(histImage, "Frequency - FPGA", cv::Point(histWidth/2, 40), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);

    // Display the histogram plot
    cv::imshow("Histogram - Nexys Video (FPGA)", histImage);

    for (int i=1; i<(int)normhistogramHW.size(); i++) {
      // Draw a black line for each histogram bin
      ;
    }
    cout << frame++ << "," << computeHistogramCorrelation(normhistogramHW, histOpenCV) << "," << elapsedFPGAProc << "," << elapsedOpenCVProc << endl;
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
