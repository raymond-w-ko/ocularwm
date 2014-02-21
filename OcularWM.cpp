#include "StableHeaders.hpp"
#include "OcularWM.hpp"

static void CALLBACK WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD         event,
    HWND          hwnd,
    LONG          idObject,
    LONG          idChild,
    DWORD         idEventThread,
    DWORD         dwmsEventTime) {
  if (event == EVENT_OBJECT_DESTROY && idObject == OBJID_WINDOW) {
  }
}

// https://github.com/scrawl/ogre-sdl2-test/blob/master/source/main.cpp
OcularWM::~OcularWM() {
  mScreenshotProducer.Stop();
  mVirtualMonitorManager.reset();

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

  SDL_DestroyWindow(mWindow);
  SDL_Quit();

  delete mOgreRoot;
  mOgreRoot = nullptr;
}

OcularWM::OcularWM()
    : mOVR(nullptr),
      mUseMainMonitorInstead(true) {
  changeToAssetDirectory();
  setupOVR();
  setupSDL();
  setupOgre();

  mScreenshotProducer.Start();
  mVirtualMonitorManager = std::make_shared<VirtualMonitorManager>(
      &mScreenshotProducer, mScene);

  SetWinEventHook(
      EVENT_OBJECT_DESTROY, EVENT_OBJECT_DESTROY, NULL,
      WinEventProc, 0, 0,
      WINEVENT_SKIPOWNPROCESS | WINEVENT_SKIPOWNPROCESS);
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
  int x = 0;
  int y = 0;
  if (!mUseMainMonitorInstead) {
    x = hmd.DesktopX;
    y = hmd.DesktopY;
  }

  Uint32 window_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS;

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

  // total amount of time (in seconds) to just blit to screens
  double blit_time = 0.0;
  // total amount of time (in seconds) to process input
  double input_time = 0.0;
  // total amount of time (in seconds) to just do the rendering
  double render_time = 0.0;
  // total amount of time (in seconds) to complete one frame
  double frame_time = 0.0;

  boost::chrono::nanoseconds ns;
  for (;;) {
    clock::time_point frame_begin_time = clock::now();

    mVirtualMonitorManager->Update(frame_time);
    ns = clock::now() - frame_begin_time;
    blit_time = static_cast<double>(ns.count()) / 1e9;

    clock::time_point input_begin_time = clock::now();
    if (!processSdlInput()) {
      break;
    }
    syncOrientationFromHMD(render_time);
    ns = clock::now() - input_begin_time;
    input_time = static_cast<double>(ns.count()) / 1e9;

    clock::time_point render_begin_time = clock::now();
    mOgreRoot->renderOneFrame();

    clock::time_point end_time = clock::now();

    ns = end_time - render_begin_time;
    render_time = static_cast<double>(ns.count()) / 1e9;

    ns = end_time - frame_begin_time;
    frame_time = static_cast<double>(ns.count()) / 1e9;

    char buffer[4096];
    sprintf(buffer, "blit: %f, input: %f, render: %f, frame:%f, FPS: %f\n",
            blit_time, input_time, render_time, frame_time, 1.0 / frame_time);
    Trace(buffer);
  }
}

void OcularWM::setupOgre() {
  mOgreRoot = new Ogre::Root("", "", "OgreLog.txt");
  Ogre::Root* root = mOgreRoot;

  root->installPlugin(new Ogre::OctreePlugin);

  root->installPlugin(new Ogre::GLPlugin);
  Ogre::RenderSystem* rs = root->getRenderSystemByName("OpenGL Rendering Subsystem");
  root->setRenderSystem(rs);
  rs->setFixedPipelineEnabled(false);
  

  root->initialise(false);

  setupOgreWindow();
  setupOgreMedia();
  setupOgreVRRender();
  setupOgreVRCameras();
  setupScene();
}

void OcularWM::setupOgreWindow() {
  struct SDL_SysWMinfo wm_info;
  SDL_VERSION(&wm_info.version);

  if (SDL_GetWindowWMInfo(mWindow, &wm_info) == SDL_FALSE) {
    throw OcularWMSetupException("could not get WM info from SDL");
  }

  WindowID myHwnd = 0;
  Ogre::String win_handle;
  switch (wm_info.subsystem) {
    //case SDL_SYSWM_X11:
      //win_handle = Ogre::StringConverter::toString(
          //(unsigned long)wm_info.info.x11.window);
      //break;
    case SDL_SYSWM_WINDOWS:
      myHwnd = wm_info.info.win.window;
      win_handle = Ogre::StringConverter::toString((unsigned long)myHwnd);
      break;
    default:
      throw OcularWMSetupException("unknown WM!");
      break;
  }
  mScreenshotProducer.SetParentHwnd(myHwnd);

  Ogre::NameValuePairList params;
  params.insert(std::make_pair("title", "OcularWM"));
  params.insert(std::make_pair("FSAA", "0"));
  params.insert(std::make_pair("vsync", "false"));
  params.insert(std::make_pair("parentWindowHandle", win_handle));
  params.insert(std::make_pair("left", "0"));
  params.insert(std::make_pair("top", "0"));

  const auto& hmd = mOVR->StereoConfig.GetHMDInfo();
  mRenderWindow = Ogre::Root::getSingleton().createRenderWindow(
      "OcularWM",
      hmd.HResolution, hmd.VResolution,
      false,
      &params);
  mRenderWindow->setVisible(true);
}

