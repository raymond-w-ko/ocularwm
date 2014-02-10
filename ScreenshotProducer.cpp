#include "StableHeaders.hpp"
#include "ScreenshotProducer.h"

using namespace std;
using namespace boost;
using namespace Ogre;

const int Screenshot::msPixelSize = 4;

Screenshot::Screenshot()
    : mPixels(nullptr),
      mWidth(-1),
      mHeight(-1),
      mHwnd(0) {
}

Screenshot::~Screenshot() {
  if (mPixels) {
    delete[] mPixels;
    mPixels = nullptr;
  }
}

void Screenshot::AllocateSpaceForPixels() {
  size_t count = mWidth * msPixelSize * mHeight;
  mPixels = new unsigned char[count];
}

///////////////////////////////////////////////////////////////////////////////

ScreenshotProducer::~ScreenshotProducer() {
}

ScreenshotProducer::ScreenshotProducer() {
  mExitFlag.store(0);
}

void ScreenshotProducer::Stop() {
  mExitFlag.store(1);
  if (mBackgroundThread.joinable()) {
    mBackgroundThread.join();
  }
}

void ScreenshotProducer::Start() {
  // don't double start
  if (mBackgroundThread.get_id() != std::thread::id()) {
    return;
  }

  mExitFlag.store(0);
  mBackgroundThread = std::thread(&ScreenshotProducer::loop, this);
}

void ScreenshotProducer::loop() {
  ::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

  for (;;) {
    if (mExitFlag.load() != 0)
      break;

    // update list of HWNDs to take a screenshot
    mVisibleWindowsLock.lock();
    mVisibleWindows.clear();
    mVisibleWindowsLock.unlock();
    ::EnumWindows(ScreenshotProducer::EnumWindowsProc,
                    reinterpret_cast<LPARAM>(this));

    // iterate over the list of HWNDs and take a screenshot for each of then
    // iff they already don't have a valid screenshot
    mVisibleWindowsLock.lock();
    for (HWND hwnd : mVisibleWindows) {
      mLock.lock();
      if (mScreenshots[hwnd]) {
        mLock.unlock();
        continue;
      }
      mLock.unlock();

      ScreenshotPtr shot = make_shared<Screenshot>();
      CaptureScreenshot(hwnd, shot);

      mLock.lock();
      mScreenshots[hwnd] = shot;
      mLock.unlock();
    }
    mVisibleWindowsLock.unlock();
  }
}

BOOL CALLBACK ScreenshotProducer::EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
  if (!::IsWindowVisible(hwnd))
    return TRUE;

  ScreenshotProducer* self = reinterpret_cast<ScreenshotProducer*>(lParam);

  // maximum length according to WNDCLASSEX structure is 256, maybe need NULL byte?
  /*
     char szClass[257];
     ::GetClassName(hwnd, szClass, sizeof(szClass));
     string winClass(szClass);

     char szTitle[1024];
     ::GetWindowText(hwnd , szTitle, sizeof(szTitle));
     string title(szTitle);

     if (szTitle == "OcularWM" || szTitle == "OcularWM_d")
     return TRUE;
     */

  self->mVisibleWindowsLock.lock();
  self->mVisibleWindows.push_back(hwnd);
  self->mVisibleWindowsLock.unlock();

  return TRUE;
}

// http://stackoverflow.com/questions/1149271/how-to-correctly-screencapture-a-specific-window-on-aero-dwm
void ScreenshotProducer::CaptureScreenshot(HWND winId, ScreenshotPtr shot)
{
  const int x = 0;
  const int y = 0;

  RECT r;
  GetWindowRect(winId, &r);
  int w = r.right - r.left;
  int h = r.bottom - r.top;

  shot->mHwnd = winId;
  shot->mWidth = w;
  shot->mHeight = h;
  shot->AllocateSpaceForPixels();

  // Create and setup bitmap
  HDC display_dc = GetDC(0);
  HDC bitmap_dc = CreateCompatibleDC(display_dc);
  HBITMAP bitmap = CreateCompatibleBitmap(display_dc, w, h);
  HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

  // copy data
  HDC window_dc = GetWindowDC(winId);
  BitBlt(bitmap_dc, 0, 0, w, h, window_dc, x, y, SRCCOPY);

  // clean up all but bitmap
  ReleaseDC(winId, window_dc);
  SelectObject(bitmap_dc, null_bitmap);
  DeleteDC(bitmap_dc);

  BITMAPINFO info = {0};
  info.bmiHeader.biSize = sizeof(info.bmiHeader);
  info.bmiHeader.biWidth = w;
  info.bmiHeader.biHeight = h;
  info.bmiHeader.biPlanes = 1;
  info.bmiHeader.biBitCount = 32;
  info.bmiHeader.biCompression = BI_RGB;

  GetDIBits(display_dc, bitmap, 0, h, shot->mPixels, &info, DIB_RGB_COLORS);

  DeleteObject(bitmap);
  ReleaseDC(0, display_dc);
}

ScreenshotPtr ScreenshotProducer::ConsumeRandom()
{
  ScreenshotPtr shot;
  mLock.lock();
  for (auto iter = mScreenshots.begin(); iter != mScreenshots.end(); ++iter) {
    if (!iter->second)
      continue;

    shot = iter->second;
    mScreenshots[iter->first].reset();
    break;
  }
  mLock.unlock();
  return shot;
}

ScreenshotPtr ScreenshotProducer::Get(HWND hwnd)
{
  ScreenshotPtr shot;

  mLock.lock();
  shot = mScreenshots[hwnd];
  mScreenshots[hwnd].reset();
  mLock.unlock();

  return shot;
}

std::vector<HWND> ScreenshotProducer::GetVisibleWindows()
{
  if (!mVisibleWindowsLock.try_lock())
    return std::vector<HWND>();

  std::vector<HWND> copy = mVisibleWindows;
  mVisibleWindowsLock.unlock();
  return copy;
}
