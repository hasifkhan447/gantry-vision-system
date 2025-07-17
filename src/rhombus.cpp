#include "opencv2/core/hal/interface.h"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
  std::string url = "http://10.20.1.227:4747/video";

  cv::VideoCapture cap(url);
  if (!cap.isOpened()) {
    std::cerr << "Failed to open stream\n";
    return -1;
  }



  cv::Mat frame, gray, filtered_out, blurred1, blurred2, thresh, edges;

  while (true) {
    cap >> frame;
    if (frame.empty()) break;  //end of stream

    // Convert to grayscale
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    // Blur and edge detection
    cv::GaussianBlur(gray, blurred1, cv::Size(11, 11), 0);
    cv::adaptiveThreshold(blurred1, thresh, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY_INV, 11, 2);
    cv::GaussianBlur(thresh, blurred2, cv::Size(21, 21), 0);
    cv::Canny(thresh, edges, 20, 50);
    // cv::Sobel(thresh, edges, CV_16S, 1, 0);
    // Find contours
    std::vector<std::vector<cv::Point>> contours;
    std::vector<std::vector<cv::Point>> filtered;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    int minArea = 300;

    for (const auto& cnt : contours) {
      if (cv::contourArea(cnt) > minArea) {
        std::vector<cv::Point> approximation;
        cv::approxPolyDP(cnt, approximation, 0.02 * cv::arcLength(cnt, 1), 1);

        if (approximation.size() <= 6 && approximation.size() >= 4 && cv::isContourConvex(approximation)) {
          filtered.push_back(approximation);
        }
      }
    }


    filtered_out = cv::Mat::zeros(frame.size(), CV_8UC3);
    cv::drawContours(frame, filtered, -1, cv::Scalar(0,255,0),2);

    // Loop through contours
    // for (const auto& cnt : filtered) {
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
    //

    cv::imshow("Filtered", edges);
    cv::imshow("Bounding", frame);


    // Press 'q' to quit
    if (cv::waitKey(1) == 'q') break;
  }

  cap.release();
  cv::destroyAllWindows();
  return 0;
}

