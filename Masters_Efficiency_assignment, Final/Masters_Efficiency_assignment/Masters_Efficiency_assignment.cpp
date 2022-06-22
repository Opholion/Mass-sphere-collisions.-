// Masters_Efficiency_assignment.cpp: A program using the TL-Engine

#include <TL-Engine.h>	// TL-Engine include file and namespace
#include "SceneManager.h"
using namespace tle;
#ifdef THREE_DIMENSIONS
SceneManager<ThreeDimensions> sceneManager;
#else
SceneManager<TwoDimensions> sceneManager;
#endif

void main()
{

}
