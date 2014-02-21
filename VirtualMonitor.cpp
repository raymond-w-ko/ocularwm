#include "StableHeaders.hpp"
#include "VirtualMonitor.hpp"

using namespace std;
using namespace boost;
using namespace Ogre;

unsigned int VirtualMonitor::msResourceCounter = 0;
int VirtualMonitor::msPixelSize = 4;
// DON'T CHANGE THIS or massive slowdowns and suttering can occur because of
// internal pixel swapping in the GPU driver
//
// from:
// http://www.opengl.org/wiki/Common_Mistakes
const PixelFormat VirtualMonitor::msBestPixelFormatGPU = PF_R8G8B8A8;
const PixelFormat VirtualMonitor::msBestPixelFormatCPU = PF_B8G8R8A8;

VirtualMonitor::~VirtualMonitor() {
  if (mTexture.get()) {
    std::string name = mTexture->getName();
    mTexture.setNull();
    Ogre::TextureManager::getSingleton().remove(name);
  }
}

VirtualMonitor::VirtualMonitor(SceneManager* scene, WindowID hwnd)
    : mScene(scene),
      mHwnd(hwnd),
      mWidth(-1),
      mHeight(-1),
      mMonitorWidth(0.5f) {
  mID = msResourceCounter++;

  createMaterial();
  createObject();
}

void VirtualMonitor::createObject() {
  mScreen = mScene->createManualObject("VirtualMonitorScreen" + lexical_cast<string>(mID));
  mScreen->begin(mMaterial->getName(), RenderOperation::OT_TRIANGLE_LIST);
  mScreen->position(-1, -1, +0);
  mScreen->textureCoord(0, 0);

  mScreen->position(+1, -1, +0);
  mScreen->textureCoord(1, 0);

  mScreen->position(+1, +1, +0);
  mScreen->textureCoord(1, 1);

  mScreen->position(-1, +1, +0);
  mScreen->textureCoord(0, 1);

  mScreen->index(0);
  mScreen->index(1);
  mScreen->index(2);
  mScreen->index(2);
  mScreen->index(3);
  mScreen->index(0);
  mScreen->end();

  mNode = mScene->getRootSceneNode()->createChildSceneNode();
  mNode->attachObject(mScreen);
}

void VirtualMonitor::createMaterial() {
  auto& material_man = MaterialManager::getSingleton();
  MaterialPtr base = material_man.getByName("OcularWM/VirtualMonitor");
  mMaterial = base->clone(
      "VirtualMonitorMaterial" + lexical_cast<string>(mID));
  mPass = mMaterial->getTechnique(0)->getPass(0);
}

void VirtualMonitor::SetMonitorWidth(float width) {
  mMonitorWidth = width;
  float pixel_scale = mWidth / 1920.0f;
  mNode->setScale(
          1.0f * mMonitorWidth * pixel_scale,
          (1.0f / mAspectRatio) * mMonitorWidth * pixel_scale,
          1.0f);
}

void VirtualMonitor::SetPosition(float x, float y, float z) {
  mNode->setPosition(x, y, z);
}

void VirtualMonitor::UpdateScale() {
  this->SetMonitorWidth(mMonitorWidth);
}

void VirtualMonitor::Blit(ScreenshotPtr screenshot) {
  if (!screenshot) {
    return;
  }

  int width, height;
  Ogre::uint8* pixels;
  bool locked = screenshot->TryLock(&pixels, &width, &height);
  if (!locked) {
    return;
  }
  if (!pixels) {
    screenshot->Unlock();
    return;
  }

  bool need_texture_recreate = false;
  if (mWidth != width || mHeight != height || mTexture.isNull()) {
    need_texture_recreate = true;
  }
  mWidth = width;
  mHeight = height;
  mAspectRatio = static_cast<float>(mWidth) / static_cast<float>(mHeight);

  if (need_texture_recreate) {
    if (mTexture.get()) {
      std::string name = mTexture->getName();
      mTexture.setNull();
      Ogre::TextureManager::getSingleton().remove(name);
    }

    std::string texture_name = "VirtualMonitorTexture" + lexical_cast<string>(mID);
    mTexture = TextureManager::getSingleton().createManual(
        texture_name,
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        TEX_TYPE_2D,
        mWidth, mHeight,
        0, // TODO: do mipmaps visually improve this / even work well with frequent updates
        msBestPixelFormatGPU,
        TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
    mPass->getTextureUnitState(0)->setTextureName(texture_name);
  }

  HardwarePixelBufferSharedPtr pixel_buffer = mTexture->getBuffer();
  PixelBox box(width, height, 1, msBestPixelFormatCPU, pixels);
  pixel_buffer->blitFromMemory(box);
  
  screenshot->SetConsumed(true);
  screenshot->Unlock();

  char buffer[1024];
  sprintf(buffer, "%d, %d - ", width, height);
  //Trace(buffer);
}

void VirtualMonitor::FocusAtUser() {
  mNode->lookAt(Vector3(0, 0, 0), Node::TS_PARENT, Vector3::UNIT_Z);
}
