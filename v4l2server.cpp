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
  v4l2::Camera camera = v4l2::Camera("/dev/video0", 320, 240, 10);
  v4l2::Buffer* buffer = NULL, * buffer2;
  try {
    camera.Open();
    std::cout << "Get ready! -> " << camera.is_active() << std::endl;
    camera.Start();
    std::cout << "Camera started! -> " << camera.is_active() << std::endl;
    for (int i = 0; i < 20; i++) {
      buffer = camera.WaitFrame(500000);
      if (buffer == NULL) {
        std::cout << "NULL" << std::endl;
        return 1;
      }
      std::cout << v4l2::FormatString2Int("YUYV") << " - " << v4l2::FormatString2Int2("YUYV") << " - " << v4l2::FormatInt2String(859981650) << std::endl;
      std::cout << "Received " << buffer->size << " bytes" << std::endl;
      std::ofstream outfile("test.raw", std::ofstream::binary);
      outfile.write((char*) buffer->mem, buffer->size);
      outfile.close();
      camera.FreeFrame(buffer);
    }
    camera.Stop();
  } catch (std::string& e) {
    std::cout << "ERROR: " << e << std::endl;
  }
  camera.Close();
  return 0;
}
