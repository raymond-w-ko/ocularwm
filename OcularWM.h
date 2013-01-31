#pragma once

#include "BaseApplication.h"
#include "UserMetrics.h"

struct VirtualMonitor {
    unsigned int mID;
    Ogre::TexturePtr mDynamicTexture;
    Ogre::HardwarePixelBufferSharedPtr mPixelBuffer;
    Ogre::MaterialPtr mMaterial;
    Ogre::ManualObject* mManualObject;
    Ogre::SceneNode* mSceneNode;
};

class OcularWM : public BaseApplication {
public:
    OcularWM(void);
    virtual ~OcularWM(void);

protected:
    virtual void createCamera(void);
    virtual void createViewports();

    virtual void createScene();
    virtual VirtualMonitor createVirtualMonitor(unsigned int id);

    virtual bool frameRenderingQueued(const Ogre::FrameEvent& evt);
    virtual bool keyPressed( const OIS::KeyEvent &arg );

    UserMetrics mUserMetrics;

    Ogre::Camera* mLeftEyeCamera;
    Ogre::Camera* mRightEyeCamera;

    Ogre::Viewport* mLeftViewport;
    Ogre::Viewport* mRightViewport;

    std::ofstream mLog;
    bool mIsBarrelWarpEnabled;
    std::vector<VirtualMonitor> mMonitors;
};
