#include "StableHeaders.hpp"
#include "ScreenshotProducer.hpp"

using namespace std;
using namespace boost;
using namespace Ogre;

const int Screenshot::msPixelSize = 4;

Screenshot::Screenshot(HWND hwnd, int width, int height)
    : mPixels(nullptr),
      mHwnd(hwnd),
      mWidth(width),
      mHeight(height) {
  size_t bytes = mWidth * msPixelSize * mHeight;
  mPixels = new unsigned char[bytes];
}

Screenshot::~Screenshot() {
  if (mPixels) {
    delete[] mPixels;
    mPixels = nullptr;
  }
}

///////////////////////////////////////////////////////////////////////////////

ScreenshotProducer::~ScreenshotProducer() {
}

ScreenshotProducer::ScreenshotProducer() {
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
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_IDLE);

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
    for (const HWND& hwnd : mVisibleWindows) {
      mLock.lock();
      if (mScreenshots[hwnd]) {
        mLock.unlock();
        continue;
      }
      mLock.unlock();

      ScreenshotPtr screenshot = CaptureScreenshot(hwnd);

      mLock.lock();
      mScreenshots[hwnd] = screenshot;
      mLock.unlock();
    }
  }
}

BOOL ScreenshotProducer::enumWindowsProc(HWND hwnd) {
  // don't capture screenshot if window is not visible
  if (!::IsWindowVisible(hwnd))
    return TRUE;

  // maximum length according to WNDCLASSEX structure is 256, maybe need NULL
  // byte?
  char szClass[257];
  ::GetClassName(hwnd, szClass, sizeof(szClass));
  string win_class(szClass);

  char szTitle[1024];
  GetWindowText(hwnd , szTitle, sizeof(szTitle));
  string win_title(szTitle);

  if (win_title == "OcularWM") {
    return TRUE;
  }

  mVisibleWindows.push_back(hwnd);

  return TRUE;
}

// http://stackoverflow.com/questions/1149271/how-to-correctly-screencapture-a-specific-window-on-aero-dwm
ScreenshotPtr ScreenshotProducer::CaptureScreenshot(HWND hwnd) {
  const int x = 0;
  const int y = 0;

  RECT r;
  GetWindowRect(hwnd, &r);
  int w = r.right - r.left;
  int h = r.bottom - r.top;

  ScreenshotPtr screenshot = make_shared<Screenshot>(hwnd, w, h);

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

  Ogre::uint8* pixels = screenshot->mPixels;
  GetDIBits(display_dc, bitmap, 0, h, pixels, &info, DIB_RGB_COLORS);

  DeleteObject(bitmap);
  ReleaseDC(0, display_dc);

  return screenshot;
}

ScreenshotPtr ScreenshotProducer::ConsumeRandom()
{
  ScreenshotPtr shot;
  mLock.lock();
  for (auto iter = mScreenshots.begin(); iter != mScreenshots.end(); ++iter) {
    if (!iter->second)
      continue;

    shot = iter->second;
    HWND hwnd = iter->first;
    mScreenshots.erase(hwnd);
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
  mScreenshots.erase(hwnd);
  mLock.unlock();

  return shot;
}

std::vector<HWND> ScreenshotProducer::GetVisibleWindows()
{
  mVisibleWindowsLock.lock();
  std::vector<HWND> copy = mVisibleWindows;
  mVisibleWindowsLock.unlock();
  return copy;
}
