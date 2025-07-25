#include <opencv2/opencv.hpp>
#include <librealsense2/rs.hpp>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sstream>
#include <vector>
#include <thread>
#include <iostream>

std::string buildMJPEGHeader() {
    std::ostringstream oss;
    oss << "HTTP/1.1 200 OK\r\n"
        << "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        << "Cache-Control: no-cache\r\n"
        << "Connection: close\r\n\r\n";
    return oss.str();
}

std::string buildJPEGPart(const std::vector<uchar>& jpegBuf) {
    std::ostringstream oss;
    oss << "--frame\r\n"
        << "Content-Type: image/jpeg\r\n"
        << "Content-Length: " << jpegBuf.size() << "\r\n\r\n";
    return oss.str();
}

void handleClient(int client_fd, rs2::pipeline& pipe, rs2::colorizer& color_map) {
    std::string header = buildMJPEGHeader();
    send(client_fd, header.c_str(), header.size(), 0);

    while (true) {
        rs2::frameset frames;
        if (!pipe.poll_for_frames(&frames)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        rs2::depth_frame depth_frame = frames.get_depth_frame();
        if (!depth_frame) continue;

        rs2::frame colorized = color_map.colorize(depth_frame);

        // Convert to OpenCV Mat
	rs2::video_frame vf = colorized.as<rs2::video_frame>();
	cv::Mat frame(cv::Size(vf.get_width(), vf.get_height()), CV_8UC3, (void*)vf.get_data(), cv::Mat::AUTO_STEP);


        std::vector<uchar> jpegBuf;
        if (!cv::imencode(".jpg", frame, jpegBuf)) {
            std::cerr << "JPEG encoding failed.\n";
            continue;
        }

        std::string partHeader = buildJPEGPart(jpegBuf);
        if (send(client_fd, partHeader.c_str(), partHeader.size(), 0) < 0 ||
            send(client_fd, reinterpret_cast<const char*>(jpegBuf.data()), jpegBuf.size(), 0) < 0 ||
            send(client_fd, "\r\n", 2, 0) < 0) {
            std::cerr << "Client disconnected.\n";
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

    close(client_fd);
    std::cout << "Client disconnected.\n";
}

int main() {
    rs2::pipeline pipe;
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);

    rs2::colorizer color_map; // This colorizes depth for visualization

    try {
        pipe.start(cfg);
    } catch (const rs2::error& e) {
        std::cerr << "RealSense error: " << e.what() << "\n";
        return -1;
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Socket creation failed.\n";
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed.\n";
        return -1;
    }

    if (listen(server_fd, 5) < 0) {
        std::cerr << "Listen failed.\n";
        return -1;
    }

    std::cout << "Streaming colorized depth at: http://localhost:8080/\n";

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);
        if (client_fd >= 0) {
            std::cout << "Client connected.\n";
            std::thread(handleClient, client_fd, std::ref(pipe), std::ref(color_map)).detach();
        }
    }

    close(server_fd);
    return 0;
}

