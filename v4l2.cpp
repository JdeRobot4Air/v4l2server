/*
 * v4l2.cpp
 *
 *  Created on: 07/04/2015
 *      Author: redstar
 */

#include <errno.h>
#include <fcntl.h>
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <string>

#include "v4l2.h"

namespace v4l2 {

std::string FormatInt2String(int format) {
  std::string output;
  output += (char) (format & 0xff);
  format >>= 8;
  output += (char) (format & 0xff);
  format >>= 8;
  output += (char) (format & 0xff);
  format >>= 8;
  output += (char) (format & 0xff);
  return output;
}

int FormatString2Int(std::string format) {
  int output;
  if (format.size() != 4) {
    return 0;
  }
  output = format.at(0);
  output += format.at(1) << 8;
  output += format.at(2) << 16;
  output += format.at(3) << 24;
  return output;
}

void Camera::GetFormat(Format *format) throw (std::string) {
  /* Get image format */
  struct v4l2_format* image_format = new v4l2_format();
  image_format->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (int rc = xioctl(VIDIOC_G_FMT, image_format) == -1) {
    std::ostringstream output_message;
    output_message << "Error in VIDIOC_G_FMT (rc=" << rc << ", errno="
                   << errno << ", " << strerror(errno) << ")";
    throw std::string(output_message.str());
  }
  format->format = FormatInt2String(image_format->fmt.pix.pixelformat);
  format->width = image_format->fmt.pix.width;
  format->height = image_format->fmt.pix.height;
}

void Camera::SetFormat(Format *format) throw (std::string) {
  /* Set image format */
  struct v4l2_format* image_format = new v4l2_format();
  int int_format = FormatString2Int(format->format);
  image_format->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  image_format->fmt.pix.width = format->width;
  image_format->fmt.pix.height = format->height;
  image_format->fmt.pix.pixelformat = int_format;
  image_format->fmt.pix.field = V4L2_FIELD_NONE;
  if (xioctl(VIDIOC_S_FMT, image_format) == -1) {
    throw std::string("Error in VIDIOC_S_FMT");
  }
  if (image_format->fmt.pix.pixelformat != int_format) {
    throw std::string("Camera doesn't support requested mode");
  }
}

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
Camera::Camera(std::string device, Format* format) {
  Camera(device, format);
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
Camera::Camera(std::string device, Format* format, int fps) {
  /* Camera not active yet */
  active_ = false;
  /* Save camera data */
  device_ = device;
  /* Save camera format */
  format_ = new Format;
  format_->width = format->width;
  format_->height = format->height;
  format_->format = format->format;
  format_->fps = fps;
  camera_fd_ = -1;
  num_buffers_ = 4;
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
  camera_fd_ = open(device_.c_str(), O_RDWR /*| O_NONBLOCK*/, 0);
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
  SetFormat(format_);
  /* Set streaming parameters */
  SetFps(format_);
  std::cout << "FPS: " << format_->fps << std::endl;
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
  buffers_ = (Buffer *) calloc(request_buffers.count, sizeof(*buffers_));

  if (buffers_ == NULL) {
    throw std::string("Not enough memory to allocate memory shared buffers");
  }

  for (int num_buffer = 0; num_buffer < (int) request_buffers.count;
      num_buffer++) {
    struct v4l2_buffer buffer;
    memset(&buffer, 1, sizeof(buffer));
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = num_buffer;

    if (xioctl(VIDIOC_QUERYBUF, &buffer) == -1) {
      throw std::string("Error in VIDIOC_QUERYBUF");
    }
    buffers_[num_buffer].size = buffer.length;
    buffers_[num_buffer].mem = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, camera_fd_, buffer.m.offset);
    if (buffers_[num_buffer].mem == MAP_FAILED) {
      throw std::string("Error in mmap (MAP_FAILED)");
    }
  }
}

/**
 * Close camera device
 */
void Camera::Close() {
  active_ = false;
  for (int i = 0; i < (unsigned int) num_buffers_; ++i) {
    if (munmap(buffers_[i].mem, buffers_[i].size) == -1) {
      throw std::string("Error in munmap");
    }
  }
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

void Camera::EnqueueBuffer(int index) throw (std::string) {
  struct v4l2_buffer* buffer = new v4l2_buffer();
  buffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer->memory = V4L2_MEMORY_MMAP;
  buffer->index = index;
  if (xioctl(VIDIOC_QBUF, buffer) == -1) {
    throw std::string("(EnqueueBuffer) Error in VIDIOC_QBUF");
  }
}

int Camera::DequeueBuffer() throw (std::string) {
  struct v4l2_buffer* buffer = new v4l2_buffer();
  buffer->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buffer->memory = V4L2_MEMORY_MMAP;
  if (xioctl(VIDIOC_DQBUF, &buffer) == -1) {
    std::ostringstream output_message;
    output_message << "(DequeueBuffer) Error reading device " << device_
                   << ": [" << errno << "] " << strerror(errno);
    throw std::string(output_message.str());
  }
  if (buffer->index >= num_buffers_) {
    throw std::string("(DequeueBuffer) mmap index returned out of range");
  }
}

void Camera::Start() throw (std::string) {
  unsigned int i;
  enum v4l2_buf_type type;
  /* Enqueue all buffers */
  for (i = 0; i < (unsigned int) num_buffers_; ++i) {
    EnqueueBuffer(i);
  }
  /* Final and more important step: start streaming images */
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (xioctl(VIDIOC_STREAMON, &type) == -1) {
    throw std::string("Error in VIDIOC_STREAMON");
  }
  current_buffer_.length = 0;
}

void Camera::Stop() throw (std::string) {
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (xioctl(VIDIOC_STREAMOFF, &type) == -1) {
    throw std::string("Error stopping camera streaming");
  }
}

/** Uses poll to wait until next frame is ready */
Buffer* Camera::WaitFrame(int milliseconds) throw (std::string) {
  std::ostringstream output_message;
  /* Check if previous frame was requeued before dequeue next one */
  if (current_buffer_.length != 0) {
    throw std::string("ERROR: Previous frame wasn't freed!");
  }
  Buffer* frame;
  struct pollfd ufds[1];
  ufds[0].fd = camera_fd_;
  ufds[0].events = POLLIN;

  int result = poll(ufds, 1, milliseconds);
  switch (result) {
    case -1:
      output_message << "Error waiting for device " << device_ << ": ["
                     << errno << "] " << strerror(errno);
      throw std::string(output_message.str());
    case 0:
      return NULL;
    case 1:
      if (ufds[0].revents & POLLIN) {
        memset(&current_buffer_, 1, sizeof(current_buffer_));
        current_buffer_.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        current_buffer_.memory = V4L2_MEMORY_MMAP;
        if (xioctl(VIDIOC_DQBUF, &current_buffer_) == -1) {
          output_message << "Error reading device " << device_ << ": ["
                         << errno << "] " << strerror(errno);
          throw std::string(output_message.str());
        }
        if (current_buffer_.index >= num_buffers_) {
          throw std::string("ERROR: mmap index returned out of range");
        }
        /* Get pointer and size of data */
        //frame = (Buffer*) calloc(1, sizeof(frame));
        frame = new Buffer;
        frame->mem = buffers_[current_buffer_.index].mem;
        frame->size = current_buffer_.bytesused;
        return frame;
      }
      break;
    default:
      throw std::string(
          "Unexpected number of file descriptors modified: " + result);
  }
  return NULL;
}

void Camera::FreeFrame(Buffer* frame) throw (std::string) {
  //free(frame);
  if (xioctl(VIDIOC_QBUF, &current_buffer_) == -1) {
    throw std::string("Error in VIDIOC_QBUF");
  }
  current_buffer_.length = 0;
}

Buffer* Camera::YuyvToRgb24(Buffer* frame) {
  Buffer* output;
  unsigned char* rgb_image = new unsigned char[format_->width * format_->height
      * 3];
  unsigned char* yuyv_image = (unsigned char*) frame->mem;

  int i, j;
  int y, cr, cb;
  double r, g, b;

  for (i = 0, j = 0; i < format_->width * format_->height * 3; i += 6, j += 4) {
    //first pixel
    y = yuyv_image[j];
    cb = yuyv_image[j + 1];
    cr = yuyv_image[j + 3];

    r = y + (1.4065 * (cr - 128));
    g = y - (0.3455 * (cb - 128)) - (0.7169 * (cr - 128));
    b = y + (1.7790 * (cb - 128));

    //This prevents colour distortions in your rgb image
    if (r < 0)
      r = 0;
    else if (r > 255)
      r = 255;
    if (g < 0)
      g = 0;
    else if (g > 255)
      g = 255;
    if (b < 0)
      b = 0;
    else if (b > 255)
      b = 255;

    rgb_image[i] = r;
    rgb_image[i + 1] = g;
    rgb_image[i + 2] = b;

    //second pixel
    y = yuyv_image[j + 2];
    cb = yuyv_image[j + 1];
    cr = yuyv_image[j + 3];

    r = y + (1.4065 * (cr - 128));
    g = y - (0.3455 * (cb - 128)) - (0.7169 * (cr - 128));
    b = y + (1.7790 * (cb - 128));

    if (r < 0)
      r = 0;
    else if (r > 255)
      r = 255;
    if (g < 0)
      g = 0;
    else if (g > 255)
      g = 255;
    if (b < 0)
      b = 0;
    else if (b > 255)
      b = 255;

    rgb_image[i + 3] = (unsigned char) r;
    rgb_image[i + 4] = (unsigned char) g;
    rgb_image[i + 5] = (unsigned char) b;
  }
  output->mem = (void*) rgb_image;
  output->size = i;
  return output;
}

/**
 * Free resources, stop capturing and close camera device
 */
Camera::~Camera() {
  Close();
}

bool Camera::EnumFormats(Format* format, int index) throw (std::string) {
  struct v4l2_fmtdesc* format_description = new v4l2_fmtdesc();
  format_description->index = index;
  format_description->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (int rc = xioctl(VIDIOC_ENUM_FMT, format_description) != 0) {
    if (rc == -1) {
      std::ostringstream output_message;
      output_message << "Error in VIDIOC_ENUM_FMT (rc=" << rc << ", errno="
                     << errno << ", " << strerror(errno) << ")";
      throw std::string(output_message.str());
    }
    return false;
  }
  format->format = FormatInt2String(format_description->pixelformat);
  return true;
}

bool Camera::EnumResolutions(Format* format, int index) throw (std::string) {
  struct v4l2_frmsizeenum* format_size = new v4l2_frmsizeenum();
  format_size->index = index;
  format_size->pixel_format = FormatString2Int(format->format);
  if (int rc = xioctl(VIDIOC_ENUM_FRAMESIZES, format_size) != 0) {
    if (rc == -1) {
      std::ostringstream output_message;
      output_message << "Error in VIDIOC_ENUM_FRAMESIZES (rc=" << rc
                     << ", errno=" << errno << ", " << strerror(errno) << ")";
      throw std::string(output_message.str());
    }
    return false;
  }
  if (format_size->type == V4L2_FRMSIZE_TYPE_DISCRETE) {
    format->width = format_size->discrete.width;
    format->height = format_size->discrete.height;
  } else if (format_size->type == V4L2_FRMSIZE_TYPE_STEPWISE) {
    format->width = format_size->stepwise.max_width;
    format->height = format_size->stepwise.max_height;
  } else {
    return false;
  }
  return true;
}

bool Camera::EnumFps(Format* format, int index) throw (std::string) {
  struct v4l2_frmivalenum* image_fps = new v4l2_frmivalenum();
  image_fps->index = index;
  image_fps->width = format->width;
  image_fps->height = format->height;
  image_fps->pixel_format = FormatString2Int(format->format);
  if (int rc = xioctl(VIDIOC_ENUM_FRAMEINTERVALS, image_fps) != 0) {
    if (rc == -1) {
      std::ostringstream output_message;
      output_message << "Error in VIDIOC_ENUM_FRAMEINTERVALS (rc=" << rc
                     << ", errno=" << errno << ", " << strerror(errno) << ")";
      throw std::string(output_message.str());
    }
    return false;
  }
  if (image_fps->type == V4L2_FRMIVAL_TYPE_DISCRETE) {
    format->fps = image_fps->discrete.denominator;
  } else if (image_fps->type == V4L2_FRMSIZE_TYPE_STEPWISE
      || image_fps->type == V4L2_FRMSIZE_TYPE_CONTINUOUS) {
    format->fps = image_fps->stepwise.max.denominator;
  } else {
    return false;
  }
  return true;
}

void Camera::SetFps(Format *format) throw (std::string) {
  /* Set streaming parameters */
  struct v4l2_streamparm* streaming_params = new v4l2_streamparm();
  streaming_params->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  streaming_params->parm.capture.timeperframe.numerator = 1;
  streaming_params->parm.capture.timeperframe.denominator = format->fps;
  streaming_params->parm.output.timeperframe.numerator = 1;
  streaming_params->parm.output.timeperframe.denominator = format->fps;
  if (xioctl(VIDIOC_S_PARM, streaming_params) == -1) {
    throw std::string("Error in VIDIOC_S_PARM");
  }
  format->fps = streaming_params->parm.capture.timeperframe.denominator;
}

void Camera::GetFps(Format *format) throw (std::string) {
  /* Get streaming parameters */
  struct v4l2_streamparm* streaming_params = new v4l2_streamparm();
  streaming_params->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (xioctl(VIDIOC_S_PARM, streaming_params) == -1) {
    throw std::string("Error in VIDIOC_S_PARM");
  }
  format->fps = streaming_params->parm.capture.timeperframe.denominator;
}

}
