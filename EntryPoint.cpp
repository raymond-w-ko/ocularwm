#include "stdafx.h"

#include "OcularWM.h"

INT WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT)
{
    try {
        OcularWM app;
        app.go();
    } catch (Ogre::Exception& e) {
        MessageBox(NULL, e.getFullDescription().c_str(),
                   "An OGRE exception has occurred!",
                   MB_OK | MB_ICONERROR | MB_TASKMODAL);
    }

    return 0;
}
