#pragma once

struct OcularWMException : public std::exception {
  OcularWMException(std::string desc) : mDescription(desc) { }

  virtual const char* what() const {
    return mDescription.c_str();
  }

  std::string mDescription;
};

struct OcularWMSetupException : public OcularWMException {
  OcularWMSetupException(std::string desc)
      : OcularWMException(desc) { }
};

class OculusVars {
 public:
  OVR::Ptr<OVR::DeviceManager> DeviceManager;
  OVR::Ptr<OVR::HMDDevice> HMD;
  OVR::HMDInfo HMDInfo;
  OVR::Ptr<OVR::SensorDevice> Sensor;
  OVR::SensorFusion* SensorFusion;
  OVR::Util::Render::StereoConfig StereoConfig;

  OculusVars() : SensorFusion(NULL) {
  }

  ~OculusVars() {
  }
};

class OcularWM {
 public:
  enum HmdEye {
    kLeft = 0,
    kRight,
    // left and right is based of the X view space offset of this camera
    kCenter,
  };
  OcularWM();
  ~OcularWM();

  void Loop();

 protected:
  void changeToAssetDirectory();
  void setupOVR();
  void setupSDL();

  void setupOgre();
  void setupOgreWindow();
  void setupOgreVR();

  void syncOrientationFromHMD(double frame_time);

  SDL_Window* mWindow;
  OculusVars* mOVR;

  Ogre::RenderWindow* mOgreWindow;
  Ogre::Root* mOgreRoot;
  Ogre::SceneManager* mScene;
  Ogre::SceneNode* mRootNode;

  std::vector<Ogre::Camera*> mCameras;
  // for position, and possible any scripted animation, or being the main
  // camera node in any created scene.
  Ogre::SceneNode* mCameraNode;
  // for syncing orientation with HMD, ignore orientation inheritance
  Ogre::SceneNode* mEyesNode;

  bool mUseMainMonitorInstead;
};
