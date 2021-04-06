#pragma once
#include "stdafx.h"

namespace BBufferManager
{
	extern ColorBuffer g_MainColorBuffer;

	void InitResource(uint32_t bufferWidth, uint32_t bufferHeight);
	void ResizeDisplayDependentBuffers(uint32_t NativeWidth, uint32_t NativeHeight);
	void DestroyRenderingBuffers();
};

