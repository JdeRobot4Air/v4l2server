/*
 * v4l2.cpp
 *
 *  Created on: 07/04/2015
 *      Author: redstar
 */

#include <errno.h>
#include <fcntl.h>
#include <sstream>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "v4l2.h"

namespace v4l2 {

Camera::Camera(std::string device, int width, int height) {
  Camera(device, width, height, 10);
}

Camera::Camera(std::string device, int width, int height, int fps) {
  /* Camera not active yet */
  active_ = false;
  /* Save camera data */
  device_ = device;
  width_ = width;
  height_ = height;
  fps_ = fps;
  camera_fd_ = -1;
}

void Camera::Start() throw (std::string) {
  struct stat st;
  /* Get stat data from device file */
  if (stat(device_.c_str(), &st) == -1) {
    std::ostringstream output_message;
    output_message << "Couldn't stat " << device_ << " file.";
    throw std::string(output_message.str());
  }
  /* Is a character special device file? */
  if (!S_ISCHR(st.st_mode)) {
    std::ostringstream output_message;
    output_message << "File " << device_ << " is not a device.";
    throw std::string(output_message.str());
  }
  /* Open device file */
  camera_fd_ = open(device_.c_str(), O_RDWR | O_NONBLOCK, 0);
  if (camera_fd_ == -1) {
    std::ostringstream output_message;
    output_message << "Can't open " << device_ << ": [" << errno << "] "
                   << strerror(errno);
  }
  active_ = true;
}

void Camera::Stop() {
  active_ = false;
  /* If camera file descriptor is opened we will close it on stop */
  if (camera_fd_ != -1) {
    close(camera_fd_);
    camera_fd_ = -1;
  }
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
