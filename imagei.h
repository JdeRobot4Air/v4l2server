/*
 * imagei.h
 *
 *  Created on: 16/04/2015
 *      Author: redstar
 */

#ifndef JDEROBOT_COMPONENTS_IMAGEI_H_
#define JDEROBOT_COMPONENTS_IMAGEI_H_

#include <IceUtil/IceUtil.h>
#include <list>

#include <jderobot/camera.h>
#include <jderobot/image.h>
#include <jderobot/datetime.h>

#include "v4l2.h"

namespace cameraserver {

class ReplyTask;
class CameraI;

class ReplyTask : public IceUtil::Thread {
 private:
  CameraI* mycamera;
  IceUtil::Mutex requestsMutex;
  std::list<jderobot::AMD_ImageProvider_getImageDataPtr> requests;

 public:
  ReplyTask(CameraI* camera);
  void pushJob(const jderobot::AMD_ImageProvider_getImageDataPtr& cb);
  virtual void run();
};

class CameraI : virtual public jderobot::Camera {

 public:

  std::string device_name;
  int fps;
  v4l2::Camera* camera;
  v4l2::Format* format;
  v4l2::Buffer* buffer;
  typedef IceUtil::Handle<ReplyTask> ReplyTaskPtr;
  std::string prefix;
  jderobot::ImageDescriptionPtr imageDescription;
  jderobot::CameraDescriptionPtr cameraDescription;
  ReplyTaskPtr replyTask;
  bool rpc_mode;
  jderobot::ImageConsumerPrx imageConsumer;
  int mirror;

  CameraI(std::string propertyPrefix, Ice::CommunicatorPtr ic);
  std::string getName();
  virtual ~CameraI();
  virtual jderobot::ImageDescriptionPtr getImageDescription(
      const Ice::Current& c);
  virtual jderobot::CameraDescriptionPtr getCameraDescription(
      const Ice::Current& c);
  virtual Ice::Int setCameraDescription(
      const jderobot::CameraDescriptionPtr &description, const Ice::Current& c);
  void getImageData_async(const jderobot::AMD_ImageProvider_getImageDataPtr& cb,
                          const Ice::Current& c);
  virtual std::string startCameraStreaming(const Ice::Current&);
  virtual void stopCameraStreaming(const Ice::Current&);
  virtual void reset(const Ice::Current&);
};
}

#endif /* JDEROBOT_COMPONENTS_IMAGEI_H_ */
