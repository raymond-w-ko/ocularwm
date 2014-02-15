#include "StableHeaders.hpp"
#include "VirtualMonitorManager.hpp"
#include "ScreenshotProducer.hpp"

VirtualMonitorManager::VirtualMonitorManager(
    ScreenshotProducer* screenshot_producer,
    Ogre::SceneManager* scene)
    : mScreenshotProducer(screenshot_producer),
      mScene(scene) {
}

VirtualMonitorManager::~VirtualMonitorManager() {
}

static bool IsAltTabWindow(HWND hwnd) {
  HWND hwndWalk = ::GetAncestor(hwnd, GA_ROOTOWNER);

  HWND lastHwndTry = 0;

  for (;;) {
    HWND hwndTry = ::GetLastActivePopup(hwndWalk);
    if (hwndTry == lastHwndTry)
      break;
    lastHwndTry = hwndTry;
    if (IsWindowVisible(hwndTry))
      break;
    hwndWalk = hwndTry;
  }
  return hwndWalk == hwnd;
}

void VirtualMonitorManager::Update(float frame_time) {
  auto hwnds = mScreenshotProducer->GetVisibleWindows();

  std::unordered_set<HWND> dead_hwnds;
  // populate from existing sets
  for (auto iter : mRootMonitors) {
    dead_hwnds.insert(iter.first);
  }
  for (auto iter : mChildMonitors) {
    dead_hwnds.insert(iter.first);
  }

  for (HWND hwnd : hwnds) {
    dead_hwnds.erase(hwnd);

    ScreenshotPtr screenshot = mScreenshotProducer->Get(hwnd);
    if (!screenshot) {
      continue;
    }

    bool is_root = IsAltTabWindow(hwnd);
    if (is_root) {
      // make sure it exists
      if (!mRootMonitors[hwnd]) {
        mRootMonitors[hwnd] = std::make_shared<VirtualMonitor>(mScene, hwnd);
      }

      auto monitor = mRootMonitors[hwnd];
      monitor->Blit(screenshot);
    } else {
      ;
    }
  }

  for (HWND hwnd : dead_hwnds) {
    mRootMonitors.erase(hwnd);
    mChildMonitors.erase(hwnd);
  }

  float num_root_monitors = static_cast<float>(mRootMonitors.size());
  const float r = 1.0f;

  int i = 0;
  for (auto iter : mRootMonitors) {
    auto monitor = iter.second;

    monitor->UpdateScale();

    float deg = 360.0f * (i / num_root_monitors);
    float rad = deg * 3.14159f / 180.0f;

    float x = r * cos(rad);
    float y = r * sin(rad);

    char buffer[8192];
    sprintf(buffer, "%f, %f %f\n", x, 0, -y);
    Trace(buffer);
    monitor->SetPosition(x, 0, -y);

    monitor->FocusAtUser();

    ++i;
  }
  Trace("*****\n");
}
