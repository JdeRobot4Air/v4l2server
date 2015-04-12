/*
 * v4l2.h
 *
 *  Created on: 07/04/2015
 *      Author: redstar
 */

#ifndef JDEROBOT_COMPONENTS_V4L2SERVER_V4L2_H_
#define JDEROBOT_COMPONENTS_V4L2SERVER_V4L2_H_

#include <linux/videodev2.h>
#include <iostream>

namespace v4l2 {

struct Buffer {
  int index;
  void* mem;
  size_t size;
  size_t used;
};

struct Format {
  std::string format;
  int width;
  int height;
  int fps;
};

std::string FormatInt2String(int format);
int FormatString2Int(std::string format);

/** Camera control class */
class Camera {
 private:
  /** Camera device name */
  std::string device_;
  /** Camera file descriptor */
  int camera_fd_;
  /** Image format */
  Format* format_;
  /** Camera active (device opened and configured) */
  bool active_;
  /** Memory mapped image buffers */
  Buffer* buffers_;
  int num_buffers_;
  /** Current dequeued V4L2 buffer */
  struct v4l2_buffer current_buffer_;

 public:
  Buffer current_frame;

  int xioctl(int request, void* argument);
  void GetFormat(Format *format) throw (std::string);
  void SetFormat(Format *format) throw (std::string);
  void SetFps(Format *format) throw (std::string);
  void GetFps(Format *format) throw (std::string);
  Camera(std::string device, Format* format, int fps);
  Camera(std::string device, Format* format);
  void Open() throw (std::string);
  void Close();
  void Start() throw (std::string);
  void Stop() throw (std::string);
  bool is_active();
  Buffer* WaitFrame(int timeout) throw (std::string);
  void FreeFrame(Buffer* frame) throw (std::string);
  Buffer* YuyvToRgb24(Buffer* frame);
  ~Camera();
  void EnqueueBuffer(int index) throw (std::string);
  int DequeueBuffer() throw (std::string);
  bool EnumFormats(Format* format, int index) throw (std::string);
  bool EnumResolutions(Format* format, int index) throw (std::string);
  bool EnumFps(Format* format, int index) throw (std::string);
};

} /* namespace */

#endif /* JDEROBOT_COMPONENTS_V4L2SERVER_V4L2_H_ */
