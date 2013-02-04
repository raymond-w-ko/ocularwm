#pragma once

#include "BaseApplication.h"
#include "UserMetrics.h"
#include "VirtualMonitor.h"

class OcularWM : public BaseApplication {
public:
    OcularWM(void);
    virtual ~OcularWM(void);

    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);

protected:
    virtual void createCamera(void);
    virtual void createViewports();

    virtual void createScene();

    virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);
    virtual bool keyPressed( const OIS::KeyEvent &arg );

    UserMetrics mUserMetrics;

    Ogre::Camera* mLeftEyeCamera;
    Ogre::Camera* mRightEyeCamera;

    Ogre::Viewport* mLeftViewport;
    Ogre::Viewport* mRightViewport;

    std::ofstream mLog;
    bool mIsBarrelWarpEnabled;
    std::vector<VirtualMonitorPtr> mMonitors;

    std::vector<std::string> mWindowTitles;
};
