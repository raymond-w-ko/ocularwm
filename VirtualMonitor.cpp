#include "StableHeaders.hpp"
#include "VirtualMonitor.hpp"

using namespace std;
using namespace boost;
using namespace Ogre;

unsigned int VirtualMonitor::msResourceCounter = 0;
int VirtualMonitor::msPixelSize = 4;

VirtualMonitor::~VirtualMonitor() {
  if (mTexture.get()) {
    std::string name = mTexture->getName();
    mTexture.setNull();
    Ogre::TextureManager::getSingleton().remove(name);
  }
}

VirtualMonitor::VirtualMonitor(SceneManager* scene, HWND hwnd)
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
  mMaterial = MaterialManager::getSingleton().create(
      "VirtualMonitorMaterial" + lexical_cast<string>(mID),
      ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
  mPass = mMaterial->getTechnique(0)->getPass(0);
  //mPass->setSceneBlending(SBT_TRANSPARENT_ALPHA);
  mPass->setEmissive(1, 1, 1);
  mPass->createTextureUnitState();
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

  bool need_texture_recreate = false;
  if (mWidth != screenshot->mWidth || mHeight != screenshot->mHeight || mTexture.isNull()) {
    need_texture_recreate = true;
  }
  mWidth = screenshot->mWidth;
  mHeight = screenshot->mHeight;
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
        PF_BYTE_BGRA,
        TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
    mPass->getTextureUnitState(0)->setTextureName(texture_name);
  }

  HardwarePixelBufferSharedPtr buffer = mTexture->getBuffer();
  buffer->lock(HardwareBuffer::HBL_DISCARD);
  const PixelBox& box = buffer->getCurrentLock();
  memcpy(box.data, screenshot->mPixels, mWidth * mHeight * msPixelSize);
  buffer->unlock();
}

void VirtualMonitor::FocusAtUser() {
  mNode->lookAt(Vector3(0, 0, 0), Node::TS_PARENT, Vector3::UNIT_Z);
}
