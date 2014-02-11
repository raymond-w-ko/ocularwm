#pragma once

#include "VirtualMonitor.hpp"

class ScreenshotProducer;

class VirtualMonitorManager {
 public:
  VirtualMonitorManager(
      ScreenshotProducer* screenshot_producer,
      Ogre::SceneManager* scene);
  ~VirtualMonitorManager();

  void Update();

 private:
  ScreenshotProducer* mScreenshotProducer;
  Ogre::SceneManager* mScene;

  std::unordered_map<HWND, VirtualMonitorPtr> mRootMonitors;
};
typedef std::shared_ptr<VirtualMonitorManager> VirtualMonitorManagerPtr;