bool OcularWM::processSdlInput() {
  SDL_Event event;
  if (!SDL_PollEvent(&event)) {
    return true;
  }

  switch (event.type) {
    case SDL_QUIT:
      return false;
    default:
      // pass
      break;
  }

  return true;
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
  m = m * center_view;
  mCameras[kRight]->setCustomViewMatrix(true, m);
}

void OcularWM::setupOgreMedia() {
  Ogre::ResourceGroupManager& man = Ogre::ResourceGroupManager::getSingleton();

  // root level stuff
  man.addResourceLocation(
      "media",
      "FileSystem",
      Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
      false);

  using namespace boost::filesystem;

  // some ZIP packs
  auto end = directory_iterator();
  for (auto iter = directory_iterator("media/packs"); iter != end; ++iter) {
    const path& item = iter->path();
    if (!is_regular_file(item))
      continue;

    man.addResourceLocation(
        item.generic_string(),
        "Zip",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        false);
  }

  // not safe to do this before RenderWindow exists
  man.initialiseAllResourceGroups();
}

void OcularWM::setupOgreVRRender() {
  const auto& hmd = mOVR->StereoConfig.GetHMDInfo();

  float scale = mOVR->StereoConfig.GetDistortionScale();
  unsigned width = (unsigned)(hmd.HResolution * scale);
  unsigned height = (unsigned)(hmd.VResolution * scale);

  const int num_mipmaps = 0;
  const bool hw_gamma_correct = false;
  const unsigned aa = 4;
  mRenderTexture = Ogre::TextureManager::getSingleton().createManual(
      "VR_Render_Target",
      Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
      Ogre::TEX_TYPE_2D,
      width, height,
      num_mipmaps,
      Ogre::PF_R8G8B8,
      Ogre::TU_RENDERTARGET,
      NULL,
      hw_gamma_correct,
      aa);
  mRenderTarget = mRenderTexture->getBuffer()->getRenderTarget();

  Ogre::MaterialManager& material_man = Ogre::MaterialManager::getSingleton();
  Ogre::Pass* pass = NULL;
  Ogre::TextureUnitState* tus = NULL;

  std::string mat_name = "OcularWM/BarrelChromaAberrFix";
  Ogre::MaterialPtr base_material = material_man.getByName(mat_name);

  pass = base_material->getTechnique(0)->getPass(0);
  tus = pass->getTextureUnitState(0);
  tus->setTextureName("VR_Render_Target");

  std::string left_mat_name = mat_name + "_L";
  Ogre::MaterialPtr left_mat = base_material->clone(left_mat_name);
  configureMaterial(kLeft, left_mat);

  std::string right_mat_name = mat_name + "_R";
  Ogre::MaterialPtr right_mat = base_material->clone(right_mat_name);
  configureMaterial(kRight, right_mat);

  Ogre::AxisAlignedBox box;
  box.setInfinite();

  const bool include_tex_coord = true;
  Ogre::Rectangle2D* left_rect = new Ogre::Rectangle2D(include_tex_coord);
  left_rect->setBoundingBox(box);
  left_rect->setCorners(-1.0f, 1.0f, 0.0f, -1.0f);
  left_rect->setUVs(
      Ogre::Vector2(0.0f, 0.0f),
      Ogre::Vector2(0.0f, 1.0f),
      Ogre::Vector2(0.5f, 0.0f),
      Ogre::Vector2(0.5f, 1.0f));
  left_rect->setMaterial(left_mat_name);

  Ogre::Rectangle2D* right_rect = new Ogre::Rectangle2D(include_tex_coord);
  right_rect->setBoundingBox(box);
  right_rect->setCorners(0.0f, 1.0f, 1.0f, -1.0f);
  right_rect->setUVs(
      Ogre::Vector2(0.5f, 0.0f),
      Ogre::Vector2(0.5f, 1.0f),
      Ogre::Vector2(1.0f, 0.0f),
      Ogre::Vector2(1.0f, 1.0f));
  right_rect->setMaterial(right_mat_name);

  mDummyScene = mOgreRoot->createSceneManager(Ogre::ST_GENERIC, "VR");
  Ogre::SceneNode* root = mDummyScene->getRootSceneNode();
  Ogre::SceneNode* node = root->createChildSceneNode("Background");
  node->attachObject(left_rect);
  node->attachObject(right_rect);

  Ogre::Camera* camera = mDummyScene->createCamera("Camera0");
  node->attachObject(camera);
  Ogre::Viewport* vp = mRenderWindow->addViewport(
      camera, 0,
      0.0f, 0.0f, 1.0f, 1.0f);
  vp->setAutoUpdated(true);
}

