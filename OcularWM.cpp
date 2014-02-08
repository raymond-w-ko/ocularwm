#include "StableHeaders.hpp"
#include "OcularWM.hpp"

// https://github.com/scrawl/ogre-sdl2-test/blob/master/source/main.cpp

OcularWM::~OcularWM() {
  // WTFLOL, how do I destructor?
  // if you don't call this, then SDK deadlocks with threads waiting for each
  // other. apparently their smart pointer implementation is very tricky and
  // doesn't actually properly destroy objects when they go out of scope!
  mOVR->Sensor.Clear();
  mOVR->HMD.Clear();
  mOVR->DeviceManager.Clear();
  delete mOVR->SensorFusion;
  mOVR->SensorFusion = NULL;
  delete mOVR;
  mOVR = NULL;
  OVR::System::Destroy();

  delete mOgreRoot;
  mOgreRoot = nullptr;

  SDL_DestroyWindow(mWindow);
  SDL_Quit();
}

OcularWM::OcularWM()
    : mOVR(nullptr),
      mUseMainMonitorInstead(true) {
  changeToAssetDirectory();
  setupOVR();
  setupSDL();
  setupOgre();
}

void OcularWM::changeToAssetDirectory() {
#ifdef _WIN32
  TCHAR path[8192];
  DWORD i = ::GetModuleFileName(NULL, path, 8192 * sizeof(TCHAR));
  if (!i) {
    throw OcularWMException("changeToAssetDirectory() failure");
  }
  // back track to directory of program
  while (path[i] != '\\') {
    i--;
  }
  i--;
  // back track to parent directory
  while (path[i] != '\\') {
    i--;
  }
  i++;
  path[i] = 0;
  if (!SetCurrentDirectory(path)) {
    throw OcularWMSetupException("changeToAssetDirectory() failure");
  }
#else
# error "TODO: implement this"
#endif
}

void OcularWM::setupOVR() {
  OVR::System::Init(OVR::Log::ConfigureDefaultLog(OVR::LogMask_All));
  mOVR = new OculusVars;
  OculusVars* ovr = mOVR;

  ovr->DeviceManager = *OVR::DeviceManager::Create();
  if (ovr->DeviceManager) {
    ovr->HMD =
        *ovr->DeviceManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
    if (ovr->HMD) {
      ovr->HMD->GetDeviceInfo(&ovr->HMDInfo);
      ovr->Sensor = *ovr->HMD->GetSensor();

      if (ovr->Sensor) {
        ovr->SensorFusion = new OVR::SensorFusion;
        ovr->SensorFusion->AttachToSensor(ovr->Sensor);
      }
    }
  }

  ovr->StereoConfig.SetDistortionFitPointVP(-1.0f, 0.0f);

  if (ovr->HMD) {
    ovr->StereoConfig.SetHMDInfo(ovr->HMDInfo);
    ovr->StereoConfig.SetFullViewport(OVR::Util::Render::Viewport(
            0, 0,
            ovr->HMDInfo.HResolution,
            ovr->HMDInfo.VResolution));
  }
  ovr->StereoConfig.SetStereoMode(
      OVR::Util::Render::Stereo_LeftRight_Multipass);
}

void OcularWM::setupSDL() {
  SDL_Init(SDL_INIT_VIDEO);

  const auto& hmd = mOVR->StereoConfig.GetHMDInfo();
  int x = SDL_WINDOWPOS_UNDEFINED;
  int y = SDL_WINDOWPOS_UNDEFINED;
  if (!mUseMainMonitorInstead) {
    x = hmd.DesktopX;
    y = hmd.DesktopY;
  }

  Uint32 window_flags = SDL_WINDOW_SHOWN;
  if (!mUseMainMonitorInstead) {
    window_flags |= SDL_WINDOW_BORDERLESS;
  } else {
    ;
  }

  mWindow = SDL_CreateWindow(
      "OcularWM",
      x, y,
      hmd.HResolution, hmd.VResolution,
      window_flags);
  if (!mWindow) {
    throw OcularWMSetupException("could not create window");
  }
}

void OcularWM::Loop() {
  typedef boost::chrono::high_resolution_clock clock;

  // total amount of time (in seconds) to complete one frame
  double frame_time = 0.0;
  // total amount of time (in seconds) to just do the rendering
  double render_time = 0.0;

  for (;;) {
    clock::time_point frame_begin_time = clock::now();

    SDL_Event event;
    if (!SDL_PollEvent(&event)) {
      continue;
    }

    switch (event.type) {
      case SDL_QUIT:
        goto OcularWM_Loop_quit;
      default:
        // pass
        break;
    }

    syncOrientationFromHMD(render_time);

    clock::time_point render_begin_time = clock::now();
    mOgreRoot->renderOneFrame();

    clock::time_point end_time = clock::now();

    boost::chrono::nanoseconds ns;

    ns = end_time - render_begin_time;
    render_time = static_cast<double>(ns.count());
    render_time /= 1e9;

    ns = end_time - frame_begin_time;
    frame_time = static_cast<double>(ns.count());
    frame_time /= 1e9;
  }
OcularWM_Loop_quit:
  return;
}

