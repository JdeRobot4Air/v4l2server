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

/**
 * Constructor without frame rate
 * This constructor will create a instance of camera with a default frame rate
 * of 10 fps
 * @param device camera device to be opened
 * @param width image width
 * @param height image height
 */
Camera::Camera(std::string device, int width, int height) {
  Camera(device, width, height, 10);
}

/**
 * Constructor with frame rate
 * This constructor will create a instance of camera with requested image size
 * and frame rate
 * @param device camera device to be opened
 * @param width image width
 * @param height image height
 * @param fps requested frame rate in frames per second
 */
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

/**
 * Open camera device and throws an exception in case of error
 */
void Camera::Open() throw (std::string) {
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
}

/**
 * Close camera device
 */
void Camera::Close() {
  active_ = false;
  /* If camera file descriptor is opened we will close it on stop */
  if (camera_fd_ != -1) {
    close(camera_fd_);
    camera_fd_ = -1;
  }
}

/**
 * Get camera status
 * @return the current status of camera device
 */
bool Camera::is_active() {
  return active_ && (camera_fd_ != -1);
}

/**
 * Free resources, stop capturing and close camera device
 */
Camera::~Camera() {
  Close();
}

}
