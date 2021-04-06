#include "BBufferManager.h"

namespace BBufferManager
{
    ColorBuffer g_MainColorBuffer;
    DXGI_FORMAT DefaultHdrColorFormat = DXGI_FORMAT_R11G11B10_FLOAT;
}

#define T2X_COLOR_FORMAT DXGI_FORMAT_R10G10B10A2_UNORM
#define HDR_MOTION_FORMAT DXGI_FORMAT_R16G16B16A16_FLOAT
#define DSV_FORMAT DXGI_FORMAT_D32_FLOAT

void BBufferManager::InitResource(uint32_t bufferWidth, uint32_t bufferHeight)
{
    GraphicsContext& InitContext = GraphicsContext::Begin();

    g_MainColorBuffer.Create(L"Scene Depth Buffer", bufferWidth, bufferHeight, 1, DefaultHdrColorFormat);

    InitContext.Finish();
}

void BBufferManager::ResizeDisplayDependentBuffers(uint32_t /*NativeWidth*/, uint32_t NativeHeight)
{
}

void BBufferManager::DestroyRenderingBuffers()
{
    g_MainColorBuffer.Destroy();
}
