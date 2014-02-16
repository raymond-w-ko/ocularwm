#include "StableHeaders.hpp"
#include "VirtualMonitorManager.hpp"
#include "ScreenshotProducer.hpp"

VirtualMonitorManager::VirtualMonitorManager(
    ScreenshotProducer* screenshot_producer,
    Ogre::SceneManager* scene)
    : mScreenshotProducer(screenshot_producer),
      mScene(scene),
      mStage(UpdateStage::UpdateLiveWindowsSet),
      mLiveHwndIndex(0) {
}

VirtualMonitorManager::~VirtualMonitorManager() {
}

static bool IsAltTabWindow(WindowID hwnd) {
  WindowID hwndWalk = ::GetAncestor(hwnd, GA_ROOTOWNER);

  WindowID lastHwndTry = 0;

  for (;;) {
    WindowID hwndTry = ::GetLastActivePopup(hwndWalk);
    if (hwndTry == lastHwndTry)
      break;
    lastHwndTry = hwndTry;
    if (IsWindowVisible(hwndTry))
      break;
    hwndWalk = hwndTry;
  }
  return hwndWalk == hwnd;
}

void VirtualMonitorManager::Update(double frame_time) {
  switch (mStage) {
    case UpdateStage::UpdateLiveWindowsSet: {
      // this shouldn't take much time so we just do this step to completion
      mLiveHwnds.clear();
      mLiveHwnds = mScreenshotProducer->GetVisibleWindows();
      // only proceed with next stage if we have live windows
      if (mLiveHwnds.size() > 0) {
        mStage = UpdateStage::InitDeadWindowSet;
        mLiveHwndIndex = 0;
      }
      break;
    }
    case UpdateStage::InitDeadWindowSet: {
      mDeadMonitors.clear();
      // populate from existing sets
      for (auto& iter : mRootMonitors) {
        mDeadMonitors.insert(iter.first);
      }
      for (auto& iter : mChildMonitors) {
        mDeadMonitors.insert(iter.first);
      }
      mStage = UpdateStage::BlitWindows;
      break;
    }
    case UpdateStage::BlitWindows: {
      WindowID win = mLiveHwnds[mLiveHwndIndex];
      mDeadMonitors.erase(win);

      ScreenshotPtr screenshot = mScreenshotProducer->Get(win);
      if (screenshot) {
        bool is_root = IsAltTabWindow(win);
        if (is_root) {
          // make sure it exists
          if (mRootMonitors.find(win) == mRootMonitors.end()) {
            mRootMonitors[win] = std::make_shared<VirtualMonitor>(mScene, win);
          }
          auto monitor = mRootMonitors[win];

          monitor->Blit(screenshot);
        }
      }

      mLiveHwndIndex++;
      if (mLiveHwndIndex >= mLiveHwnds.size()) {
        mStage = UpdateStage::RemoveDeadWindows;
      }
      break;
    }
    case UpdateStage::RemoveDeadWindows: {
      for (WindowID win : mDeadMonitors) {
        mRootMonitors.erase(win);
        mChildMonitors.erase(win);
      }
      mStage = UpdateStage::UpdateWindowSRT;
      break;
    }
    case UpdateStage::UpdateWindowSRT: {
      float num_root_monitors = static_cast<float>(mRootMonitors.size());
      const float r = 0.5f;

      int i = 0;
      for (auto iter : mRootMonitors) {
        auto monitor = iter.second;

        monitor->UpdateScale();

        float deg = 360.0f * (i / num_root_monitors);
        float rad = deg * 3.14159f / 180.0f;
        float x = r * cos(rad);
        float y = r * sin(rad);
        monitor->SetPosition(x, 0, -y);

        monitor->FocusAtUser();

        ++i;
      }
      mStage = UpdateStage::UpdateLiveWindowsSet;
      break;
    }
  }

}
