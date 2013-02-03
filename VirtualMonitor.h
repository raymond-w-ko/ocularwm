#pragma once

class VirtualMonitor {
public:
    struct Pixel {
        unsigned char b;
        unsigned char g;
        unsigned char r;
        //unsigned char a;
    };

    static std::shared_ptr<VirtualMonitor> Create(
        Ogre::SceneManager* sceneMgr,
        std::string name,
        int width, int height);
    ~VirtualMonitor();

    Pixel& GetPixel(void* pixels,
                    size_t x, size_t y)
    {
        return *reinterpret_cast<VirtualMonitor::Pixel*>(
            &(static_cast<Ogre::uint8*>(pixels))
            [(y * mWidth * mPixelSize) + (x * mPixelSize)]);
    }

    void ChangeResolution(int width, int height);
    void Blank(Ogre::ColourValue colour);

    const Ogre::PixelBox& Lock();
    void Unlock();

    void SetScale(float scale);
    void SetPosition(float x, float y, float z);
    void UpdateScale();

protected:
    VirtualMonitor(
        Ogre::SceneManager* sceneMgr,
        std::string name,
        int width, int height);

    void createMaterial();
    void createObject();

    static unsigned int msResourceCounter;

    std::string mName;

    int mPixelSize;
    int mWidth;
    int mHeight;
    float mAspectRatio;
    float mUserScale;

    Ogre::TexturePtr mTexture;
    Ogre::HardwarePixelBufferSharedPtr mPixelBuffer;
    Ogre::MaterialPtr mMaterial;
    /// managed by Ogre::SceneManager, no need to be a smart pointer
    Ogre::ManualObject* mManualObject;
    /// managed by Ogre::SceneManager, no need to be a smart pointer
    Ogre::SceneNode* mSceneNode;

    /// outside our scope
    Ogre::SceneManager* mSceneMgr;
};
typedef std::shared_ptr<VirtualMonitor> VirtualMonitorPtr;
