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

  cap.set(CAP_PROP_FRAME_WIDTH,640);
  cap.set(CAP_PROP_FRAME_HEIGHT,480);

  for(int i = 0; i < 10; i++){

    Mat frame;
    // Capture frame-by-frame
    cap >> frame;

    // If the frame is empty, break immediately
    if (frame.empty())
      break;

    // Wait 1 second before writing frame...
    usleep(1000000);

    // Writting frame to file...
    imwrite("/stream/pic.png", frame);
  }

  // When everything done, release the video capture object
  cap.release();

  return 0;
}
