#pragma once

enum RaytracingTypes
{
    Primarybarycentric = 0,
    Reflectionbarycentric,
    Shadows,
    DiffuseHitShader,
    Reflection,
    NumTypes
};
class DescriptorHeapStack
{
public:
    DescriptorHeapStack(ID3D12Device& device, UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT NodeMask);
    ID3D12DescriptorHeap& GetDescriptorHeap();

    void AllocateDescriptor(_Out_ D3D12_CPU_DESCRIPTOR_HANDLE& cpuHandle, _Out_ UINT& descriptorHeapIndex);

    UINT AllocateBufferSrv(_In_ ID3D12Resource& resource);

    UINT AllocateBufferUav(_In_ ID3D12Resource& resource);

    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuHandle(UINT descriptorIndex);
private:
    ID3D12Device& m_device;
    CComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;
    UINT m_descriptorsAllocated = 0;
    UINT m_descriptorSize;
    D3D12_CPU_DESCRIPTOR_HANDLE m_descriptorHeapCpuBase;
};

__declspec(align(16)) struct HitShaderConstants
{
	Vector3 sunDirection;
	Vector3 sunLight;
	Vector3 ambientLight;
	float ShadowTexelSize[4];
	Matrix4 modelToShadow;
	UINT32 IsReflection;
	UINT32 UseShadowRays;
};

struct DynamicCB
{
	float4x4 cameraToWorld;
	float3   worldCameraPosition;
	uint     padding;
	float2   resolution;
};

struct MaterialRootConstant
{
	UINT MaterialID;
};

struct RayTraceMeshInfo
{
	UINT  m_indexOffsetBytes;
	UINT  m_uvAttributeOffsetBytes;
	UINT  m_normalAttributeOffsetBytes;
	UINT  m_tangentAttributeOffsetBytes;
	UINT  m_bitangentAttributeOffsetBytes;
	UINT  m_positionAttributeOffsetBytes;
	UINT  m_attributeStrideBytes;
	UINT  m_materialInstanceId;
};

struct RaytracingDispatchRayInputs
{
	RaytracingDispatchRayInputs();
	RaytracingDispatchRayInputs(
		ID3D12Device5& device,
		ID3D12StateObject* pPSO,
		void* pHitGroupShaderTable,
		UINT HitGroupStride,
		UINT HitGroupTableSize,
		LPCWSTR rayGenExportName,
		LPCWSTR missExportName);

	D3D12_DISPATCH_RAYS_DESC GetDispatchRayDesc(UINT DispatchWidth, UINT DispatchHeight);

	UINT m_HitGroupStride;
	CComPtr<ID3D12StateObject> m_pPSO;
	ByteAddressBuffer   m_RayGenShaderTable;
	ByteAddressBuffer   m_MissShaderTable;
	ByteAddressBuffer   m_HitShaderTable;
};

extern ColorBuffer g_SceneNormalBuffer;
extern CComPtr<ID3D12Device5> g_pRaytracingDevice;

const static UINT MaxRayRecursion = 2;
const static UINT c_NumCameraPositions = 6;

extern std::wstring m_assetsPath;
extern BModel* testModel;

extern std::unique_ptr<DescriptorHeapStack> g_pRaytracingDescriptorHeap;

extern ByteAddressBuffer          g_dynamicConstantBuffer;
extern ByteAddressBuffer          g_hitConstantBuffer;
extern StructuredBuffer    g_hitShaderMeshInfoBuffer;

extern D3D12_GPU_DESCRIPTOR_HANDLE g_GpuSceneMaterialSrvs[27];
extern D3D12_CPU_DESCRIPTOR_HANDLE g_SceneMeshInfo;
extern D3D12_CPU_DESCRIPTOR_HANDLE g_SceneIndices;

extern D3D12_GPU_DESCRIPTOR_HANDLE g_OutputUAV;
extern D3D12_GPU_DESCRIPTOR_HANDLE g_DepthAndNormalsTable;
extern D3D12_GPU_DESCRIPTOR_HANDLE g_SceneSrvs;

extern std::vector<CComPtr<ID3D12Resource>>   g_bvh_bottomLevelAccelerationStructures;
extern CComPtr<ID3D12Resource>   g_bvh_topLevelAccelerationStructure;
extern CComPtr<ID3D12RootSignature> g_GlobalRaytracingRootSignature;
extern CComPtr<ID3D12RootSignature> g_LocalRaytracingRootSignature;

extern DynamicCB           g_dynamicCb;

extern const UINT MaxRayRecursion;

extern const UINT c_NumCameraPositions;

extern RaytracingDispatchRayInputs g_RaytracingInputs[RaytracingTypes::NumTypes];
extern D3D12_CPU_DESCRIPTOR_HANDLE g_bvh_attributeSrvs[34];

extern ExpVar m_SunLightIntensity;
extern ExpVar m_AmbientIntensity;
extern NumVar m_SunOrientation;
extern NumVar m_SunInclination;
extern NumVar ShadowDimX;
extern NumVar ShadowDimY;
extern NumVar ShadowDimZ;

std::wstring GetAssetFullPath(LPCWSTR assetName);
inline bool IsDirectXRaytracingSupported(IDXGIAdapter1* adapter);
void SetPipelineStateStackSize(LPCWSTR raygen, LPCWSTR closestHit, LPCWSTR miss, UINT maxRecursion, ID3D12StateObject* pStateObject);
D3D12_STATE_SUBOBJECT CreateDxilLibrary(LPCWSTR entrypoint, const void* pShaderByteCode, SIZE_T bytecodeLength, D3D12_DXIL_LIBRARY_DESC& dxilLibDesc, D3D12_EXPORT_DESC& exportDesc);
void InitializeSceneInfo(const Model& model);
void InitializeViews(const Model& model);
void InitializeRaytracingStateObjects(const Model& model, UINT numMeshes);