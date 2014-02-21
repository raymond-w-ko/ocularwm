#include "StableHeaders.hpp"
#include "ScreenshotProducer.hpp"

using namespace std;
using namespace boost;
using namespace Ogre;

const int Screenshot::msPixelSize = 4;

Screenshot::Screenshot(WindowID hwnd)
    : mPixels(nullptr),
      mConsumed(true),
      mHwnd(hwnd),
      mWidth(-1), mHeight(-1) {
  ;
}

Screenshot::~Screenshot() {
  if (mPixels) {
    delete[] mPixels;
    mPixels = nullptr;
  }
}

void Screenshot::EnsurePixelsAllocated(int width, int height) {
  std::lock_guard<std::mutex> lock(mLock);

  if (mPixels && mWidth == width && mHeight == height) {
    return;
  }

  if (mPixels) {
    delete[] mPixels;
    mPixels = nullptr;
  }

  mWidth = width;
  mHeight = height;
  mPixels = new Ogre::uint8[mWidth * mHeight * msPixelSize];
}

void Screenshot::Lock(Ogre::uint8** pixels, int* width, int* height) {
  mLock.lock();
  *pixels = mPixels;
  *width = mWidth;
  *height = mHeight;
}

bool Screenshot::TryLock(Ogre::uint8** pixels, int* width, int* height) {
  if (!mLock.try_lock()) {
    return false;
  }

  *pixels = mPixels;
  *width = mWidth;
  *height = mHeight;

  return true;
}

void Screenshot::Unlock() {
  mLock.unlock();
}

void Screenshot::ArrangePixelsToBestPixelFormat() {
  size_t num_pixels = mWidth * mHeight;
  Ogre::uint8* pixel = mPixels;
  // a should be copied to be absolutely correct, but since we don't use the a
  // channel anyways, skip for speed.
  char r, g, b/*, a*/;
  for (size_t i = 0; i < num_pixels; ++i) {
    b = pixel[0];
    g = pixel[1];
    r = pixel[2];
    //a = pixel[3];

    //pixel[0] = a;
    pixel[1] = r;
    pixel[2] = g;
    pixel[3] = b;

    pixel += 4;
  }
}

///////////////////////////////////////////////////////////////////////////////

ScreenshotProducer::~ScreenshotProducer() {
}

ScreenshotProducer::ScreenshotProducer()
    : mParentHwnd(0) {
  mExitFlag.store(0);
}

void ScreenshotProducer::Start() {
  // don't double start
  if (mBackgroundThread.get_id() != std::thread::id()) {
    return;
  }

  mExitFlag.store(0);
  mBackgroundThread = std::thread(&ScreenshotProducer::loop, this);
}

void ScreenshotProducer::Stop() {
  mExitFlag.store(1);
  if (mBackgroundThread.joinable()) {
    mBackgroundThread.join();
  }
}

void ScreenshotProducer::loop() {
  HANDLE thread_id = GetCurrentThread();
  SetThreadPriority(thread_id, THREAD_PRIORITY_IDLE);
  SetThreadPriorityBoost(thread_id, TRUE);

  for (;;) {
    // check if Stop() is called
    if (mExitFlag.load() != 0)
      break;

    // update list of windows for taking a screenshot
    mVisibleWindowsLock.lock();
    mVisibleWindows.clear();
    ::EnumWindows(ScreenshotProducer::EnumWindowsProc, (LPARAM)this);
    mVisibleWindowsLock.unlock();

    // iterate over the list of windows and take a screenshot for each of then
    // if they already don't have a valid screenshot
    for (const WindowID& hwnd : mVisibleWindows) {
      ScreenshotPtr screenshot;

      mMapLock.lock();
      if (mScreenshots.find(hwnd) == mScreenshots.end()) {
        mScreenshots[hwnd] = std::make_shared<Screenshot>(hwnd);
      }
      screenshot = mScreenshots[hwnd];
      mMapLock.unlock();

      Ogre::uint8* pixels;
      int dummy1, dummy2;
      screenshot->Lock(&pixels, &dummy1, &dummy2);
      bool need_update = screenshot->IsConsumed();
      screenshot->Unlock();

      if (need_update) {
        captureScreenshot(hwnd, screenshot);
      }
    }

    ::SwitchToThread();
  }
}

