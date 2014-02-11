#pragma once

struct Screenshot : public boost::noncopyable {
  Screenshot(HWND hwnd, int width, int height);
  ~Screenshot();

  const static int msPixelSize;
  const HWND mHwnd;
  const int mWidth;
  const int mHeight;
  Ogre::uint8* mPixels;
};
typedef std::shared_ptr<Screenshot> ScreenshotPtr;

struct WindowInfo {
  HWND hwnd;
  std::string mClassName;
  std::string mTitle;
};

namespace std {
  template <>
  struct hash<WindowInfo> : public unary_function<WindowInfo, size_t> {
    size_t operator()(const WindowInfo& v) const {
      return std::hash<HWND>()(v.hwnd);
    }
  };
}

class ScreenshotProducer {
public:
  ScreenshotProducer();
  ~ScreenshotProducer();

  // call from primary thread only
  void Stop();
  ScreenshotPtr ConsumeRandom();
  ScreenshotPtr Get(HWND hwnd);

  // call from background thread only
  void Start();
  ScreenshotPtr CaptureScreenshot(HWND hwnd);

  std::vector<HWND> GetVisibleWindows();

  void SetParentHwnd(HWND hwnd) { mParentHwnd = hwnd; }

protected:
  void loop();
  static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    return ((ScreenshotProducer*)lParam)->enumWindowsProc(hwnd);
  }
  BOOL enumWindowsProc(HWND hwnd);

  std::thread mBackgroundThread;
  std::atomic<int> mExitFlag;
  std::mutex mLock;
  std::mutex mVisibleWindowsLock;

  std::vector<HWND> mVisibleWindows;
  std::unordered_map<HWND, ScreenshotPtr> mScreenshots;

  HWND mParentHwnd;
};
