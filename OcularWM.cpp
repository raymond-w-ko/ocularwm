#include "StableHeaders.hpp"
#include "OcularWM.hpp"

// https://github.com/scrawl/ogre-sdl2-test/blob/master/source/main.cpp

OcularWM::~OcularWM() {
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
  for (;;) {
    mOgreRoot->renderOneFrame();

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
  }
OcularWM_Loop_quit:
  return;
}

void OcularWM::setupOgre() {
  mOgreRoot = new Ogre::Root("", "", "OgreLog.txt");
  Ogre::Root* root = mOgreRoot;

  root->installPlugin(new Ogre::GLPlugin);
  root->setRenderSystem(
      root->getRenderSystemByName("OpenGL Rendering Subsystem"));

  root->initialise(false);

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

  mScene = root->createSceneManager(Ogre::ST_GENERIC);
  Ogre::Camera* camera = mScene->createCamera("cam");
  Ogre::Viewport* vp = mOgreWindow->addViewport(camera);
  vp->setBackgroundColour(Ogre::ColourValue(0, 1, 0, 1));

  	Ogre::ManualObject* manual = mScene->createManualObject();

    // Use identity view/projection matrices to get a 2d quad
    manual->setUseIdentityProjection(true);
    manual->setUseIdentityView(true);

    Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create("mat", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    material->getTechnique(0)->getPass(0)->setLightingEnabled(false);
    manual->begin(material->getName());

    manual->position(-1, -1, 0.0);
    manual->textureCoord(0, 1);
    manual->colour(Ogre::ColourValue(1,0,0,1));

    manual->position(1, -1, 0.0);
    manual->textureCoord(1, 1);
    manual->colour(Ogre::ColourValue(1,1,0,1));

    manual->position(1, 1, 0.0);
    manual->textureCoord(1, 0);
    manual->colour(Ogre::ColourValue(1,1,1,1));

    manual->position(-1, 1, 0.0);
    manual->textureCoord(0, 0);
    manual->colour(Ogre::ColourValue(1,0,1,1));

    manual->quad(0,1,2,3);

    manual->end();

    manual->setBoundingBox(Ogre::AxisAlignedBox::BOX_INFINITE);

    mScene->getRootSceneNode()->createChildSceneNode()->attachObject(manual);
}
