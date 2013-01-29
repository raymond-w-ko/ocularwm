#include "stdafx.h"
#include "OcularWM.h"

using namespace std;
using namespace boost;
using namespace Ogre;

//-------------------------------------------------------------------------------------
OcularWM::OcularWM(void)
{
}
//-------------------------------------------------------------------------------------
OcularWM::~OcularWM(void)
{
}

//-------------------------------------------------------------------------------------
void OcularWM::createScene(void)
{
    // create your scene here :)
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.5f, 0.5f, 0.5f));
    Entity* ogre_head = mSceneMgr->createEntity("Head", "ogrehead.mesh");
    SceneNode* head_node = mSceneMgr->getRootSceneNode()->createChildSceneNode(
        "HeadNode");
    head_node->attachObject(ogre_head);

    Light* light = mSceneMgr->createLight("MainLight");
    light->setPosition(20, 80, 50);
}
