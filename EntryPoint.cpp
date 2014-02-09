#include "StableHeaders.hpp"
#include "OcularWM.hpp"

#ifdef __cplusplus
extern "C"
#endif
int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

  try {
    OcularWM ocularWM;
    ocularWM.Loop();
  } catch (std::exception& e) {
    Trace(e.what());
  }

  return 0;
}

  /*
void OcularWM::createScene()
{
    mSceneMgr->setSkyBox(true, "Examples/CloudyNoonSkyBox");

    // make everything broken unless they are self emissive
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.0f, 0.0f, 0.0f));

    VirtualMonitorPtr mon0 = VirtualMonitor::Create(
        mSceneMgr, "primary", 1920, 1080);
    mon0->SetScale(52);
    mon0->SetPosition(0, 160, -45);
    mMonitors.push_back(mon0);

    mScreenshotProducer.StartBackgroundLoop();

    mWindowTitles.clear();

    string titles;
    for (auto& title : mWindowTitles) {
        titles += title + "\n";
    }
    //::MessageBox(nullptr, titles.c_str(), "", MB_OK);
}

void OcularWM::createFrameListener()
{
    mWindow->getCustomAttribute("WINDOW", &mHwnd);
    ::SetWindowLongPtr(mHwnd, GWLP_WNDPROC, (LONG_PTR)&OcularWM::WindowProc);
    ::SendMessage(mHwnd, WM_USER, (WPARAM)this, 0);
}

static int32 sScreenOffset = 0;

bool OcularWM::frameRenderingQueued(const Ogre::FrameEvent& evt)
{
    //if (!BaseApplication::frameRenderingQueued(evt))
        //return false;

    std::vector<HWND> hwnds = mScreenshotProducer.GetVisibleWindows();
    for (HWND hwnd : hwnds) {
        ScreenshotPtr shot = mScreenshotProducer.Get(hwnd);
        if (!shot)
            continue;

        HWND hwnd = shot->hwnd;

        if (!mMonitorFromHwnd[hwnd]) {
            VirtualMonitorPtr monitor = VirtualMonitor::Create(
                mSceneMgr, "", shot->mWidth, shot->mHeight);
            monitor->SetScale(52);
            monitor->SetPosition(sScreenOffset, 160, -45);
            sScreenOffset -= 300;

            mMonitors.push_back(monitor);
            mMonitorFromHwnd[hwnd] = monitor;
        }

        mMonitorFromHwnd[hwnd]->ChangeResolution(shot->mWidth, shot->mHeight);
        mMonitorFromHwnd[hwnd]->WritePixels(shot->mPixels);
    }

    return true;
}


LRESULT CALLBACK OcularWM::WindowProc(HWND hwnd,
                                      UINT uMsg,
                                      WPARAM wParam,
                                      LPARAM lParam)
{
    static OcularWM* self = NULL;

    switch (uMsg) {
    case WM_USER: {
        self = reinterpret_cast<OcularWM*>(wParam);
    }
    case WM_MOUSEMOVE: {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
    }
    case WM_CHAR: {
        break;
    }
    default: {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    }

    return 0;
}
*/