void OcularWM::setupOgre() {
  mOgreRoot = new Ogre::Root("", "", "OgreLog.txt");
  Ogre::Root* root = mOgreRoot;

  root->installPlugin(new Ogre::OctreePlugin);

  root->installPlugin(new Ogre::GLPlugin);
  root->setRenderSystem(
      root->getRenderSystemByName("OpenGL Rendering Subsystem"));

  root->initialise(false);

  setupOgreWindow();
  setupOgreVR();
}

void OcularWM::setupOgreWindow() {
  struct SDL_SysWMinfo wm_info;
  SDL_VERSION(&wm_info.version);

  if (SDL_GetWindowWMInfo(mWindow, &wm_info) == SDL_FALSE) {
    throw OcularWMSetupException("could not get WM info from SDL");
  }

  Ogre::String win_handle;
  switch (wm_info.subsystem) {
    //case SDL_SYSWM_X11:
      //win_handle = Ogre::StringConverter::toString(
          //(unsigned long)wm_info.info.x11.window);
      //break;
    case SDL_SYSWM_WINDOWS:
      win_handle = Ogre::StringConverter::toString(
          (unsigned long)wm_info.info.win.window);
      break;
    default:
      throw OcularWMSetupException("unknown WM!");
      break;
  }

  Ogre::NameValuePairList params;
  params.insert(std::make_pair("title", "OcularWM"));
  params.insert(std::make_pair("FSAA", "0"));
  params.insert(std::make_pair("vsync", "true"));
  params.insert(std::make_pair("parentWindowHandle", win_handle));
  params.insert(std::make_pair("left", "0"));
  params.insert(std::make_pair("top", "0"));

  const auto& hmd = mOVR->StereoConfig.GetHMDInfo();
  mOgreWindow = Ogre::Root::getSingleton().createRenderWindow(
      "OcularWM",
      hmd.HResolution, hmd.VResolution,
      false,
      &params);
  mOgreWindow->setVisible(true);
}

void OcularWM::setupOgreVR() {
  mScene = mOgreRoot->createSceneManager(Ogre::ST_GENERIC);
  Ogre::Camera* camera;

  camera = mScene->createCamera("VR_Left_Eye");
  mCameras.push_back(camera);
  camera = mScene->createCamera("VR_Right_Eye");
  mCameras.push_back(camera);
  camera = mScene->createCamera("VR_Center_Eye");
  mCameras.push_back(camera);

  for (auto cam : mCameras) {
    cam->setOrientation(Ogre::Quaternion::IDENTITY);
  }

  mRootNode = mScene->getRootSceneNode();
  mCameraNode = mRootNode->createChildSceneNode("Main_Camera_Node");

  mEyesNode = mCameraNode->createChildSceneNode("VR_Eyes_Node");
  mEyesNode->setInheritOrientation(false);
  mEyesNode->attachObject(mCameras[0]);
  mEyesNode->attachObject(mCameras[1]);
  mEyesNode->attachObject(mCameras[2]);

  // TODO: account for near far plane distances once SDK has support for this
  Ogre::Matrix4 m;

  const OVR::Matrix4f& left_proj = mOVR->StereoConfig.GetEyeRenderParams(
      OVR::Util::Render::StereoEye_Left).Projection;
  if (sizeof(*m[0]) != sizeof(left_proj.M[0][0])) {
    throw OcularWMException("matrix member size mismatch");
  }
  Convert(m, left_proj);
  mCameras[kLeft]->setCustomProjectionMatrix(true, m);

  const OVR::Matrix4f& right_proj = mOVR->StereoConfig.GetEyeRenderParams(
      OVR::Util::Render::StereoEye_Right).Projection;
  Convert(m, right_proj);
  mCameras[kRight]->setCustomProjectionMatrix(true, m);
}

void OcularWM::syncOrientationFromHMD(double render_time) {
  OVR::Quatf q = mOVR->SensorFusion->GetPredictedOrientation(
      static_cast<float>(render_time));
  Ogre::Quaternion orientation(q.w, q.x, q.y, q.z);
  mEyesNode->setOrientation(orientation);

  //mCameras[kCenter]->updateView();
  // line below assumes the above is called internally; we can't do so
  // ourselve since the method is protected protected.
  mCameras[kCenter]->getDerivedOrientation();

  Ogre::Matrix4 m;
  Ogre::Matrix4 center_view = mCameras[kCenter]->getViewMatrix();

  // TODO: make sure eye offset has been converted to world scale
  // if we assume 1 world unit == 1 meter, then we don't have to anything
  float world_unit_scale = 1.0f;

  const OVR::Matrix4f& left_adjust = mOVR->StereoConfig.GetEyeRenderParams(
      OVR::Util::Render::StereoEye_Left).ViewAdjust;
  Convert(m, left_adjust);
  m[0][3] *= world_unit_scale;
  m = m * center_view;
  mCameras[kLeft]->setCustomViewMatrix(true, m);

  const OVR::Matrix4f& right_adjust = mOVR->StereoConfig.GetEyeRenderParams(
      OVR::Util::Render::StereoEye_Right).ViewAdjust;
  Convert(m, right_adjust);
  m[0][3] *= world_unit_scale;
  mCameras[kRight]->setCustomViewMatrix(true, m);

}
