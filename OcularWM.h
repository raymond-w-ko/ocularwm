#pragma once

#include "BaseApplication.h"
#include "UserMetrics.h"

class OcularWM : public BaseApplication {
public:
    OcularWM(void);
    virtual ~OcularWM(void);

protected:
    virtual void createCamera(void);
    virtual void createViewports();

    virtual void createScene();

    UserMetrics mUserMetrics;

    Ogre::Camera* mLeftEyeCamera;
    Ogre::Camera* mRightEyeCamera;

    Ogre::Viewport* mLeftViewport;
    Ogre::Viewport* mRightViewport;

    std::ofstream mLog;
};
