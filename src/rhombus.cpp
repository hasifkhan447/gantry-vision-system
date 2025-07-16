#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
  std::string url = "http://192.168.0.100:4747/video";

  cv::VideoCapture cap(url);
  if (!cap.isOpened()) {
    std::cerr << "Failed to open stream\n";
    return -1;
  }



  cv::Mat frame, gray, blurred, thresh, edges;

  while (true) {
    cap >> frame;
    if (frame.empty()) break;  //end of stream

    // Convert to grayscale
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    // Blur and edge detection
    cv::GaussianBlur(gray, blurred, cv::Size(13, 13), 0);
    cv::adaptiveThreshold(blurred, thresh, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY_INV, 11, 2);
    cv::Canny(thresh, edges, 40, 100);
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    // cv::findContours(edges, thresh, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // // Loop through contours
    // for (const auto& cnt : contours) {
    //   double perimeter = cv::arcLength(cnt, true);
    //   std::vector<cv::Point> approx;
    //   cv::approxPolyDP(cnt, approx, 0.02 * perimeter, true);
    //
    //   double area = cv::contourArea(approx);
    //   if (approx.size() == 4 && area > 400) {
    //     cv::RotatedRect rotRect = cv::minAreaRect(approx);
    //
    //     cv::Point2f vertices[4];
    //     rotRect.points(vertices);
    //     for (int i = 0; i < 4; i++)
    //       cv::line(frame, vertices[i], vertices[(i + 1) % 4], cv::Scalar(0, 255, 0), 2);
    //   } else {
    //     // For debugging non-quad contours
    //     if (area > 400) {
    //       cv::Rect bbox = cv::boundingRect(cnt);
    //       cv::rectangle(frame, bbox, cv::Scalar(0, 0, 255), 1); // Show rejected ones
    //     }
    //   }
    // }

    cv::imshow("Threshold", thresh);
    cv::imshow("Edges", edges);


    // Press 'q' to quit
    if (cv::waitKey(1) == 'q') break;
  }

  cap.release();
  cv::destroyAllWindows();
  return 0;
}

