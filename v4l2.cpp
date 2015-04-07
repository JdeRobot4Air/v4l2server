/*
 * v4l2.cpp
 *
 *  Created on: 07/04/2015
 *      Author: redstar
 */

#include "v4l2.h"

namespace v4l2 {

Camera::Camera(const char *device, int weigth, int height, int fps = 10) {
  /* Camera not active yet */
  this->active = false;
  /* Save camera data */
  this->device = device;
  this->width = width;
  this->height = height;
  this->fps = fps;
}

void Camera::Start() {
  active = true;
}

void Camera::Stop() {
  active = false;
}

Camera::~Camera() {
  /* Stop and close camera, free buffers, ect */
  if (active == true) {
    Stop();
  }
}

}
