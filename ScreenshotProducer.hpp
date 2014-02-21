#pragma once

// So what I've learned from experimentation, is that we have to allocate a
// constant Screenshot for each WindowID, otherwise the destructor for can take
// quite a while to run.

class Screenshot : public boost::noncopyable {
 public:
  Screenshot(WindowID hwnd);
  ~Screenshot();

  void EnsurePixelsAllocated(int width, int height);
  void Lock(Ogre::uint8** pixels, int* width, int* height);
  bool TryLock(Ogre::uint8** pixels, int* width, int* height);
  void Unlock();

  bool IsConsumed() { return mConsumed; }
  void SetConsumed(bool consumed) { mConsumed = consumed; }

  void ArrangePixelsToBestPixelFormat();

 protected:
  const static int msPixelSize;

 protected:
  const WindowID mHwnd;

  bool mConsumed;
  int mWidth;
  int mHeight;
  Ogre::uint8* mPixels;

  std::mutex mLock;
};
typedef std::shared_ptr<Screenshot> ScreenshotPtr;

class ScreenshotProducer {
public:
  ScreenshotProducer();
  ~ScreenshotProducer();

  // call from primary thread only
  void Start();
  void Stop();

  ScreenshotPtr Get(WindowID hwnd);
  std::vector<WindowID> GetVisibleWindows();

  void SetParentHwnd(WindowID hwnd) { mParentHwnd = hwnd; }

protected:
  // background thread only
  void loop();
  void captureScreenshot(WindowID hwnd, ScreenshotPtr screenshot);

  static BOOL CALLBACK EnumWindowsProc(WindowID hwnd, LPARAM lParam) {
    return ((ScreenshotProducer*)lParam)->enumWindowsProc(hwnd);
  }
  BOOL enumWindowsProc(WindowID hwnd);

  std::thread mBackgroundThread;
  std::atomic<int> mExitFlag;
  std::mutex mMapLock;
  std::mutex mVisibleWindowsLock;

  std::vector<WindowID> mVisibleWindows;
  std::unordered_map<WindowID, ScreenshotPtr> mScreenshots;

  WindowID mParentHwnd;
};
