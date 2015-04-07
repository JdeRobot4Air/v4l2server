/*
 * v4l2.cpp
 *
 *  Created on: 07/04/2015
 *      Author: redstar
 */

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "v4l2.h"

namespace v4l2 {

Camera::Camera(const char *device, int weigth, int height, int fps = 10) {
  /* Camera not active yet */
  this->active_ = false;
  /* Save camera data */
  this->device_ = device;
  this->width_ = width_;
  this->height_ = height;
  this->fps_ = fps;
}

void Camera::Start() {
  struct stat st;
  /* Get stat data from device file */
  if (stat(device_, &st) == -1) {
    throw std::string("Couldn't stat " << device_ << " file.");
  }
  /* Is a character special device file? */
  if (!S_ISCHR(st.st_mode)) {
    throw std::string("File " << device_ << " is not a device.");
  }
  /* Open device file */
  camera_fd_ = open(device_, O_RDWR | O_NONBLOCK, 0);
  if (camera_fd_ == -1) {
    throw std::string(
        "Can't open " << device_ << ": [" << errno << "] " << strerror(errno));
  }
  active_ = true;
}

void Camera::Stop() {
  active_ = false;
}

bool Camera::is_active() {
  return active_;
}

Camera::~Camera() {
  /* Stop and close camera, free buffers, ect */
  if (active_ == true) {
    Stop();
  }
}

}
