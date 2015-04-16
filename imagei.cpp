/*
 * imagei.cpp
 *
 *  Created on: 16/04/2015
 *      Author: redstar
 */

#include <Ice/Ice.h>

#include "imagei.h"

namespace cameraserver {

CameraI::CameraI(std::string propertyPrefix, Ice::CommunicatorPtr ic)
      : prefix(propertyPrefix),
        imageConsumer(),
        rpc_mode(false),
        format() {

    std::cout << "Constructor CameraI -> " << propertyPrefix << std::endl;

    imageDescription = new jderobot::ImageDescription();
    cameraDescription = new jderobot::CameraDescription();

    Ice::PropertiesPtr prop = ic->getProperties();

    /* Fill cameraDescription */
    cameraDescription->name = prop->getPropertyWithDefault(prefix + "Name", "");
    cameraDescription->shortDescription = prop->getPropertyWithDefault(
        prefix + "ShortDescription", "");
    cameraDescription->streamingUri = prop->getPropertyWithDefault(
        prefix + "StreamingUri", "");

    /* Fill imageDescription */
    format->width = imageDescription->width = prop->getPropertyAsIntWithDefault(
        prefix + "ImageWidth", 320);
    format->height = imageDescription->height = prop
        ->getPropertyAsIntWithDefault(prefix + "ImageHeight", 240);

    /* We need to translate V4L2 formats to colorspaces string format */
    std::string fmtStr = prop->getPropertyWithDefault(prefix + "Format",
                                                      "RGB8");
    /* We only supports those formats that we could resolve to V4L2 */
    // convert colorspaces to V4L2 and throw error if we don't know it
    format->format = "YUYV";

    /*********************************************************************
     imageDescription->size = imageDescription->width * imageDescription->height
     * CV_ELEM_SIZE(imageFmt->cvType);
     imageDescription->format = imageFmt->name;
     ***********************************************************************/

    /* Get camera device */
    device_name = prop->getProperty(prefix + "Uri");
    fps = prop->getPropertyAsIntWithDefault(prefix + "fps", 10);

    std::cout << "Device name: " << device_name << std::endl;

    camera = new v4l2::Camera(device_name, format, fps);

    replyTask = new ReplyTask(this);
    replyTask->start();  // my own thread
  }

  std::string CameraI::getName() {
    return (cameraDescription->name);
  }

  CameraI::~CameraI() {
  }

  jderobot::ImageDescriptionPtr CameraI::getImageDescription(
      const Ice::Current& c) {
    return imageDescription;
  }

  jderobot::CameraDescriptionPtr CameraI::getCameraDescription(
      const Ice::Current& c) {
    return cameraDescription;
  }

  Ice::Int CameraI::setCameraDescription(
      const jderobot::CameraDescriptionPtr &description,
      const Ice::Current& c) {
    return 0;
  }

  void CameraI::getImageData_async(
      const jderobot::AMD_ImageProvider_getImageDataPtr& cb,
      const Ice::Current& c) {
    replyTask->pushJob(cb);
  }

  std::string CameraI::startCameraStreaming(const Ice::Current&) {
    return "";
  }

  void CameraI::stopCameraStreaming(const Ice::Current&) {
  }

  void CameraI::reset(const Ice::Current&) {
  }


  ReplyTask::ReplyTask(CameraI* camera) {
      std::cout << "safeThread" << std::endl;
      mycamera = camera;
    }

    void ReplyTask::pushJob(const jderobot::AMD_ImageProvider_getImageDataPtr& cb) {
      IceUtil::Mutex::Lock sync(requestsMutex);
      requests.push_back(cb);
    }

    void ReplyTask::run() {
      jderobot::ImageDataPtr reply(new jderobot::ImageData);
      ::jderobot::Time t;
      while (1) {
        IceUtil::Time t = IceUtil::Time::now();
        reply->timeStamp.seconds = (long) t.toSeconds();
        reply->timeStamp.useconds = (long) t.toMicroSeconds()
            - reply->timeStamp.seconds * 1000000;
        {  //critical region start
          IceUtil::Mutex::Lock sync(requestsMutex);
          while (!requests.empty()) {
            jderobot::AMD_ImageProvider_getImageDataPtr cb = requests.front();
            requests.pop_front();
            cb->ice_response(reply);
          }
        }
      }
    }

}  //namespace

