#include "StableHeaders.hpp"
#include "OcularWM.h"

OcularWM::OcularWM()
{
  SDL_Init(SDL_INIT_VIDEO);
}

OcularWM::~OcularWM()
{
  SDL_Quit();
}

OcularWM::Loop() {
  ;
}

