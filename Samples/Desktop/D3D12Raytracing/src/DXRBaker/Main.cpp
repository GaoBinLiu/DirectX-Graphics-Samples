#define NOMINMAX
#include "stdafx.h"
#ifndef HLSL
#include "HlslCompat.h"
#endif

#include "CameraController.h"
#include "Camera.h"
#include "Model.h"
#include "TextRenderer.h"
#include "ShaderResource.h"
#include "Scene.h"
#include "PostEffects.h"

#ifdef HLSL
struct RayPayload
{
	bool SkipShading;
	float RayHitT;
};

#endif
/*
#include "CompiledShaders/DepthViewerVS.h"
#include "CompiledShaders/DepthViewerPS.h"
#include "CompiledShaders/ModelViewerVS.h"
#include "CompiledShaders/ModelViewerPS.h"
#include "CompiledShaders/WaveTileCountPS.h"
#include "CompiledShaders/RayGenerationShaderLib.h"
#include "CompiledShaders/RayGenerationShaderSSRLib.h"
#include "CompiledShaders/HitShaderLib.h"
#include "CompiledShaders/MissShaderLib.h"
#include "CompiledShaders/DiffuseHitShaderLib.h"
#include "CompiledShaders/RayGenerationShadowsLib.h"
#include "CompiledShaders/MissShadowsLib.h"*/
#include <MotionBlur.h>
#include <FXAA.h>
#include <TemporalEffects.h>
#include <MotionBlur.h>
#include <SSAO.h>
#include "DXRBaker.h"
//#include "RaytracingHlslCompat.h"
//#include "ModelViewerRayTracing.h"

using namespace GameCore;
using namespace Math;
using namespace Graphics;


DescriptorHeapStack::DescriptorHeapStack(ID3D12Device& device, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT NodeMask) :
	m_device(device)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	desc.NodeMask = NodeMask;
	device.CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_pDescriptorHeap));

	m_descriptorSize = device.GetDescriptorHandleIncrementSize(type);
	m_descriptorHeapCpuBase = m_pDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

ID3D12DescriptorHeap& DescriptorHeapStack::GetDescriptorHeap() { return *m_pDescriptorHeap; }

void DescriptorHeapStack::AllocateDescriptor(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, _Out_ UINT& descriptorHeapIndex)
{
	descriptorHeapIndex = m_descriptorsAllocated;
	cpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_descriptorHeapCpuBase, descriptorHeapIndex, m_descriptorSize);
	m_descriptorsAllocated++;
}

UINT DescriptorHeapStack::AllocateBufferSrv(_In_ ID3D12Resource& resource)
{
	UINT descriptorHeapIndex;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	AllocateDescriptor(cpuHandle, descriptorHeapIndex);
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.NumElements = (UINT)(resource.GetDesc().Width / sizeof(UINT32));
	srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
	srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_device.CreateShaderResourceView(&resource, &srvDesc, cpuHandle);
	return descriptorHeapIndex;
}

UINT DescriptorHeapStack::AllocateBufferUav(_In_ ID3D12Resource& resource)
{
	UINT descriptorHeapIndex;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	AllocateDescriptor(cpuHandle, descriptorHeapIndex);
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = (UINT)(resource.GetDesc().Width / sizeof(UINT32));
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
	uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;

	m_device.CreateUnorderedAccessView(&resource, nullptr, &uavDesc, cpuHandle);
	return descriptorHeapIndex;
}

D3D12_GPU_DESCRIPTOR_HANDLE DescriptorHeapStack::GetGpuHandle(UINT descriptorIndex)
{
	return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_pDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), descriptorIndex, m_descriptorSize);
}

