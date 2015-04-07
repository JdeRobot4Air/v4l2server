/*
 * v4l2.h
 *
 *  Created on: 07/04/2015
 *      Author: redstar
 */

#ifndef JDEROBOT_COMPONENTS_V4L2SERVER_V4L2_H_
#define JDEROBOT_COMPONENTS_V4L2SERVER_V4L2_H_

namespace v4l2 {
class Camera {
 private:
  /* Camera device name */
  const char *device;
  /* Image width */
  int width;
  /* Image height */
  int height;
  /* Frames per second requested */
  int fps;
  /* Camera active (capturing frames) */
  bool active;

 public:
  Camera(const char *device, int weight, int height, int fps = 10);
  void Start();
  void Stop();
  ~Camera();
};
}

#endif /* JDEROBOT_COMPONENTS_V4L2SERVER_V4L2_H_ */
