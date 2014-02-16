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
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

// custom C++
#include <boost/chrono.hpp>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/utility.hpp>

#include <OgreRoot.h>
#include <OgreOctreePlugin.h>
#include <OgreGLPlugin.h>
#include <OgreRenderWindow.h>
#include <OgreManualObject.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreEntity.h>

#include <OVR.h>

#ifdef _WIN32
typedef HWND WindowID;
#endif

static inline void Trace(std::string msg) {
#ifdef _WIN32
  OutputDebugStringA(msg.c_str());
#endif
}

static inline void Convert(Ogre::Matrix4& dst, const OVR::Matrix4f src) {
  ::memcpy(dst[0], src.M, 16 * sizeof(src.M[0][0]));
}

#include "ScopedStopwatch.hpp"