RaytracingDispatchRayInputs::RaytracingDispatchRayInputs() {}
RaytracingDispatchRayInputs::RaytracingDispatchRayInputs(
	ID3D12Device5& device,
	ID3D12StateObject* pPSO,
	void* pHitGroupShaderTable,
	UINT HitGroupStride,
	UINT HitGroupTableSize,
	LPCWSTR rayGenExportName,
	LPCWSTR missExportName) : m_pPSO(pPSO)
{
	const UINT shaderTableSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	ID3D12StateObjectProperties* stateObjectProperties = nullptr;
	ThrowIfFailed(pPSO->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));
	void* pRayGenShaderData = stateObjectProperties->GetShaderIdentifier(rayGenExportName);
	void* pMissShaderData = stateObjectProperties->GetShaderIdentifier(missExportName);

	m_HitGroupStride = HitGroupStride;

	// MiniEngine requires that all initial data be aligned to 16 bytes
	UINT alignment = 16;
	std::vector<BYTE> alignedShaderTableData(shaderTableSize + alignment - 1);
	BYTE* pAlignedShaderTableData = alignedShaderTableData.data() + ((UINT64)alignedShaderTableData.data() % alignment);
	memcpy(pAlignedShaderTableData, pRayGenShaderData, shaderTableSize);
	m_RayGenShaderTable.Create(L"Ray Gen Shader Table", 1, shaderTableSize, alignedShaderTableData.data());

	memcpy(pAlignedShaderTableData, pMissShaderData, shaderTableSize);
	m_MissShaderTable.Create(L"Miss Shader Table", 1, shaderTableSize, alignedShaderTableData.data());

	m_HitShaderTable.Create(L"Hit Shader Table", 1, HitGroupTableSize, pHitGroupShaderTable);
}

D3D12_DISPATCH_RAYS_DESC RaytracingDispatchRayInputs::GetDispatchRayDesc(UINT DispatchWidth, UINT DispatchHeight)
{
	D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = {};

	dispatchRaysDesc.RayGenerationShaderRecord.StartAddress = m_RayGenShaderTable.GetGpuVirtualAddress();
	dispatchRaysDesc.RayGenerationShaderRecord.SizeInBytes = m_RayGenShaderTable.GetBufferSize();
	dispatchRaysDesc.HitGroupTable.StartAddress = m_HitShaderTable.GetGpuVirtualAddress();
	dispatchRaysDesc.HitGroupTable.SizeInBytes = m_HitShaderTable.GetBufferSize();
	dispatchRaysDesc.HitGroupTable.StrideInBytes = m_HitGroupStride;
	dispatchRaysDesc.MissShaderTable.StartAddress = m_MissShaderTable.GetGpuVirtualAddress();
	dispatchRaysDesc.MissShaderTable.SizeInBytes = m_MissShaderTable.GetBufferSize();
	dispatchRaysDesc.MissShaderTable.StrideInBytes = dispatchRaysDesc.MissShaderTable.SizeInBytes; // Only one entry
	dispatchRaysDesc.Width = DispatchWidth;
	dispatchRaysDesc.Height = DispatchHeight;
	dispatchRaysDesc.Depth = 1;
	return dispatchRaysDesc;
}

extern ByteAddressBuffer   g_bvh_bottomLevelAccelerationStructure;
ColorBuffer g_SceneNormalBuffer;
CComPtr<ID3D12Device5> g_pRaytracingDevice;

std::wstring m_assetsPath;
BModel* testModel;

std::unique_ptr<DescriptorHeapStack> g_pRaytracingDescriptorHeap;

ByteAddressBuffer          g_dynamicConstantBuffer;
ByteAddressBuffer          g_hitConstantBuffer;
StructuredBuffer    g_hitShaderMeshInfoBuffer;

D3D12_GPU_DESCRIPTOR_HANDLE g_GpuSceneMaterialSrvs[27];
D3D12_CPU_DESCRIPTOR_HANDLE g_SceneMeshInfo;
D3D12_CPU_DESCRIPTOR_HANDLE g_SceneIndices;

D3D12_GPU_DESCRIPTOR_HANDLE g_OutputUAV;
D3D12_GPU_DESCRIPTOR_HANDLE g_DepthAndNormalsTable;
D3D12_GPU_DESCRIPTOR_HANDLE g_SceneSrvs;

std::vector<CComPtr<ID3D12Resource>>   g_bvh_bottomLevelAccelerationStructures;
CComPtr<ID3D12Resource>   g_bvh_topLevelAccelerationStructure;
CComPtr<ID3D12RootSignature> g_GlobalRaytracingRootSignature;
CComPtr<ID3D12RootSignature> g_LocalRaytracingRootSignature;

