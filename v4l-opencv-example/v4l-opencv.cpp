#include "opencv2/opencv.hpp"
#include <iostream>
#include <unistd.h>

using namespace std;
using namespace cv;

int main(){

    // Create a VideoCapture object and open the input file
    // If the input is the web camera, pass 0 instead of the video file name
    VideoCapture cap("/dev/video0");

    // Check if camera opened successfully
    if(!cap.isOpened()){
        cout << "Error opening video stream or file" << endl;
        return -1;
    }

    int cap_width = 3264;
    int cap_height = 1832;

    // Testing shtoofs
    cap.set(CAP_PROP_FOURCC, VideoWriter::fourcc('M', 'J', 'P', 'G'));
    cap.set(CAP_PROP_FRAME_WIDTH,cap_width);
    cap.set(CAP_PROP_FRAME_HEIGHT,cap_height);

    for(int i = 0; i < 10; i++){

        printf("Getting image...\n");
        Mat frame, crop;
        // Capture frame-by-frame
        cap >> frame;

        // If the frame is empty, break immediately
        if (frame.empty()) {
            printf("Oops! Blank image...\n");
            break;
        }

        // Crop image before reading it...
        int height = 480;
        int width = 640;
        int start_x = (cap_width / 2) - (width / 2);
        int start_y = (cap_height / 2) - (height / 2);

        frame(Rect(start_x,start_y,width,height)).copyTo(crop);


        printf("Writing cropped image...\n");
        // Writting frame to file...
        imwrite("/stream/pic.jpg", crop);
    }

    // When everything done, release the video capture object
    cap.release();

    return 0;
}
