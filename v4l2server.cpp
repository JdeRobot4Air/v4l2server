/*
 * v4l2server.cpp
 *
 *  Created on: 07/04/2015
 *      Author: Oscar Javier Garcia Baudet
 */

#include <iostream>

#include "v4l2.h"

int main(int argc, char** argv) {
  v4l2::Camera camera = v4l2::Camera("/dev/video1", 320, 240, 10);
  try {
    camera.Start();
    std::cout << "Get ready! -> " << camera.is_active() << std::endl;
  } catch (std::string& e) {
    std::cout << "ERROR: " << e << std::endl;
  }
  camera.Stop();
  return 0;
}