DynamicCB           g_dynamicCb;

RaytracingDispatchRayInputs g_RaytracingInputs[RaytracingTypes::NumTypes];
D3D12_CPU_DESCRIPTOR_HANDLE g_bvh_attributeSrvs[34];

ExpVar m_SunLightIntensity("Application/Lighting/Sun Light Intensity", 4.0f, 0.0f, 16.0f, 0.1f);
ExpVar m_AmbientIntensity("Application/Lighting/Ambient Intensity", 0.1f, -16.0f, 16.0f, 0.1f);
NumVar m_SunOrientation("Application/Lighting/Sun Orientation", -0.5f, -100.0f, 100.0f, 0.1f);
NumVar m_SunInclination("Application/Lighting/Sun Inclination", 0.75f, 0.0f, 1.0f, 0.01f);
NumVar ShadowDimX("Application/Lighting/Shadow Dim X", 5000, 1000, 10000, 100);
NumVar ShadowDimY("Application/Lighting/Shadow Dim Y", 3000, 1000, 10000, 100);
NumVar ShadowDimZ("Application/Lighting/Shadow Dim Z", 3000, 1000, 10000, 100);

std::wstring GetAssetFullPath(LPCWSTR assetName)
{
	return m_assetsPath + assetName;
}

// Returns bool whether the device supports DirectX Raytracing tier.
inline bool IsDirectXRaytracingSupported(IDXGIAdapter1* adapter)
{
	ComPtr<ID3D12Device> testDevice;
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};

	return SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&testDevice)))
		&& SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData)))
		&& featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

void SetPipelineStateStackSize(LPCWSTR raygen, LPCWSTR closestHit, LPCWSTR miss, UINT maxRecursion, ID3D12StateObject* pStateObject)
{
	ID3D12StateObjectProperties* stateObjectProperties = nullptr;
	ThrowIfFailed(pStateObject->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));
	UINT64 closestHitStackSize = stateObjectProperties->GetShaderStackSize(closestHit);
	UINT64 missStackSize = stateObjectProperties->GetShaderStackSize(miss);
	UINT64 raygenStackSize = stateObjectProperties->GetShaderStackSize(raygen);

	UINT64 totalStackSize = raygenStackSize + std::max(missStackSize, closestHitStackSize) * maxRecursion;
	stateObjectProperties->SetPipelineStackSize(totalStackSize);
}

D3D12_STATE_SUBOBJECT CreateDxilLibrary(LPCWSTR entrypoint, const void* pShaderByteCode, SIZE_T bytecodeLength, D3D12_DXIL_LIBRARY_DESC& dxilLibDesc, D3D12_EXPORT_DESC& exportDesc)
{
	exportDesc = { entrypoint, nullptr, D3D12_EXPORT_FLAG_NONE };
	D3D12_STATE_SUBOBJECT dxilLibSubObject = {};
	dxilLibDesc.DXILLibrary.pShaderBytecode = pShaderByteCode;
	dxilLibDesc.DXILLibrary.BytecodeLength = bytecodeLength;
	dxilLibDesc.NumExports = 1;
	dxilLibDesc.pExports = &exportDesc;
	dxilLibSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	dxilLibSubObject.pDesc = &dxilLibDesc;
	return dxilLibSubObject;
}
void InitializeSceneInfo(const Model& model)
{
	//
	// Mesh info
	//
	std::vector<RayTraceMeshInfo>   meshInfoData(model.m_Header.meshCount);
	for (UINT i = 0; i < model.m_Header.meshCount; ++i)
	{
		meshInfoData[i].m_indexOffsetBytes = model.m_pMesh[i].indexDataByteOffset;
		meshInfoData[i].m_uvAttributeOffsetBytes = model.m_pMesh[i].vertexDataByteOffset + model.m_pMesh[i].attrib[Model::attrib_texcoord0].offset;
		meshInfoData[i].m_normalAttributeOffsetBytes = model.m_pMesh[i].vertexDataByteOffset + model.m_pMesh[i].attrib[Model::attrib_normal].offset;
		meshInfoData[i].m_positionAttributeOffsetBytes = model.m_pMesh[i].vertexDataByteOffset + model.m_pMesh[i].attrib[Model::attrib_position].offset;
		meshInfoData[i].m_tangentAttributeOffsetBytes = model.m_pMesh[i].vertexDataByteOffset + model.m_pMesh[i].attrib[Model::attrib_tangent].offset;
		meshInfoData[i].m_bitangentAttributeOffsetBytes = model.m_pMesh[i].vertexDataByteOffset + model.m_pMesh[i].attrib[Model::attrib_bitangent].offset;
		meshInfoData[i].m_attributeStrideBytes = model.m_pMesh[i].vertexStride;
		meshInfoData[i].m_materialInstanceId = model.m_pMesh[i].materialIndex;
		//ASSERT(meshInfoData[i].m_materialInstanceId < 27);
	}

	g_hitShaderMeshInfoBuffer.Create(L"RayTraceMeshInfo",
		(UINT)meshInfoData.size(),
		sizeof(meshInfoData[0]),
		meshInfoData.data());

	g_SceneIndices = model.m_IndexBuffer.GetSRV();
	g_SceneMeshInfo = g_hitShaderMeshInfoBuffer.GetSRV();
}

