#include <librealsense2/rs.hpp> // RealSense SDK
#include <opencv2/opencv.hpp>
#include <iostream>
#include <deque>
#include <cmath>

using namespace cv;
using namespace std;

// Structure to store contour with frame count for temporal smoothing
struct TrackedContour {
    vector<Point> points;
    int frameCount;
};

// Sort the corners in consistent order (top-left, top-right, bottom-right, bottom-left)
void sortCorners(vector<Point>& corners) {
    Point center(0, 0);
    for (const auto& pt : corners)
        center += pt;
    center *= (1.0 / corners.size());

    sort(corners.begin(), corners.end(), [center](Point a, Point b) {
        double angleA = atan2(a.y - center.y, a.x - center.x);
        double angleB = atan2(b.y - center.y, b.x - center.x);
        return angleA < angleB;
    });
}

// Compute the average distance between two contours
double contourDistance(const vector<Point>& a, const vector<Point>& b) {
    if (a.size() != b.size()) return DBL_MAX;
    double totalDist = 0;
    for (size_t i = 0; i < a.size(); ++i)
        totalDist += norm(a[i] - b[i]);
    return totalDist / a.size();
}

int main() {
    rs2::pipeline pipe;
    rs2::config cfg;

    cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);
    cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);

    rs2::pipeline_profile profile = pipe.start(cfg);
    rs2::align align_to_color(RS2_STREAM_COLOR);

    deque<TrackedContour> trackedContours;
    const int maxFrames = 5;
    const double maxDist = 30.0;

    while (true) {
        rs2::frameset frames = pipe.wait_for_frames();
        frames = align_to_color.process(frames);

        rs2::video_frame color_frame = frames.get_color_frame();
        rs2::depth_frame depth_frame = frames.get_depth_frame();

        Mat frame(Size(640, 480), CV_8UC3, (void*)color_frame.get_data(), Mat::AUTO_STEP);
        Mat gray, edges;
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        GaussianBlur(gray, gray, Size(5, 5), 1.5);
        Canny(gray, edges, 50, 150);
        morphologyEx(edges, edges, MORPH_CLOSE, Mat());

        vector<vector<Point>> contours;
        findContours(edges, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

        for (const auto& contour : contours) {
            vector<Point> approx;
            approxPolyDP(contour, approx, arcLength(contour, true) * 0.02, true);

            if (approx.size() == 4 && isContourConvex(approx)) {
                sortCorners(approx);

                bool matched = false;
                for (auto& tracked : trackedContours) {
                    if (contourDistance(approx, tracked.points) < maxDist) {
                        tracked.points = approx;
                        tracked.frameCount++;
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    trackedContours.push_back({approx, 1});
                }
            }
        }

        if (!trackedContours.empty() && trackedContours.size() > 20)
            trackedContours.pop_front();

        for (const auto& tracked : trackedContours) {
            if (tracked.frameCount >= 2) {
                for (int i = 0; i < 4; i++)
                    line(frame, tracked.points[i], tracked.points[(i + 1) % 4], Scalar(0, 255, 0), 2);

                // Depth at center of quadrilateral
                Point center(0, 0);
                for (const auto& pt : tracked.points)
                    center += pt;
                center *= (1.0 / tracked.points.size());

                float depth_m = depth_frame.get_distance(center.x, center.y);
                int depth_mm = static_cast<int>(depth_m * 1000);

                string label = to_string(depth_mm) + " mm";
                putText(frame, label, center, FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 255), 2);
            }
        }
	// Normalize depth for visualization (0–255 range)
	Mat depth_image(Size(640, 480), CV_16U, (void*)depth_frame.get_data(), Mat::AUTO_STEP);
	Mat depth_display_8u, depth_colormap;
	depth_image.convertTo(depth_display_8u, CV_8U, 15.0 / 256.0); // Normalize to 8-bit
	applyColorMap(depth_display_8u, depth_colormap, COLORMAP_JET); // or COLORMAP_TURBO, COLORMAP_HOT, etc.
	imshow("Depth View (Colorized)", depth_colormap);

        imshow("Detected Rectangles (RGB + Depth)", frame);
        imshow("Contours", edges);
        if (waitKey(1) == 27) break; // ESC to quit
    }

    return 0;
}

