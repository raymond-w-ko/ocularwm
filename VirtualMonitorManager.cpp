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

void VirtualMonitorManager::Update() {
  auto hwnds = mScreenshotProducer->GetVisibleWindows();
  for (HWND hwnd : hwnds) {
    if (IsAltTabWindow(hwnd)) {
      char szTitle[1024];
      GetWindowText(hwnd , szTitle, sizeof(szTitle));
      std::string win_title(szTitle);
    }
  }
}