void InitializeViews(const Model& model)
{
	D3D12_CPU_DESCRIPTOR_HANDLE uavHandle;
	UINT uavDescriptorIndex;
	g_pRaytracingDescriptorHeap->AllocateDescriptor(uavHandle, uavDescriptorIndex);
	Graphics::g_Device->CopyDescriptorsSimple(1, uavHandle, Graphics::g_SceneColorBuffer.GetUAV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_OutputUAV = g_pRaytracingDescriptorHeap->GetGpuHandle(uavDescriptorIndex);

	{
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
		UINT srvDescriptorIndex;
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, srvDescriptorIndex);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Graphics::g_SceneDepthBuffer.GetDepthSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		g_DepthAndNormalsTable = g_pRaytracingDescriptorHeap->GetGpuHandle(srvDescriptorIndex);

		/*UINT unused;
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SceneNormalBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);*/

	}

	{
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandle;
		UINT srvDescriptorIndex;
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, srvDescriptorIndex);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SceneMeshInfo, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		g_SceneSrvs = g_pRaytracingDescriptorHeap->GetGpuHandle(srvDescriptorIndex);

		UINT unused;
		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, g_SceneIndices, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		g_pRaytracingDescriptorHeap->AllocateBufferSrv(*const_cast<ID3D12Resource*>(model.m_VertexBuffer.GetResource()));

		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Graphics::g_ShadowBuffer.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
		Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, Graphics::g_SSAOFullScreen.GetSRV(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		for (UINT i = 0; i < model.m_Header.materialCount; i++)
		{
			UINT slot;
			g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, slot);
			Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, *model.GetSRVs(i), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			g_pRaytracingDescriptorHeap->AllocateDescriptor(srvHandle, unused);
			Graphics::g_Device->CopyDescriptorsSimple(1, srvHandle, model.GetSRVs(i)[3], D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			g_GpuSceneMaterialSrvs[i] = g_pRaytracingDescriptorHeap->GetGpuHandle(slot);
		}
	}
}

