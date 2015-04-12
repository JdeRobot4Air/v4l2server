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
  v4l2::Camera* camera;
  try {
    std::cout << "Creating Camera instance" << std::endl;
    format->width = 320;
    format->height = 240;
    format->format = "MJPG";  // YUYV MJPG
    camera = new v4l2::Camera("/dev/video0", format, 5);
    std::cout << "Opening camera device" << std::endl;
    camera->Open();
    /* Example 1: Listing all image formats, resolutions and frame rates */
    std::cout << "Listing image formats:" << std::endl;
    int index = 0;
    while (camera->EnumFormats(format, index)) {
      std::cout << "[" << index << "] Listing resolutions for image format: "
                << format->format << std::endl;
      int index2 = 0;
      while (camera->EnumResolutions(format, index2)) {
        int index3 = 0;
        while (camera->EnumFps(format, index3)) {
          std::cout << "[" << index << "," << index2 << "," << index3 << "]: "
                    << format->format << " " << format->width << "x"
                    << format->height << " @" << format->fps << "fps"
                    << std::endl;
          index3++;
        }
        index2++;
      }
      index++;
    }

    /* Let's get actual format, resolution and frame rate */
    camera->GetFormat(format);
    camera->GetFps(format);
    std::cout << "Previrous image format: " << format->format << " "
              << format->width << "x" << format->height << " @" << format->fps
              << std::endl;

    /* Desired image format */
    format->width = 320;
    format->height = 240;
    format->format = "MJPG";  // YUYV MJPG
    camera->Start();
    std::cout << "Camera started! -> " << camera->is_active() << std::endl;
    std::ofstream outfile("test.mjpg", std::ofstream::binary);
    std::cout << "Writing frames to file";
    std::cout.flush();
    for (int i = 0; i < 20; i++) {
      buffer = camera->WaitFrame(1500000);
      if (buffer == NULL) {
        std::cout << "WaitFrame returns NULL pointer" << std::endl;
        return 1;
      }
      /*std::cout << "Received " << buffer->size << " bytes" << std::endl;*/
      std::cout << ".";
      std::cout.flush();
      outfile.write((char*) buffer->mem, buffer->size);
      camera->FreeFrame(buffer);
    }
    outfile.close();
    std::cout << std::endl << "Ended writing file" << std::endl;
    camera->Stop();
  } catch (std::string& e) {
    std::cout << "ERROR: " << e << std::endl;
  }
  camera->Close();
  return 0;
}
