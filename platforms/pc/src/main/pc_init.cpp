#include <iostream>
#include <cstdlib>
#include <malloc.h>

#include <pc_gfx.h>

#include <DiligentCore/Graphics/GraphicsEngine/interface/LoadEngineDll.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/EngineFactory.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#include <DiligentCore/Graphics/GraphicsEngineVulkan/interface/EngineFactoryVk.h>
#include <DiligentCore/Graphics/GraphicsEngineD3D12/interface/EngineFactoryD3D12.h>
#include <DiligentCore/Graphics/GraphicsEngineD3D11/interface/EngineFactoryD3D11.h>
#include <DiligentCore/Graphics/GraphicsEngineOpenGL/interface/EngineFactoryOpenGL.h>
#pragma GCC diagnostic pop

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#include <main.h>
#include <input.h>
#include <mem.h>

using namespace Diligent;

constexpr size_t mem_pool_size = 0x1000000;
constexpr int window_width = 800;
constexpr int window_height = 600;

SDL_Window* window;

void platformInit(void)
{
    uint8_t *allMem = static_cast<uint8_t*>(malloc(mem_pool_size));
    initMemAllocator(&allMem[0], &allMem[mem_pool_size]);

    // Initialize SDL systems
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        exit(1);
    }
    window = SDL_CreateWindow(APP_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if(window == NULL)
    {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        exit(1);
    }

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    if (!SDL_GetWindowWMInfo(window, &wmi)) {
        std::cerr << "SDL could not get window info! SDL_Error: " << SDL_GetError() << std::endl;
        exit(1);
    }
    
    HWND hWnd = wmi.info.win.window;

    SwapChainDesc SCDesc;
    
    {
        EngineVkCreateInfo EngineCI;
        auto GetEngineFactoryVk = LoadGraphicsEngineVk();
        auto engineFactory = GetEngineFactoryVk();
        engineFactory->CreateDeviceAndContextsVk(EngineCI, &m_pDevice, &m_pImmediateContext);
        Win32NativeWindow Window{hWnd};
        engineFactory->CreateSwapChainVk(m_pDevice, m_pImmediateContext, SCDesc, Window, &m_pSwapChain);
        m_pEngineFactory = engineFactory;
    }
}

void profileStartMainLoop()
{

}

void profileBeforeGfxMainLoop()
{

}

void profileEndMainLoop()
{
}