void InitializeRaytracingStateObjects(const Model& model, UINT numMeshes)
{
	ZeroMemory(&g_dynamicCb, sizeof(g_dynamicCb));

	D3D12_STATIC_SAMPLER_DESC staticSamplerDescs[2] = {};
	D3D12_STATIC_SAMPLER_DESC& defaultSampler = staticSamplerDescs[0];
	defaultSampler.Filter = D3D12_FILTER_ANISOTROPIC;
	defaultSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	defaultSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	defaultSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	defaultSampler.MipLODBias = 0.0f;
	defaultSampler.MaxAnisotropy = 16;
	defaultSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	defaultSampler.MinLOD = 0.0f;
	defaultSampler.MaxLOD = D3D12_FLOAT32_MAX;
	defaultSampler.MaxAnisotropy = 8;
	defaultSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	defaultSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	defaultSampler.ShaderRegister = 0;

	D3D12_STATIC_SAMPLER_DESC& shadowSampler = staticSamplerDescs[1];
	shadowSampler = staticSamplerDescs[0];
	shadowSampler.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	shadowSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	shadowSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	shadowSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	shadowSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	shadowSampler.ShaderRegister = 1;

	D3D12_DESCRIPTOR_RANGE1 sceneBuffersDescriptorRange = {};
	sceneBuffersDescriptorRange.BaseShaderRegister = 1;
	sceneBuffersDescriptorRange.NumDescriptors = 5;
	sceneBuffersDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	sceneBuffersDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	D3D12_DESCRIPTOR_RANGE1 srvDescriptorRange = {};
	srvDescriptorRange.BaseShaderRegister = 12;
	srvDescriptorRange.NumDescriptors = 2;
	srvDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	D3D12_DESCRIPTOR_RANGE1 uavDescriptorRange = {};
	uavDescriptorRange.BaseShaderRegister = 2;
	uavDescriptorRange.NumDescriptors = 10;
	uavDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	uavDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	CD3DX12_ROOT_PARAMETER1 globalRootSignatureParameters[8];
	globalRootSignatureParameters[0].InitAsDescriptorTable(1, &sceneBuffersDescriptorRange);
	globalRootSignatureParameters[1].InitAsConstantBufferView(0);
	globalRootSignatureParameters[2].InitAsConstantBufferView(1);
	globalRootSignatureParameters[3].InitAsDescriptorTable(1, &srvDescriptorRange);
	globalRootSignatureParameters[4].InitAsDescriptorTable(1, &uavDescriptorRange);
	globalRootSignatureParameters[5].InitAsUnorderedAccessView(0);
	globalRootSignatureParameters[6].InitAsUnorderedAccessView(1);
	globalRootSignatureParameters[7].InitAsShaderResourceView(0);
	auto globalRootSignatureDesc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(ARRAYSIZE(globalRootSignatureParameters), globalRootSignatureParameters, ARRAYSIZE(staticSamplerDescs), staticSamplerDescs);

	CComPtr<ID3DBlob> pGlobalRootSignatureBlob;
	CComPtr<ID3DBlob> pErrorBlob;
	if (FAILED(D3D12SerializeVersionedRootSignature(&globalRootSignatureDesc, &pGlobalRootSignatureBlob, &pErrorBlob)))
	{
		OutputDebugStringA((LPCSTR)pErrorBlob->GetBufferPointer());
	}
	g_pRaytracingDevice->CreateRootSignature(0, pGlobalRootSignatureBlob->GetBufferPointer(), pGlobalRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&g_GlobalRaytracingRootSignature));

	D3D12_DESCRIPTOR_RANGE1 localTextureDescriptorRange = {};
	localTextureDescriptorRange.BaseShaderRegister = 6;
	localTextureDescriptorRange.NumDescriptors = 2;
	localTextureDescriptorRange.RegisterSpace = 0;
	localTextureDescriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	localTextureDescriptorRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

	CD3DX12_ROOT_PARAMETER1 localRootSignatureParameters[2];
	UINT sizeOfRootConstantInDwords = (sizeof(MaterialRootConstant) - 1) / sizeof(DWORD) + 1;
	localRootSignatureParameters[0].InitAsDescriptorTable(1, &localTextureDescriptorRange);
	localRootSignatureParameters[1].InitAsConstants(sizeOfRootConstantInDwords, 3);
	auto localRootSignatureDesc = CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC(ARRAYSIZE(localRootSignatureParameters), localRootSignatureParameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE);

	CComPtr<ID3DBlob> pLocalRootSignatureBlob;
	D3D12SerializeVersionedRootSignature(&localRootSignatureDesc, &pLocalRootSignatureBlob, nullptr);
	g_pRaytracingDevice->CreateRootSignature(0, pLocalRootSignatureBlob->GetBufferPointer(), pLocalRootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&g_LocalRaytracingRootSignature));

	std::vector<D3D12_STATE_SUBOBJECT> subObjects;
	D3D12_STATE_SUBOBJECT nodeMaskSubObject;
	UINT nodeMask = 1;
	nodeMaskSubObject.pDesc = &nodeMask;
	nodeMaskSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_NODE_MASK;
	subObjects.push_back(nodeMaskSubObject);

	D3D12_STATE_SUBOBJECT rootSignatureSubObject;
	rootSignatureSubObject.pDesc = &g_GlobalRaytracingRootSignature.p;
	rootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subObjects.push_back(rootSignatureSubObject);

	D3D12_STATE_SUBOBJECT configurationSubObject;
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	pipelineConfig.MaxTraceRecursionDepth = MaxRayRecursion;
	configurationSubObject.pDesc = &pipelineConfig;
	configurationSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subObjects.push_back(configurationSubObject);

	std::vector<LPCWSTR> shadersToAssociate;

	// Ray Gen shader stuff
	// ----------------------------------------------------------------//
	LPCWSTR rayGenShaderExportName = L"RayGen";
	subObjects.push_back(D3D12_STATE_SUBOBJECT{});
	D3D12_STATE_SUBOBJECT& rayGenDxilLibSubobject = subObjects.back();
	D3D12_EXPORT_DESC rayGenExportDesc;
	D3D12_DXIL_LIBRARY_DESC rayGenDxilLibDesc = {};
	rayGenDxilLibSubobject = CreateDxilLibrary(rayGenShaderExportName, g_pRayGenerationShaderLib, sizeof(g_pRayGenerationShaderLib), rayGenDxilLibDesc, rayGenExportDesc);
	shadersToAssociate.push_back(rayGenShaderExportName);

	// Hit Group shader stuff
	// ----------------------------------------------------------------//
	LPCWSTR closestHitExportName = L"Hit";
	D3D12_EXPORT_DESC hitGroupExportDesc;
	D3D12_DXIL_LIBRARY_DESC hitGroupDxilLibDesc = {};
	D3D12_STATE_SUBOBJECT hitGroupLibSubobject = CreateDxilLibrary(closestHitExportName, g_phitShaderLib, sizeof(g_phitShaderLib), hitGroupDxilLibDesc, hitGroupExportDesc);

	subObjects.push_back(hitGroupLibSubobject);
	shadersToAssociate.push_back(closestHitExportName);

	LPCWSTR missExportName = L"Miss";
	D3D12_EXPORT_DESC missExportDesc;
	D3D12_DXIL_LIBRARY_DESC missDxilLibDesc = {};
	D3D12_STATE_SUBOBJECT missLibSubobject = CreateDxilLibrary(missExportName, g_pmissShaderLib, sizeof(g_pmissShaderLib), missDxilLibDesc, missExportDesc);

	subObjects.push_back(missLibSubobject);
	D3D12_STATE_SUBOBJECT& missDxilLibSubobject = subObjects.back();

	shadersToAssociate.push_back(missExportName);

	D3D12_STATE_SUBOBJECT shaderConfigStateObject;
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig;
	shaderConfig.MaxAttributeSizeInBytes = 8;
	shaderConfig.MaxPayloadSizeInBytes = 8;
	shaderConfigStateObject.pDesc = &shaderConfig;
	shaderConfigStateObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subObjects.push_back(shaderConfigStateObject);
	D3D12_STATE_SUBOBJECT& shaderConfigSubobject = subObjects.back();

	LPCWSTR hitGroupExportName = L"HitGroup";
	D3D12_HIT_GROUP_DESC hitGroupDesc = {};
	hitGroupDesc.ClosestHitShaderImport = closestHitExportName;
	hitGroupDesc.HitGroupExport = hitGroupExportName;
	D3D12_STATE_SUBOBJECT hitGroupSubobject = {};
	hitGroupSubobject.Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	hitGroupSubobject.pDesc = &hitGroupDesc;
	subObjects.push_back(hitGroupSubobject);

	D3D12_STATE_SUBOBJECT localRootSignatureSubObject;
	localRootSignatureSubObject.pDesc = &g_LocalRaytracingRootSignature.p;
	localRootSignatureSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
	subObjects.push_back(localRootSignatureSubObject);
	D3D12_STATE_SUBOBJECT& rootSignatureSubobject = subObjects.back();

	D3D12_STATE_OBJECT_DESC stateObject;
	stateObject.NumSubobjects = (UINT)subObjects.size();
	stateObject.pSubobjects = subObjects.data();
	stateObject.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	const UINT shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
