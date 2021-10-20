#ifndef __PC_GFX_H__
#define __PC_GFX_H__

#define PLATFORM_WIN32 1
#define EXPLICITLY_LOAD_ENGINE_VK_DLL 1
#define EXPLICITLY_LOAD_ENGINE_GL_DLL 1
#define ENGINE_DLL 1
#define VULKAN_SUPPORTED 1
#define GL_SUPPORTED 1
#define D3D12_SUPPORTED 1

#include <DiligentCore/Common/interface/RefCntAutoPtr.hpp>
#include <DiligentCore/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <DiligentCore/Graphics/GraphicsEngine/interface/SwapChain.h>

inline Diligent::IRenderDevice*  m_pDevice;
inline Diligent::IDeviceContext* m_pImmediateContext;
inline Diligent::ISwapChain*     m_pSwapChain;
inline Diligent::IPipelineState* m_pPSO;
inline Diligent::RENDER_DEVICE_TYPE            m_DeviceType = Diligent::RENDER_DEVICE_TYPE_D3D11;
inline Diligent::IEngineFactory* m_pEngineFactory;

#endif
