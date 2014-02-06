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
  OcularWM();
  ~OcularWM();

  void Loop();
 protected:
  void changeToAssetDirectory();
  void setupOVR();
  void setupSDL();
  void setupOgre();

  SDL_Window* mWindow;
  OculusVars* mOVR;

  Ogre::RenderWindow* mOgreWindow;
  Ogre::Root* mOgreRoot;
  Ogre::SceneManager* mScene;

  bool mUseMainMonitorInstead;
};