#define ALIGN(alignment, num) ((((num) + alignment - 1) / alignment) * alignment)
	const UINT offsetToDescriptorHandle = ALIGN(sizeof(D3D12_GPU_DESCRIPTOR_HANDLE), shaderIdentifierSize);
	const UINT offsetToMaterialConstants = ALIGN(sizeof(UINT32), offsetToDescriptorHandle + sizeof(D3D12_GPU_DESCRIPTOR_HANDLE));
	const UINT shaderRecordSizeInBytes = ALIGN(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT, offsetToMaterialConstants + sizeof(MaterialRootConstant));

	std::vector<byte> pHitShaderTable(shaderRecordSizeInBytes * numMeshes);
	auto GetShaderTable = [=](const Model& model, ID3D12StateObject* pPSO, byte* pShaderTable)
	{
		ID3D12StateObjectProperties* stateObjectProperties = nullptr;
		ThrowIfFailed(pPSO->QueryInterface(IID_PPV_ARGS(&stateObjectProperties)));
		void* pHitGroupIdentifierData = stateObjectProperties->GetShaderIdentifier(hitGroupExportName);
		for (UINT i = 0; i < numMeshes; i++)
		{
			byte* pShaderRecord = i * shaderRecordSizeInBytes + pShaderTable;
			memcpy(pShaderRecord, pHitGroupIdentifierData, shaderIdentifierSize);

			UINT materialIndex = model.m_pMesh[i].materialIndex;
			memcpy(pShaderRecord + offsetToDescriptorHandle, &g_GpuSceneMaterialSrvs[materialIndex].ptr, sizeof(g_GpuSceneMaterialSrvs[materialIndex].ptr));

			MaterialRootConstant material;
			material.MaterialID = i;
			memcpy(pShaderRecord + offsetToMaterialConstants, &material, sizeof(material));
		}
	};

	{
		CComPtr<ID3D12StateObject> pbarycentricPSO;
		g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS(&pbarycentricPSO));
		GetShaderTable(model, pbarycentricPSO, pHitShaderTable.data());
		g_RaytracingInputs[Primarybarycentric] = RaytracingDispatchRayInputs(*g_pRaytracingDevice, pbarycentricPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), rayGenShaderExportName, missExportName);
	}

	{
		rayGenDxilLibSubobject = CreateDxilLibrary(rayGenShaderExportName, g_pRayGenerationShaderSSRLib, sizeof(g_pRayGenerationShaderSSRLib), rayGenDxilLibDesc, rayGenExportDesc);
		CComPtr<ID3D12StateObject> pReflectionbarycentricPSO;
		g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS(&pReflectionbarycentricPSO));
		GetShaderTable(model, pReflectionbarycentricPSO, pHitShaderTable.data());
		g_RaytracingInputs[Reflectionbarycentric] = RaytracingDispatchRayInputs(*g_pRaytracingDevice, pReflectionbarycentricPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), rayGenShaderExportName, missExportName);
	}

	{
		rayGenDxilLibSubobject = CreateDxilLibrary(rayGenShaderExportName, g_pRayGenerationShadowsLib, sizeof(g_pRayGenerationShadowsLib), rayGenDxilLibDesc, rayGenExportDesc);
		missDxilLibSubobject = CreateDxilLibrary(missExportName, g_pmissShadowsLib, sizeof(g_pmissShadowsLib), missDxilLibDesc, missExportDesc);

		CComPtr<ID3D12StateObject> pShadowsPSO;
		g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS(&pShadowsPSO));
		GetShaderTable(model, pShadowsPSO, pHitShaderTable.data());
		g_RaytracingInputs[Shadows] = RaytracingDispatchRayInputs(*g_pRaytracingDevice, pShadowsPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), rayGenShaderExportName, missExportName);
	}

	{
		rayGenDxilLibSubobject = CreateDxilLibrary(rayGenShaderExportName, g_pRayGenerationShaderLib, sizeof(g_pRayGenerationShaderLib), rayGenDxilLibDesc, rayGenExportDesc);
		hitGroupLibSubobject = CreateDxilLibrary(closestHitExportName, g_pDiffuseHitShaderLib, sizeof(g_pDiffuseHitShaderLib), hitGroupDxilLibDesc, hitGroupExportDesc);
		missDxilLibSubobject = CreateDxilLibrary(missExportName, g_pmissShaderLib, sizeof(g_pmissShaderLib), missDxilLibDesc, missExportDesc);

		CComPtr<ID3D12StateObject> pDiffusePSO;
		g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS(&pDiffusePSO));
		GetShaderTable(model, pDiffusePSO, pHitShaderTable.data());
		g_RaytracingInputs[DiffuseHitShader] = RaytracingDispatchRayInputs(*g_pRaytracingDevice, pDiffusePSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), rayGenShaderExportName, missExportName);
	}

	{
		rayGenDxilLibSubobject = CreateDxilLibrary(rayGenShaderExportName, g_pRayGenerationShaderSSRLib, sizeof(g_pRayGenerationShaderSSRLib), rayGenDxilLibDesc, rayGenExportDesc);
		hitGroupLibSubobject = CreateDxilLibrary(closestHitExportName, g_pDiffuseHitShaderLib, sizeof(g_pDiffuseHitShaderLib), hitGroupDxilLibDesc, hitGroupExportDesc);
		missDxilLibSubobject = CreateDxilLibrary(missExportName, g_pmissShaderLib, sizeof(g_pmissShaderLib), missDxilLibDesc, missExportDesc);

		CComPtr<ID3D12StateObject> pReflectionPSO;
		g_pRaytracingDevice->CreateStateObject(&stateObject, IID_PPV_ARGS(&pReflectionPSO));
		GetShaderTable(model, pReflectionPSO, pHitShaderTable.data());
		g_RaytracingInputs[Reflection] = RaytracingDispatchRayInputs(*g_pRaytracingDevice, pReflectionPSO, pHitShaderTable.data(), shaderRecordSizeInBytes, (UINT)pHitShaderTable.size(), rayGenShaderExportName, missExportName);
	}

	for (auto& raytracingPipelineState : g_RaytracingInputs)
	{
		WCHAR hitGroupExportNameClosestHitType[64];
		swprintf_s(hitGroupExportNameClosestHitType, L"%s::closesthit", hitGroupExportName);
		SetPipelineStateStackSize(rayGenShaderExportName, hitGroupExportNameClosestHitType, missExportName, MaxRayRecursion, raytracingPipelineState.m_pPSO);
	}
}


