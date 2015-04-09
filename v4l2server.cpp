/*
 * v4l2server.cpp
 *
 *  Created on: 07/04/2015
 *      Author: Oscar Javier Garcia Baudet
 */

#include <iostream>

#include "v4l2.h"

/** Interface class to define callback function */
class FrameCallback {
 public:
  // Callback function called when new frame is fetched from camera
  virtual int onFrame(void* buffer, int length) = 0;
 protected:
     ~FrameCallback(){}
};

int main(int argc, char** argv) {
  v4l2::Camera camera = v4l2::Camera("/dev/video0", 320, 240, 10);
  try {
    camera.Open();
    std::cout << "Get ready! -> " << camera.is_active() << std::endl;
    camera.Start();
    std::cout << "Camera started! -> " << camera.is_active() << std::endl;
    camera.Stop();
  } catch (std::string& e) {
    std::cout << "ERROR: " << e << std::endl;
  }
  camera.Close();
  return 0;
}
