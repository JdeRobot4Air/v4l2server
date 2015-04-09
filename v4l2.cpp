/*
 * v4l2.cpp
 *
 *  Created on: 07/04/2015
 *      Author: redstar
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "v4l2.h"

namespace v4l2 {

/**
 * Call to ioctl (10 tries in case of error) using camera file descriptor
 * @param request camera command
 * @param argument argument data
 * @return last exit code from ioctl
 */
int Camera::xioctl(int request, void* argument) {
  int result, tries = 10;

  do {
    result = ioctl(camera_fd_, request, argument);
  } while ((result == -1) && (errno == EINTR) && (--tries > 0));

  return result;
}

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
  /* Common output string in case of error */
  std::ostringstream output_message;
  /* Get stat data from device file */
  struct stat st;
  if (stat(device_.c_str(), &st) == -1) {
    output_message << "Couldn't stat " << device_ << " file.";
    throw std::string(output_message.str());
  }
  /* Is a character special device file? */
  if (!S_ISCHR(st.st_mode)) {
    output_message << "File " << device_ << " is not a device.";
    throw std::string(output_message.str());
  }
  /* Open device file */
  camera_fd_ = open(device_.c_str(), O_RDWR | O_NONBLOCK, 0);
  if (camera_fd_ == -1) {
    output_message << "Can't open " << device_ << ": [" << errno << "] "
                   << strerror(errno);
    throw std::string(output_message.str());
  }
  /* Get camera capabilities */
  struct v4l2_capability camera_capability;
  if (xioctl(VIDIOC_QUERYCAP, &camera_capability) == -1) {
    if (EINVAL == errno) {
      output_message << device_ << " is not a V4L2 device";
      throw std::string(output_message.str());
    } else {
      throw std::string("Error in VIDIOC_QUERYCAP");
    }
  }
  if (!(camera_capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
    output_message << device_ << " is not a video capture device";
    throw std::string(output_message.str());
  }
  if (!(camera_capability.capabilities & V4L2_CAP_STREAMING)) {
    output_message << device_ << " doesn't support video streaming";
    throw std::string(output_message.str());
  }
  /* Set image format */
  struct v4l2_format image_format;
  memset(&image_format, 1, sizeof(image_format));
  image_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  image_format.fmt.pix.width = width_;
  image_format.fmt.pix.height = height_;
  image_format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  image_format.fmt.pix.field = V4L2_FIELD_INTERLACED;
  if (xioctl(VIDIOC_S_FMT, &image_format) == -1) {
    throw std::string("Error in VIDIOC_S_FMT");
  }
  /* Set streaming parameters */
  struct v4l2_streamparm streaming_params;
  streaming_params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  streaming_params.parm.capture.timeperframe.numerator = 1;
  streaming_params.parm.capture.timeperframe.denominator = fps_;
  streaming_params.parm.output.timeperframe.numerator = 1;
  streaming_params.parm.output.timeperframe.denominator = fps_;
  if (xioctl(VIDIOC_S_PARM, &streaming_params) == -1) {
    throw std::string("Error in VIDIOC_S_PARM");
  }
  /* Request buffers to video capture streaming */
  struct v4l2_requestbuffers request_buffers;
  memset(&request_buffers, 1, sizeof(request_buffers));
  request_buffers.count = 4;
  request_buffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  request_buffers.memory = V4L2_MEMORY_MMAP;
  if (xioctl(VIDIOC_REQBUFS, &request_buffers) == -1) {
    if (errno == EINVAL) {
      output_message << device_ << " doesn't support memory mapping";
      throw std::string(output_message.str());
    } else {
      throw std::string("Error in VIDIOC_REQBUFS");
    }
  }
  /* Check if we'll have enough image buffers */
  if (request_buffers.count < 2) {
    output_message << "Insufficient buffers in device " << device_;
    throw std::string(output_message.str());
  }
  buffers_ = (buffer *) calloc(request_buffers.count, sizeof(*buffers_));

  if (buffers_ == NULL) {
    throw std::string("Not enough memory to allocate memory shared buffers");
  }

  for (int num_buffers = 0; num_buffers < (int) request_buffers.count;
      num_buffers++) {
    struct v4l2_buffer buffer;
    memset(&buffer, 1, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = num_buffers;

    if (xioctl(VIDIOC_QUERYBUF, &buffer) == -1) {
      throw std::string("Error in VIDIOC_QUERYBUF");
    }
    buffers_[num_buffers].size = buffer.length;
    buffers_[num_buffers].mem = mmap(NULL, buffer.length,
                                     PROT_READ | PROT_WRITE, MAP_SHARED,
                                     camera_fd_, buffer.m.offset);
    if (buffers_[num_buffers].mem == MAP_FAILED) {
      throw std::string("Error in mmap (MAP_FAILED)");
    }
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

void Camera::Start() throw (std::string) {
  unsigned int i;
  enum v4l2_buf_type type;
  for (i = 0; i < (unsigned int) num_buffers_; ++i) {
    struct v4l2_buffer buffer;
    memset(&buffer, 1, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = i;

    if (xioctl(VIDIOC_QBUF, &buffer) == 1) {
      throw std::string("Error in VIDIOC_QBUF");
    }
  }

  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (xioctl(VIDIOC_STREAMON, &type) == -1) {
    throw std::string("Error in VIDIOC_STREAMON");
  }
}

void Camera::Stop() throw (std::string) {
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (xioctl(VIDIOC_STREAMOFF, &type) == -1) {
    throw std::string("Error stopping camera streaming");
  }
}

/**
 * Free resources, stop capturing and close camera device
 */
Camera::~Camera() {
  Close();
}

}