int wmain(int argc, wchar_t** argv)
{
	WCHAR assetsPath[512];
	GetAssetsPath(assetsPath, _countof(assetsPath));
	m_assetsPath = assetsPath;
#if _DEBUG
	CComPtr<ID3D12Debug> debugInterface;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
	{
		debugInterface->EnableDebugLayer();
	}
#endif

	CComPtr<ID3D12Device> pDevice;
	CComPtr<IDXGIAdapter1> pAdapter;
	CComPtr<IDXGIFactory2> pFactory;
	CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));
	bool validDeviceFound = false;
	for (uint32_t Idx = 0; !validDeviceFound && DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(Idx, &pAdapter); ++Idx)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		if (IsDirectXRaytracingSupported(pAdapter))
		{
			validDeviceFound = true;
		}
		pAdapter = nullptr;
	}

	Graphics::s_EnableVSync.Decrement();
	Graphics::TargetResolution = Graphics::k720p;
	Graphics::g_DisplayWidth = 1280;
	Graphics::g_DisplayHeight = 720;

	MotionBlur::Enable = false;//true;
	TemporalEffects::EnableTAA = false;//true;
	FXAA::Enable = false;
	PostEffects::EnableHDR = false;//true;
	PostEffects::EnableAdaptation = false;//true;
	PostEffects::BloomEnable = false;
	SSAO::Enable = false;
	DXRBaker app;
	GameCore::RunApplication(app, L"DXRBaker");
	return 0;
}