BOOL ScreenshotProducer::enumWindowsProc(WindowID hwnd) {
  // Exclude our own window to avoid a deadlock situation the main thread locks
  // mVisibleWindowsLock, and goes to sleep, thus never running its own
  // WindowProc when GetClassName() and GetWindowText() is called below
  if (mParentHwnd == hwnd)
    return TRUE;

  // Do not capture screenshot if window is not visible
  if (!::IsWindowVisible(hwnd))
    return TRUE;

  // Does not work in Window 7+, but do so just to be safe
  if (hwnd == ::GetDesktopWindow())
      return TRUE;

  // Maximum length according to WNDCLASSEX structure is 256, maybe need NULL
  // byte anyway?
  char szClass[257];
  ::GetClassName(hwnd, szClass, sizeof(szClass));

  char szTitle[1024];
  GetWindowText(hwnd , szTitle, sizeof(szTitle));

  // Do not capture the total area of the desktop.
  //
  // On a 3 monitor setup like mines this will result in XBOX HUEG textures of
  // 5120x1080. This takes much more time than usual to process and copy to the
  // GPU than regular desktop sized windows.
  if (!strcmp(szClass, "Progman") && !strcmp(szTitle, "Program Manager"))
    return TRUE;

  // Also do not treat the taskbar as a real window.
  // Windows 8 and above also has "Shell_SecondaryTrayWnd" for secondary
  // monitors. Ignore those too.
  if (!strcmp(szClass, "Shell_TrayWnd") || !strcmp(szClass, "Shell_SecondaryTrayWnd"))
    return TRUE;

  // TODO: maybe exclude large windows as they can take enough time to cause
  // stutters?
  //RECT r;
  //GetWindowRect(hwnd, &r);
  //int w = r.right - r.left;
  //int h = r.bottom - r.top;

  mVisibleWindows.push_back(hwnd);

  return TRUE;
}

// http://stackoverflow.com/questions/1149271/how-to-correctly-screencapture-a-specific-window-on-aero-dwm
void ScreenshotProducer::captureScreenshot(WindowID hwnd, ScreenshotPtr screenshot) {
  const int x = 0;
  const int y = 0;

  RECT r;
  GetWindowRect(hwnd, &r);
  int w = r.right - r.left;
  int h = r.bottom - r.top;

  // Create and setup bitmap
  HDC display_dc = GetDC(0);
  HDC bitmap_dc = CreateCompatibleDC(display_dc);
  HBITMAP bitmap = CreateCompatibleBitmap(display_dc, w, h);
  HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

  // copy data
  HDC window_dc = GetWindowDC(hwnd);
  BitBlt(bitmap_dc, 0, 0, w, h, window_dc, x, y, SRCCOPY);

  // clean up all but bitmap
  ReleaseDC(hwnd, window_dc);
  SelectObject(bitmap_dc, null_bitmap);
  DeleteDC(bitmap_dc);

  BITMAPINFO info = {0};
  info.bmiHeader.biSize = sizeof(info.bmiHeader);
  info.bmiHeader.biWidth = w;
  info.bmiHeader.biHeight = h;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;

  screenshot->EnsurePixelsAllocated(w, h);
  int dummy1;
  int dummy2;
  Ogre::uint8* pixels;
  screenshot->Lock(&pixels, &dummy1, &dummy2);
  GetDIBits(display_dc, bitmap, 0, h, pixels, &info, DIB_RGB_COLORS);
  screenshot->ArrangePixelsToBestPixelFormat();
  screenshot->SetConsumed(false);
  screenshot->Unlock();

  DeleteObject(bitmap);
  ReleaseDC(0, display_dc);
}

ScreenshotPtr ScreenshotProducer::Get(WindowID hwnd) {
  ScreenshotPtr shot;
  mMapLock.lock();
  auto iter = mScreenshots.find(hwnd);
  if (iter != mScreenshots.end()) {
    shot = iter->second;
  }
  mMapLock.unlock();
  return shot;
}

std::vector<WindowID> ScreenshotProducer::GetVisibleWindows() {
  std::vector<WindowID> copy;
  if (!mVisibleWindowsLock.try_lock()) {
    return copy;
  }
  copy = mVisibleWindows;
  mVisibleWindowsLock.unlock();
  return copy;
}
