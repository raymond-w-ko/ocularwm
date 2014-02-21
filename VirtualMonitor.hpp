#pragma once

#include "ScreenshotProducer.hpp"
class VirtualMonitor {
public:
    struct Pixel {
        unsigned char b;
        unsigned char g;
        unsigned char r;
        unsigned char a;
    };

    static const Ogre::PixelFormat msBestPixelFormatGPU;
    static const Ogre::PixelFormat msBestPixelFormatCPU;

public:
    VirtualMonitor(Ogre::SceneManager* scene, WindowID hwnd);
    ~VirtualMonitor();

    // update monitor with a given screenshot
    void Blit(ScreenshotPtr screenshot);

    void ChangeResolution(int width, int height);
    void Blank(Ogre::ColourValue colour);

    const Ogre::PixelBox& Lock();
    void Unlock();

    void SetMonitorWidth(float scale);
    void SetPosition(float x, float y, float z);
    void UpdateScale();

    void WritePixels(Ogre::uint8* src);

    void FocusAtUser();

protected:
    static unsigned int msResourceCounter;
    static int msPixelSize;

protected:
    void createObject();
    void createMaterial();

    Ogre::SceneManager* mScene;
    WindowID mHwnd;
    unsigned mID;

    Ogre::ManualObject* mScreen;
    Ogre::SceneNode* mNode;
    Ogre::MaterialPtr mMaterial;
    Ogre::Pass* mPass;

    int mWidth;
    int mHeight;
    float mAspectRatio;
    float mMonitorWidth;

    Ogre::TexturePtr mTexture;
};
typedef std::shared_ptr<VirtualMonitor> VirtualMonitorPtr;
