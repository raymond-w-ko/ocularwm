#pragma once

struct Screenshot {
    Screenshot();
    ~Screenshot();

    /// assumes mWidth and mHeight are properly set
    void AllocateSpaceForPixels();

    const static int msPixelSize;
    HWND hwnd;
    int mWidth;
    int mHeight;
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
       size_t operator()(const WindowInfo& v) const
       {
           return std::hash<HWND>()(v.hwnd);
       }
   };
}

class ScreenshotProducer {
public:
    ScreenshotProducer();
    ~ScreenshotProducer();

    // call from primary thread only
    void RequestAndWaitForExit();
    ScreenshotPtr ConsumeRandom();
    ScreenshotPtr Get(HWND hwnd);

    // call from background thread only
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
    void StartBackgroundLoop();
    void CaptureScreenshot(HWND hwnd, ScreenshotPtr shot);

    std::vector<HWND> GetVisibleWindows();

protected:
    void loop();

    std::thread mBackgroundThread;
    std::atomic<int> mExitFlag;
    std::mutex mLock;
    std::mutex mVisibleWindowsLock;

    std::vector<HWND> mVisibleWindows;
    std::unordered_map<HWND, std::shared_ptr<Screenshot>> mScreenshots;
};
