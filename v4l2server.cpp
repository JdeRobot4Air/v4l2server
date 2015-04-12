/*
 * v4l2server.cpp
 *
 *  Created on: 07/04/2015
 *      Author: Oscar Javier Garcia Baudet
 */

#include <iostream>
#include <fstream>

#include "v4l2.h"

/** Interface class to define callback function */
class FrameCallback {
 public:
  // Callback function called when new frame is fetched from camera
  virtual int onFrame(void* buffer, int length) = 0;
 protected:
  ~FrameCallback() {
  }
};

int main(int argc, char** argv) {
  v4l2::Format* format = new v4l2::Format();
  v4l2::Buffer* buffer = NULL, *buffer2;
  format->width = 320;
  format->height = 240;
  format->format = "YUYV";
  v4l2::Camera camera = v4l2::Camera("/dev/video0", format, 10);
  try {
    camera.Open();
    std::cout << "Get ready! -> " << camera.is_active() << std::endl;
    camera.GetFormat(format);
    std::cout << "Initial format: " << format->format << " (" << format->width
              << "x" << format->height << ")" << std::endl;
    camera.Start();
    std::cout << "Camera started! -> " << camera.is_active() << std::endl;
    for (int i = 0; i < 20; i++) {
      buffer = camera.WaitFrame(1500000);
      if (buffer == NULL) {
        std::cout << "NULL" << std::endl;
        return 1;
      }
      std::cout << "Received " << buffer->size << " bytes" << std::endl;
/*      std::ofstream outfile("test.raw", std::ofstream::binary);
      outfile.write((char*) buffer->mem, buffer->size);
      outfile.close();*/
      camera.FreeFrame(buffer);
    }
    camera.Stop();
  } catch (std::string& e) {
    std::cout << "ERROR: " << e << std::endl;
  }
  camera.Close();
  return 0;
}