void OcularWM::configureMaterial(HmdEye hmd_eye, Ogre::MaterialPtr mat) {
  OVR::Util::Render::StereoEye ovr_eye;

  if (hmd_eye == kLeft) {
    ovr_eye = OVR::Util::Render::StereoEye_Left;
  } else if (hmd_eye == kRight) {
    ovr_eye = OVR::Util::Render::StereoEye_Right;
  } else {
    throw OcularWMException("invalid eye specified");
  }

  const auto& hmd = mOVR->StereoConfig.GetHMDInfo();

  float w = 0.5f;
  float h = 1.0f;
  float x;
  if (hmd_eye == kLeft) {
    x = 0.0f;
  } else /*if (hmd_eye == kRight)*/ {
    x = 0.5f;
  }
  float y = 0.0f;

  float as = (float)hmd.HResolution * 0.5f / hmd.VResolution;

  const OVR::Util::Render::StereoEyeParams& params =
      mOVR->StereoConfig.GetEyeRenderParams(ovr_eye);

  float LensCenter[2];
  float ScreenCenter[2];
  float Scale[2];
  float ScaleIn[2];
  float HmdWarpParam[4];
  float ChromAbParam[4];

  // WTF: I should not have to do this
  // WTF2: Oculus also does this in their TinyRoom demo
  float flip = 1.0;
  if (hmd_eye == kRight) {
    flip = -1.0f;
  }
  LensCenter[0] = x + (w + flip * params.pDistortion->XCenterOffset * 0.5f) * 0.5f;
  LensCenter[1] = y + h * 0.5f;

  ScreenCenter[0] = x + w * 0.5f;
  ScreenCenter[1] = y + h * 0.5f;

  float scaleFactor = 1.0f / params.pDistortion->Scale;

  Scale[0] = (w / 2) * scaleFactor;
  Scale[1] = (h / 2) * scaleFactor * as;

  ScaleIn[0] = 2 / w;
  ScaleIn[1] = (2/h) / as;

  for (int i = 0; i < 4; ++i) {
    HmdWarpParam[i] = params.pDistortion->K[i];
  }

  for (int i = 0; i < 4; ++i) {
    ChromAbParam[i] = params.pDistortion->ChromaticAberration[i];
  }

  Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
  Ogre::GpuProgramParametersSharedPtr frag =
      pass->getFragmentProgramParameters();
  frag->setNamedConstant("LensCenter", Ogre::Vector2(LensCenter));
  frag->setNamedConstant("ScreenCenter", Ogre::Vector2(ScreenCenter));
  frag->setNamedConstant("Scale", Ogre::Vector2(Scale));
  frag->setNamedConstant("ScaleIn", Ogre::Vector2(ScaleIn));
  frag->setNamedConstant("HmdWarpParam", Ogre::Vector4(HmdWarpParam));
  if (mat->getName().find("ChromaAberr") != std::string::npos) {
    frag->setNamedConstant("ChromAbParam", Ogre::Vector4(ChromAbParam));
  }
}

void OcularWM::setupOgreVRCameras() {
  mScene = mOgreRoot->createSceneManager(Ogre::ST_GENERIC, "Primary");
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

  mLeftViewport = mRenderTarget->addViewport(
      mCameras[kLeft], 0,
      0.0f, 0.0f, 0.5f, 1.0f);
  mRightViewport = mRenderTarget->addViewport(
      mCameras[kRight], 1,
      0.5f, 0.0f, 0.5f, 1.0f);

  mLeftViewport->setBackgroundColour(Ogre::ColourValue(1, 0, 0, 1));
  mLeftViewport->setAutoUpdated(true);
  mRightViewport->setBackgroundColour(Ogre::ColourValue(0, 0, 1, 1));
  mRightViewport->setAutoUpdated(true);
}

void OcularWM::setupScene() {
  mScene->setSkyBox(true, "Examples/CloudyNoonSkyBox", 1000);
  mDummyScene->setSkyBox(true, "Examples/CloudyNoonSkyBox", 1000);
}
