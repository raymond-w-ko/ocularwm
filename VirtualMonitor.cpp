#include "stdafx.h"
#include "VirtualMonitor.h"

using namespace std;
using namespace boost;
using namespace Ogre;

unsigned int VirtualMonitor::msResourceCounter = 0;

VirtualMonitor::~VirtualMonitor()
{
    ;
}

std::shared_ptr<VirtualMonitor> VirtualMonitor::Create(
    SceneManager* sceneMgr,
    std::string name,
    int width, int height)
{
    std::shared_ptr<VirtualMonitor> mon(new VirtualMonitor(
            sceneMgr, name, width, height));
    return mon;
}

VirtualMonitor::VirtualMonitor(SceneManager* sceneMgr,
                               std::string name,
                               int width, int height) :
    mSceneMgr(sceneMgr),
    mName(name),
    mManualObject(nullptr),
    mSceneNode(nullptr),
    mWidth(-1), mHeight(-1), mPixelSize(-1), mAspectRatio(-1),
    mUserScale(1)
{
    ChangeResolution(width, height);
    createMaterial();
    createObject();

    Blank(ColourValue(0, 1, 0, 1));
}

void VirtualMonitor::createMaterial()
{
    mMaterial = MaterialManager::getSingleton().create(
        "VirtualMonitorMaterial" + lexical_cast<string>(msResourceCounter++),
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mMaterial->getTechnique(0)->getPass(0)->createTextureUnitState(
        mTexture->getName());
    mMaterial->getTechnique(0)->getPass(0)->setSceneBlending(SBT_TRANSPARENT_ALPHA);
    mMaterial->getTechnique(0)->getPass(0)->setEmissive(1, 1, 1);
}

void VirtualMonitor::createObject()
{
    ManualObject* mManualObject = mSceneMgr->createManualObject(
        "VirtualMonitorManualObject" + lexical_cast<string>(msResourceCounter++));
    mManualObject->begin(mMaterial->getName(), RenderOperation::OT_TRIANGLE_LIST);
    mManualObject->position(-1, -1, +0);
    mManualObject->textureCoord(0, 0);

    mManualObject->position(+1, -1, +0);
    mManualObject->textureCoord(1, 0);

    mManualObject->position(+1, +1, +0);
    mManualObject->textureCoord(1, 1);

    mManualObject->position(-1, +1, +0);
    mManualObject->textureCoord(0, 1);

    mManualObject->index(0);
    mManualObject->index(1);
    mManualObject->index(2);
    mManualObject->index(2);
    mManualObject->index(3);
    mManualObject->index(0);
    mManualObject->end();

    mSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();
    mSceneNode->attachObject(mManualObject);
}

void VirtualMonitor::ChangeResolution(int width, int height)
{
    // enforce minimums or crash!
    if (width == 0)
        width = 1;
    if (height == 0)
        height = 1;

    if (mWidth == width && mHeight == height)
        return;

    mWidth = width;
    mHeight = height;
    mAspectRatio = static_cast<float>(mWidth) / static_cast<float>(mHeight);
    mPixelSize = 4; // for B, G, R

    mTexture = TextureManager::getSingleton().createManual(
        "VirtualMonitorTexture" + lexical_cast<string>(msResourceCounter++),
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
        TEX_TYPE_2D,
        mWidth, mHeight,
        0, // TODO: do mipmaps visually improve this / even work?
        PF_BYTE_BGR,
        TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

    mPixelBuffer = mTexture->getBuffer();

    // update texture if material has already been created
    if (mMaterial.get()) {
        TextureUnitState* tus =
            mMaterial->getTechnique(0)->getPass(0)->getTextureUnitState(0);
        if (tus) {
            tus->setTextureName(mTexture->getName());
        }
    }

    // rescale only if we are not in intial construction phase
    if (mSceneNode)
        UpdateScale();
}

void VirtualMonitor::Blank(Ogre::ColourValue colour)
{
    const PixelBox& box = Lock();

    for (size_t y = 0; y < mHeight; ++y) {
        for (size_t x = 0; x < mWidth; ++x) {
            Pixel& pixel = GetPixel(box.data, x, y);
            pixel.b = colour.g;
            pixel.g = colour.g;
            pixel.r = colour.r;
            pixel.a = colour.a;
        }
    }

    Unlock();
}

const Ogre::PixelBox& VirtualMonitor::Lock()
{
    // for best performance use HBL_DISCARD!
    mPixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
    const PixelBox& pixelBox = mPixelBuffer->getCurrentLock();
    return pixelBox;
}

void VirtualMonitor::Unlock()
{
    mPixelBuffer->unlock();
}

void VirtualMonitor::SetScale(float scale)
{
    mUserScale = scale;
    mSceneNode->setScale(
        1.0 * scale,
        (1.0 / mAspectRatio) * scale,
        1.0);
}

void VirtualMonitor::SetPosition(float x, float y, float z)
{
    mSceneNode->setPosition(x, y, z);
}

void VirtualMonitor::UpdateScale()
{
    this->SetScale(mUserScale);
}

void VirtualMonitor::WritePixels(uint8* src)
{
    uint8* dst = static_cast<uint8*>(this->Lock().data);
    const size_t count = mWidth * mPixelSize * mHeight;
    memcpy(dst, src, count);
    this->Unlock();
}
