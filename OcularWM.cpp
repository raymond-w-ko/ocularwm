#include "stdafx.h"
#include "OcularWM.h"

using namespace std;
using namespace boost;
using namespace Ogre;

static const float kOculusRiftVerticalFov = 110.0f;

//-------------------------------------------------------------------------------------
OcularWM::OcularWM() :
    mLog("OcularWM.log", ios_base::trunc),
    mIsBarrelWarpEnabled(false)
{
    ::SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    // my personal metrics
    mUserMetrics.SetHeightFromInches(68);
    // TODO measure these
    mUserMetrics.IpdInMm = 65;
    mUserMetrics.eyesCoronalOffsetInCm = 4;
    mUserMetrics.eyesTransverseOffsetInCm = 10;
}

OcularWM::~OcularWM()
{
    mScreenshotProducer.RequestAndWaitForExit();
}

//-------------------------------------------------------------------------------------

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

    /*
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
    */
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

    /*
    VirtualMonitorPtr mon0 = VirtualMonitor::Create(
        mSceneMgr, "primary", 1920, 1080);
    mon0->SetScale(52);
    mon0->SetPosition(0, 160, -45);
    mMonitors.push_back(mon0);
    */

    mScreenshotProducer.StartBackgroundLoop();

    /*
    mWindowTitles.clear();

    string titles;
    for (auto& title : mWindowTitles) {
        titles += title + "\n";
    }
    */
    //::MessageBox(nullptr, titles.c_str(), "", MB_OK);
}

static int32 sScreenOffset = 0;

bool OcularWM::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    if (!BaseApplication::frameRenderingQueued(evt))
        return false;

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

