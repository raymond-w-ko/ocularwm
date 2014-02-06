#include "StableHeaders.hpp"
#include "OcularWM.hpp"

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  try {
    OcularWM ocularWM;
    ocularWM.Loop();
  } catch (std::exception& e) {
    Trace(e.what());
  }

  return 0;
}

  /*
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
    */

/*
void OcularWM::createCamera(void)
{
    mLeftEyeCamera = mSceneMgr->createCamera("LeftEye");
    mLeftEyeCamera->setPosition(mUserMetrics.CalculateLeftEyeOffset());
    mLeftEyeCamera->setNearClipDistance(5.0f);
    mLeftEyeCamera->setFOVy(Radian(Degree(kOculusRiftVerticalFov)));
    mLeftEyeCamera->lookAt(Ogre::Vector3(0, 160, -300.0f));

    mRightEyeCamera = mSceneMgr->createCamera("RightEye");
    mRightEyeCamera->setPosition(mUserMetrics.CalculateRightEyeOffset());
    mRightEyeCamera->setNearClipDistance(5.0f);
    mRightEyeCamera->setFOVy(Radian(Degree(kOculusRiftVerticalFov)));
    mRightEyeCamera->lookAt(Ogre::Vector3(0, 160, -300.0f));

    // Position it at 500 in Z direction
    mCameraMan = new OgreBites::SdkCameraMan(mLeftEyeCamera);
    mCamera = mLeftEyeCamera;

    auto& left = mUserMetrics.CalculateLeftEyeOffset();
    auto& right = mUserMetrics.CalculateRightEyeOffset();

    mLog << "left" << "\n" <<
        left.x << "\n" <<
        left.y << "\n" <<
        left.z << "\n";

    mLog << "right" << "\n" <<
        right.x << "\n" <<
        right.y << "\n" <<
        right.z << "\n";
}

void OcularWM::createViewports()
{
    // just to make sure
    mWindow->removeAllViewports();

    mLeftViewport = mWindow->addViewport(mLeftEyeCamera, 9001,
                                         0.0f, 0.0f,
                                         0.5f, 1.0f);
    mLeftEyeCamera->setAspectRatio(
        Ogre::Real(mLeftViewport->getActualWidth()) /
        Ogre::Real(mLeftViewport->getActualHeight()));
    mLeftViewport->setBackgroundColour(Ogre::ColourValue(0, 1.0f, 0));

    mRightViewport = mWindow->addViewport(mRightEyeCamera, 9002,
                                         0.5f, 0.0f,
                                         0.5f, 1.0f);
    mRightEyeCamera->setAspectRatio(
        Ogre::Real(mRightViewport->getActualWidth()) /
        Ogre::Real(mRightViewport->getActualHeight()));
    mRightViewport->setBackgroundColour(Ogre::ColourValue(0, 1.0f, 0));

    Ogre::CompositorManager::ResourceMapIterator resourceIterator =
        Ogre::CompositorManager::getSingleton().getResourceIterator();
    while (resourceIterator.hasMoreElements()) {
        mLog << resourceIterator.getNext()->getName() << "\n";
    }

    mIsBarrelWarpEnabled = true;
    CompositorManager::getSingleton().addCompositor(mLeftViewport, "BarrelDistortion");
    CompositorManager::getSingleton().setCompositorEnabled(
        mLeftViewport, "BarrelDistortion", mIsBarrelWarpEnabled);
    CompositorManager::getSingleton().addCompositor(mRightViewport, "BarrelDistortion");
    CompositorManager::getSingleton().setCompositorEnabled(
        mRightViewport, "BarrelDistortion", mIsBarrelWarpEnabled);
}

bool OcularWM::keyPressed( const OIS::KeyEvent &arg )
{
    if (!BaseApplication::keyPressed(arg))
        return false;

    if (arg.key == OIS::KC_B) {
        mIsBarrelWarpEnabled = !mIsBarrelWarpEnabled;
        CompositorManager::getSingleton().setCompositorEnabled(
            mLeftViewport, "BarrelDistortion", mIsBarrelWarpEnabled);
        CompositorManager::getSingleton().setCompositorEnabled(
            mRightViewport, "BarrelDistortion", mIsBarrelWarpEnabled);
    }

    return true;
}

void OcularWM::createScene()
{
    mSceneMgr->setSkyBox(true, "Examples/CloudyNoonSkyBox");

    // make everything broken unless they are self emissive
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.0f, 0.0f, 0.0f));

    VirtualMonitorPtr mon0 = VirtualMonitor::Create(
        mSceneMgr, "primary", 1920, 1080);
    mon0->SetScale(52);
    mon0->SetPosition(0, 160, -45);
    mMonitors.push_back(mon0);

    mScreenshotProducer.StartBackgroundLoop();

    mWindowTitles.clear();

    string titles;
    for (auto& title : mWindowTitles) {
        titles += title + "\n";
    }
    //::MessageBox(nullptr, titles.c_str(), "", MB_OK);
}

void OcularWM::createFrameListener()
{
    mWindow->getCustomAttribute("WINDOW", &mHwnd);
    ::SetWindowLongPtr(mHwnd, GWLP_WNDPROC, (LONG_PTR)&OcularWM::WindowProc);
    ::SendMessage(mHwnd, WM_USER, (WPARAM)this, 0);
}

static int32 sScreenOffset = 0;

bool OcularWM::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    //if (!BaseApplication::frameRenderingQueued(evt))
        //return false;

    std::vector<HWND> hwnds = mScreenshotProducer.GetVisibleWindows();
    for (HWND hwnd : hwnds) {
        ScreenshotPtr shot = mScreenshotProducer.Get(hwnd);
        if (!shot)
            continue;

        HWND hwnd = shot->hwnd;

        if (!mMonitorFromHwnd[hwnd]) {
            VirtualMonitorPtr monitor = VirtualMonitor::Create(
                mSceneMgr, "", shot->mWidth, shot->mHeight);
            monitor->SetScale(52);
            monitor->SetPosition(sScreenOffset, 160, -45);
            sScreenOffset -= 300;

            mMonitors.push_back(monitor);
            mMonitorFromHwnd[hwnd] = monitor;
        }

        mMonitorFromHwnd[hwnd]->ChangeResolution(shot->mWidth, shot->mHeight);
        mMonitorFromHwnd[hwnd]->WritePixels(shot->mPixels);
    }

    return true;
}


LRESULT CALLBACK OcularWM::WindowProc(HWND hwnd,
                                      UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
    static OcularWM* self = NULL;

    switch (uMsg) {
    case WM_USER: {
        self = reinterpret_cast<OcularWM*>(wParam);
    }
    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
    }
    case WM_CHAR: {
        break;
    }
    default: {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    }

    return 0;
}
*/
