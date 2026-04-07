#include "PlatformServices.h"
#include "StdFileIO.h"

#include "sdl2/Render.h"

static StdFileIO s_stdFileIO;

IPlatformFileIO& PlatformFileIO = s_stdFileIO;
IPlatformRenderer& PlatformRender = RenderManager;
