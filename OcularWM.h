#pragma once

#include "BaseApplication.h"
#include "UserMetrics.h"
#include "VirtualMonitor.h"
#include "ScreenshotProducer.h"

class OcularWM : public BaseApplication {
public:
    OcularWM(void);
    virtual ~OcularWM(void);

protected:
    virtual void createCamera(void);
    virtual void createViewports();

    virtual void createScene();
    virtual void createFrameListener();

    virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);
    virtual bool keyPressed( const OIS::KeyEvent &arg );

    static LRESULT CALLBACK WindowProc(HWND hwnd,
                                       UINT uMsg,
                                       WPARAM wParam,
                                       LPARAM lParam);

    HWND mHwnd;

    UserMetrics mUserMetrics;

    Ogre::Camera* mLeftEyeCamera;
    Ogre::Camera* mRightEyeCamera;

    Ogre::Viewport* mLeftViewport;
    Ogre::Viewport* mRightViewport;

    std::ofstream mLog;
    bool mIsBarrelWarpEnabled;

    ScreenshotProducer mScreenshotProducer;

    std::vector<VirtualMonitorPtr> mMonitors;
    std::unordered_map<HWND, VirtualMonitorPtr> mMonitorFromHwnd;

    //std::vector<std::string> mWindowTitles;
};
