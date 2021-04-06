//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently.

#pragma once
#include <d3d12.h>
#include <d3dcompiler.h>
#ifdef _DEBUG
#include <dxgidebug.h>
#endif
#include <atlcomcli.h>
#include <dxgi1_3.h>
#include <d3d12video.h>
#include <atlbase.h>
#include <string>
#include <iostream>

#include "SystemTime.h"
#include "CommandContext.h"
#include "SamplerManager.h"
#include "GpuBuffer.h"
#include "BufferManager.h"
#include "GraphicsCore.h"
#include "GameCore.h"
#include "GameInput.h"
#include "DXSampleHelper.h"
