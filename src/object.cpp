#include <opencv2/opencv.hpp>
#include <iostream>

using namespace cv;
using namespace std;

int main() {
    // Connect to IP webcam stream
    VideoCapture cap("http://localhost:8080/video");


    if (!cap.isOpened()) {
        cerr << "Error: Could not open the stream from http://localhost:8080/" << endl;
        return -1;
    }

    Mat img;
    while (true) {
        cap >> img;
        if (img.empty()) {
            cerr << "Error: Empty frame received." << endl;
            break;
        }

        Mat img_gray;
        cvtColor(img, img_gray, COLOR_BGR2GRAY);

        // Bilateral filter for noise removal
        Mat noise_removal;
        bilateralFilter(img_gray, noise_removal, 9, 75, 75);

        // Threshold using OTSU
        Mat thresh_image;
        threshold(noise_removal, thresh_image, 0, 255, THRESH_BINARY | THRESH_OTSU);

        // Canny edge detection
        Mat canny_image;
        Canny(thresh_image, canny_image, 250, 255);
        convertScaleAbs(canny_image, canny_image);

        // Dilation to strengthen edges
        Mat dilated_image;
        Mat kernel = Mat::ones(3, 3, CV_8U);
        dilate(canny_image, dilated_image, kernel);

        // Find contours
        vector<vector<Point>> contours;
        vector<Vec4i> hierarchy;
        findContours(dilated_image, contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        if (!contours.empty()) {
            sort(contours.begin(), contours.end(),
                [](const vector<Point>& c1, const vector<Point>& c2) {
                    return contourArea(c1, false) > contourArea(c2, false);
                });

            vector<Point> cnt = contours[0];
            vector<Point> approx;
            approxPolyDP(cnt, approx, 0.01 * arcLength(cnt, true), true);

            cout << "Number of corners: " << approx.size() << endl;

            Point pt(180, 3 * img.rows / 4);
            if (approx.size() == 6 || approx.size() == 7) {
                putText(img, "Cube", pt, FONT_HERSHEY_SCRIPT_SIMPLEX, 2, Scalar(0, 255, 255), 2);
                drawContours(img, vector<vector<Point>>{cnt}, -1, Scalar(255, 0, 0), 3);
                cout << "Cube" << endl;
            } else if (approx.size() == 8) {
                putText(img, "Cylinder", pt, FONT_HERSHEY_SCRIPT_SIMPLEX, 2, Scalar(0, 255, 255), 2);
                drawContours(img, vector<vector<Point>>{cnt}, -1, Scalar(255, 0, 0), 3);
                cout << "Cylinder" << endl;
            } else if (approx.size() > 10) {
                putText(img, "Sphere", pt, FONT_HERSHEY_SCRIPT_SIMPLEX, 2, Scalar(255, 0, 0), 2);
                drawContours(img, vector<vector<Point>>{cnt}, -1, Scalar(255, 0, 0), 3);
                cout << "Sphere" << endl;
            }
        }

        // Corner detection
        vector<Point2f> corners;
        goodFeaturesToTrack(thresh_image, corners, 6, 0.06, 25);
        for (const auto& pt : corners) {
            circle(img, pt, 10, Scalar(255, 255, 255), -1);
        }
	//namedWindow("Shape Detection", WINDOW_NORMAL);
	
	//namedWindow("Canny Edge Image", WINDOW_NORMAL);
	//namedWindow("Thresholded Image", WINDOW_NORMAL);
	//namedWindow("Grayscale Image", WINDOW_NORMAL);

        imshow("Grayscale Image", img_gray);
	imshow("Thresholded Image", thresh_image);
	imshow("Canny Edge Image", canny_image);
	imshow("Shape Detection", img);


        if (waitKey(1) == 27) break; // Press Esc to exit
    }

    return 0;
}

