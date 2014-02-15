#pragma once

#include "VirtualMonitor.hpp"

class ScreenshotProducer;

class VirtualMonitorManager {
 public:
  VirtualMonitorManager(
      ScreenshotProducer* screenshot_producer,
      Ogre::SceneManager* scene);
  ~VirtualMonitorManager();

  void Update(float frame_time);

 private:
  ScreenshotProducer* mScreenshotProducer;
  Ogre::SceneManager* mScene;

  std::unordered_map<HWND, VirtualMonitorPtr> mRootMonitors;
  std::unordered_map<HWND, VirtualMonitorPtr> mChildMonitors;
};
typedef std::shared_ptr<VirtualMonitorManager> VirtualMonitorManagerPtr;
