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
    // my personal metrics
    mUserMetrics.SetHeightFromInches(68);
    // TODO measure these
    mUserMetrics.IpdInMm = 65;
    mUserMetrics.eyesCoronalOffsetInCm = 4;
    mUserMetrics.eyesTransverseOffsetInCm = 10;
}

OcularWM::~OcularWM()
{
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
    //mSceneMgr->setSkyDome(true, "Examples/CloudySky", 32, 8);
    mSceneMgr->setSkyBox(true, "Examples/CloudyNoonSkyBox");

    // create your scene here :)
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.5f, 0.5f, 0.5f));

    //Entity* ogreHead = mSceneMgr->createEntity("Head", "ogrehead.mesh");
    //SceneNode* headNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
        //"HeadNode");
    //headNode->attachObject(ogreHead);
    //headNode->setPosition(Ogre::Vector3(0, 160, -75.0f));
    //headNode->setScale(0.1f, 0.1f, 0.1f);
    //headNode->setScale(50, 50, 50);

    Light* light = mSceneMgr->createLight("MainLight");
    light->setPosition(0, 160, 0.0f);

    VirtualMonitorPtr mon0 = VirtualMonitor::Create(
        mSceneMgr, "primary", 1920, 1080);
    mon0->SetScale(52);
    mon0->SetPosition(0, 160, -45);
    mMonitors.push_back(mon0);
}

bool OcularWM::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    if (!BaseApplication::frameRenderingQueued(evt))
        return false;

    HWND winId = 0;
    do {
        winId = FindWindowEx(NULL, winId, "Chrome_WidgetWin_1", NULL);
    } while (GetWindowTextLength(winId) == 0);
    if (!winId)
        return true;

    int w = -1;
    int h = -1;
    int x = 0;
    int y = 0;

    RECT r;
    GetClientRect(winId, &r);
    if (w < 0) w = r.right - r.left;
    if (h < 0) h = r.bottom - r.top;
    // Create and setup bitmap
    HDC display_dc = GetDC(0);
    HDC bitmap_dc = CreateCompatibleDC(display_dc);
    HBITMAP bitmap = CreateCompatibleBitmap(display_dc, w, h);
    HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

    // copy data
    HDC window_dc = GetDC(winId);
    BitBlt(bitmap_dc, 0, 0, w, h, window_dc, x, y, SRCCOPY);

    // clean up all but bitmap
    ReleaseDC(winId, window_dc);
    SelectObject(bitmap_dc, null_bitmap);
    DeleteDC(bitmap_dc);

    mMonitors[0]->ChangeResolution(w, h);

    uint8* pixels = static_cast<uint8*>(mMonitors[0]->Lock().data);

    BITMAPINFO info = {0};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = w;
    info.bmiHeader.biHeight = h;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    GetDIBits(display_dc, bitmap, 0, h, pixels, &info, DIB_RGB_COLORS);

    mMonitors[0]->Unlock();

    DeleteObject(bitmap);
    ReleaseDC(0, display_dc);

    return true;
}

