#pragma once

// standard C
#ifdef _WIN32
#include <tchar.h>
#include <windows.h>
#endif

// custom C
#include <SDL.h>
#include <SDL_syswm.h>

// standard C++
#include <cstdio>
#include <cstdlib>

#include <exception>
#include <string>

// custom C++
#include <boost/chrono.hpp>

#include <OgreRoot.h>
#include <OgreOctreePlugin.h>
#include <OgreGLPlugin.h>
#include <OgreRenderWindow.h>
#include <OgreManualObject.h>

#include <OVR.h>

static inline void Trace(std::string msg) {
#ifdef _WIN32
  OutputDebugStringA(msg.c_str());
#endif
}

static inline void Convert(Ogre::Matrix4& dst, const OVR::Matrix4f src) {
  ::memcpy(dst[0], src.M, 16 * sizeof(src.M[0][0]));
}
