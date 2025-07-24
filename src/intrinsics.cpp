#include <librealsense2/rs.hpp>
#include <iostream>

int main() {
    rs2::pipeline pipe;
    pipe.start();

    auto stream = pipe.get_active_profile().get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
    auto intr = stream.get_intrinsics();

    std::cout << "fx: " << intr.fx << std::endl;
    std::cout << "fy: " << intr.fy << std::endl;
    std::cout << "cx: " << intr.ppx << std::endl;
    std::cout << "cy: " << intr.ppy << std::endl;

    std::cout << "Distortion coefficients: ";
    for (int i = 0; i < 5; ++i)
        std::cout << intr.coeffs[i] << " ";
    std::cout << std::endl;

    pipe.stop();
    return 0;
}


/* 
Output: 

fx: 1389.83
fy: 1389.83
cx: 962.018
cy: 538.503
Distortion coefficients: 0 0 0 0 0 
*/
