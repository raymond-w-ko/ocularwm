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

    Entity* ogreHead = mSceneMgr->createEntity("Head", "ogrehead.mesh");
    SceneNode* headNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
        "HeadNode");
    headNode->attachObject(ogreHead);
    headNode->setPosition(Ogre::Vector3(0, 160, -75.0f));
    //headNode->setScale(0.1f, 0.1f, 0.1f);
    //headNode->setScale(50, 50, 50);

    Light* light = mSceneMgr->createLight("MainLight");
    light->setPosition(0, 160, 0.0f);

    VirtualMonitor screen0 = createVirtualMonitor(0);
    screen0.mSceneNode->setScale(50, 50, 50);
    screen0.mSceneNode->setPosition(0, 160, -70);
    mMonitors.push_back(screen0);
}

VirtualMonitor OcularWM::createVirtualMonitor(unsigned int id)
{
    VirtualMonitor mon;
    mon.mID = id;

    string suffix = lexical_cast<string>(id);

    string texture_name = "DynamicTexture" + suffix;
    TexturePtr texture = TextureManager::getSingleton().createManual(
        texture_name,
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        TEX_TYPE_2D,
        2000, 2000,
        0,
        PF_BYTE_BGRA,
        TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
    mon.mDynamicTexture = texture;

    HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
    mon.mPixelBuffer = pixelBuffer;

    // Lock the pixel buffer and get a pixel box
    // for best performance use HBL_DISCARD!
    pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
    const PixelBox& pixelBox = pixelBuffer->getCurrentLock();

    uint8* pDest = static_cast<uint8*>(pixelBox.data);
    uint8* pPixels = pDest;

    // Fill in some pixel data. This will give a semi-transparent blue,
    // but this is of course dependent on the chosen pixel format.
    /*
    for (size_t j = 0; j < 512; j++) {
        for(size_t i = 0; i < 1024; i++) {
            *pDest++ = 255; // B
            *pDest++ = 0; // G
            *pDest++ = 0; // R
            *pDest++ = 127; // A
        }
    }
    for (size_t j = 0; j < 512; j++) {
        for(size_t i = 0; i < 1024; i++) {
            *pDest++ = 0; // B
            *pDest++ = 0; // G
            *pDest++ = 255; // R
            *pDest++ = 127; // A
        }
    }

    // UL
    for (size_t y = 0; y < 100; ++y) {
        for (size_t x = 0; x < 100; ++x) {
            uint8* pixel = &pPixels[(y * 1024 * 4) + (x * 4)];
            pixel[0] = 0;
            pixel[1] = 255;
            pixel[2] = 0;
            pixel[3] = 255;
        }
    }

    // UR
    for (size_t y = 0; y < 100; ++y) {
        for (size_t x = 1024 - 100; x < 1024; ++x) {
            uint8* pixel = &pPixels[(y * 1024 * 4) + (x * 4)];
            pixel[0] = 0;
            pixel[1] = 255;
            pixel[2] = 255;
            pixel[3] = 255;
        }
    }

    // LL
    for (size_t y = 1024 - 100; y < 1024; ++y) {
        for (size_t x = 0; x < 100; ++x) {
            uint8* pixel = &pPixels[(y * 1024 * 4) + (x * 4)];
            pixel[0] = 0;
            pixel[1] = 0;
            pixel[2] = 0;
            pixel[3] = 255;
        }
    }

    // LR
    for (size_t y = 1024 - 100; y < 1024; ++y) {
        for (size_t x = 1024 - 100; x < 1024; ++x) {
            uint8* pixel = &pPixels[(y * 1024 * 4) + (x * 4)];
            pixel[0] = 255;
            pixel[1] = 255;
            pixel[2] = 255;
            pixel[3] = 255;
        }
    }
    */


    pixelBuffer->unlock();

    string material_name = "DynamicTextureMaterial" + suffix;
    MaterialPtr material = MaterialManager::getSingleton().create(
        material_name,
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    material->getTechnique(0)->getPass(0)->createTextureUnitState(texture_name);
    material->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
    mon.mMaterial = material;

    ManualObject* object = mSceneMgr->createManualObject("ManualObject" + suffix);
    object->begin(material_name, RenderOperation::OT_TRIANGLE_STRIP);
    object->position(-1, -1, +0);
    object->textureCoord(0, 1);

    object->position(+1, -1, +0);
    object->textureCoord(1, 1);

    object->position(+1, +1, +0);
    object->textureCoord(1, 0);

    object->position(-1, +1, +0);
    object->textureCoord(0, 0);

    object->index(0);
    object->index(1);
    object->index(2);
    object->index(3);
    object->index(0);
    object->end();
    mon.mManualObject = object;

    SceneNode* node = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    node->attachObject(object);
    mon.mSceneNode = node;

    return mon;
}

bool OcularWM::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    if (!BaseApplication::frameRenderingQueued(evt))
        return false;

    HWND chrome = 0;
    do {
        chrome = FindWindowEx(NULL, chrome, "Chrome_WidgetWin_1", NULL);
    } while (GetWindowTextLength(chrome) == 0);
    if (!chrome)
        return true;

    HDC activeDC = ::GetWindowDC(chrome);
    if (activeDC) {
        HDC copyDC = ::CreateCompatibleDC(activeDC);
        if (copyDC) {
            RECT r;
            ::GetWindowRect(chrome, &r);
            int width = r.right - r.left;
            int height = r.bottom - r.top;
            HBITMAP hBitmap = ::CreateCompatibleBitmap(activeDC, width, height);
            if (hBitmap) {
                if (::SelectObject(copyDC, hBitmap) &&
                    ::PrintWindow(chrome, copyDC, 0))
                {
                    auto& pixelBuffer = mMonitors[0].mPixelBuffer;
                    pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
                    const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
                    uint8* pixels = static_cast<uint8*>(pixelBox.data);

                    BITMAPINFO info = {0};
                    /*
                    if (0 == ::GetDIBits(copyDC, hBitmap,
                                         0, 0, NULL, &info, DIB_RGB_COLORS))
                    {
                        goto cleanup;
                    }
                    */

                    info.bmiHeader.biSize = sizeof(info.bmiHeader);
                    info.bmiHeader.biPlanes = 1;
                    info.bmiHeader.biBitCount = 32;
                    info.bmiHeader.biCompression = BI_RGB;
                    /*
                    info.bmiHeader.biHeight =
                        (info.bmiHeader.biHeight < 0) ?
                        (-info.bmiHeader.biHeight) :
                        (info.bmiHeader.biHeight);
                    */
                    info.bmiHeader.biHeight = 2000;
                    info.bmiHeader.biWidth = 2000;

                    if(0 == ::GetDIBits(copyDC, hBitmap,
                                        0, info.bmiHeader.biHeight,
                                        (LPVOID)pixels, &info, DIB_RGB_COLORS))
                    {
                        goto cleanup;
                    }

                    cleanup:

                    pixelBuffer->unlock();
                }

                ::DeleteObject(hBitmap);
            }
            ::DeleteObject(copyDC);
        }
        ::ReleaseDC(chrome, activeDC);
    }

    return true;
}

