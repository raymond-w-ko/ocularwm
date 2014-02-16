#pragma once

#include "VirtualMonitor.hpp"

class ScreenshotProducer;

class VirtualMonitorManager {
 public:
  VirtualMonitorManager(
      ScreenshotProducer* screenshot_producer,
      Ogre::SceneManager* scene);
  ~VirtualMonitorManager();

  void Update(double frame_time);

 private:
  enum class UpdateStage {
    UpdateLiveWindowsSet,
    InitDeadWindowSet,
    BlitWindows,
    RemoveDeadWindows,
    UpdateWindowSRT,
  };
  UpdateStage mStage;

  ScreenshotProducer* mScreenshotProducer;
  Ogre::SceneManager* mScene;

  std::unordered_map<WindowID, VirtualMonitorPtr> mRootMonitors;
  std::unordered_map<WindowID, VirtualMonitorPtr> mChildMonitors;

  std::vector<WindowID> mLiveHwnds;
  size_t mLiveHwndIndex;
  std::unordered_set<WindowID> mDeadMonitors;
};
typedef std::shared_ptr<VirtualMonitorManager> VirtualMonitorManagerPtr;
