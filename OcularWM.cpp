#include "stdafx.h"
#include "OcularWM.h"

using namespace std;
using namespace boost;
using namespace Ogre;

//-------------------------------------------------------------------------------------
OcularWM::OcularWM()
    : mLog("OcularWM.log", ios_base::trunc)
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
    mLeftEyeCamera->setFOVy(Radian(Degree(110)));
    mLeftEyeCamera->lookAt(Ogre::Vector3(0, 160, -300.0f));

    mRightEyeCamera = mSceneMgr->createCamera("RightEye");
    mRightEyeCamera->setPosition(mUserMetrics.CalculateRightEyeOffset());
    mRightEyeCamera->setNearClipDistance(5.0f);
    mRightEyeCamera->setFOVy(Radian(Degree(110)));
    mRightEyeCamera->lookAt(Ogre::Vector3(0, 160, -300.0f));

    // Position it at 500 in Z direction
    //mCameraMan = new OgreBites::SdkCameraMan(mLeftEyeCamera);
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
    mLeftViewport->setBackgroundColour(Ogre::ColourValue(0, 0, 0));

    mRightViewport = mWindow->addViewport(mRightEyeCamera, 9002,
                                         0.5f, 0.0f,
                                         0.5f, 1.0f);
    mRightEyeCamera->setAspectRatio(
        Ogre::Real(mRightViewport->getActualWidth()) /
        Ogre::Real(mRightViewport->getActualHeight()));
    mRightViewport->setBackgroundColour(Ogre::ColourValue(0, 0, 0));

    Ogre::CompositorManager::ResourceMapIterator resourceIterator =
        Ogre::CompositorManager::getSingleton().getResourceIterator();
    while (resourceIterator.hasMoreElements()) {
        mLog << resourceIterator.getNext()->getName() << "\n";
    }

    CompositorManager::getSingleton().addCompositor(mLeftViewport, "BarrelDistortion");
    CompositorManager::getSingleton().setCompositorEnabled(
        mLeftViewport, "BarrelDistortion", true);
    //CompositorManager::getSingleton().addCompositor(mRightViewport, "BarrelDistortion");
    //CompositorManager::getSingleton().setCompositorEnabled(
        //mRightViewport, "BarrelDistortion", true);
}

void OcularWM::createScene()
{
    mSceneMgr->setSkyDome(true, "Examples/CloudySky", 32, 8);

    // create your scene here :)
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.5f, 0.5f, 0.5f));

    Entity* ogreHead = mSceneMgr->createEntity("Head", "ogrehead.mesh");
    SceneNode* headNode = mSceneMgr->getRootSceneNode()->createChildSceneNode(
        "HeadNode");
    headNode->attachObject(ogreHead);
    headNode->setPosition(Ogre::Vector3(0, 160, -120.0f));
    //headNode->setScale(0.1f, 0.1f, 0.1f);
    //headNode->setScale(50, 50, 50);

    Light* light = mSceneMgr->createLight("MainLight");
    light->setPosition(0, 100, -50.0f);
}
