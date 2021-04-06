#include "DXRBaker.h"

DXRBaker::DXRBaker(void)
{
}

DXRBaker::~DXRBaker()
{
    if (m_MainShader != nullptr)
        delete m_MainShader;
    m_MainShader = nullptr;
    if (m_WireframeShader != nullptr)
        delete m_WireframeShader;
    m_WireframeShader = nullptr;

}

void DXRBaker::Test()
{
}
void DXRBaker::ReloadModel(int id)
{
    const std::string ASSET_DIRECTORY = "../../../../../../../../MiniEngine/ModelViewer/Models/";
    //bool bModelLoadSuccess = m_Model.Load(ASSET_DIRECTORY "sponza.h3d");
    m_Model.Clear();
    std::string name;
    int type = 0;
    switch (id)
    {
    case 2:
        name = "tianyidao.bscene";
        break;
    case 3:
        name = "maliao.bscene";
        break;
    case 4:
        name = "sponza.h3d";
        type = 1;
        break;
    case 1:
    default:
        name = "unity.bscene";
        break;
    }
    bool bModelLoadSuccess = false;
    if(type == 1)
        bModelLoadSuccess = m_Model.Load((ASSET_DIRECTORY + name).c_str());
    else
        bModelLoadSuccess = m_Model.LoadUnity((ASSET_DIRECTORY + name).c_str());

    const std::string info = "Failed to load model" + ASSET_DIRECTORY + name;
    ASSERT(bModelLoadSuccess, info.c_str());
    ASSERT(m_Model.m_Header.meshCount > 0, "Model contains no meshes");
}
void DXRBaker::Startup(void)
{
    Test();

    ThrowIfFailed(Graphics::g_Device->QueryInterface(IID_PPV_ARGS(&g_pRaytracingDevice)), L"Couldn't get DirectX Raytracing interface for the device.\n");
    g_pRaytracingDescriptorHeap = std::unique_ptr<DescriptorHeapStack>(
        new DescriptorHeapStack(*Graphics::g_Device, 200, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 0));

    SamplerDesc DefaultSamplerDesc;
    DefaultSamplerDesc.MaxAnisotropy = 8;

    m_RootSig.Reset(6, 2);
    m_RootSig.InitStaticSampler(0, DefaultSamplerDesc, D3D12_SHADER_VISIBILITY_PIXEL);
    m_RootSig.InitStaticSampler(1, Graphics::SamplerShadowDesc, D3D12_SHADER_VISIBILITY_PIXEL);
    m_RootSig[0].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
    m_RootSig[1].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);
    m_RootSig[2].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, 6, D3D12_SHADER_VISIBILITY_PIXEL);
    m_RootSig[3].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 64, 6, D3D12_SHADER_VISIBILITY_PIXEL);
    m_RootSig[4].InitAsConstants(1, 2, D3D12_SHADER_VISIBILITY_VERTEX);
    m_RootSig[5].InitAsConstants(1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
    m_RootSig.Finalize(L"D3D12RaytracingMiniEngineSample", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    D3D12_INPUT_ELEMENT_DESC vertElem[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    DXGI_FORMAT ColorFormat = Graphics::g_SceneColorBuffer.GetFormat();
    DXGI_FORMAT DepthFormat = g_SceneDepthBuffer.GetFormat();
    ShaderResource::Init(m_assetsPath.c_str());
    m_MainShader = new ShaderResource(GetAssetFullPath(L"standard.hlsl").c_str());
    m_WireframeShader = new ShaderResource(GetAssetFullPath(L"wireframe.hlsl").c_str(), "WireframeVS");
    m_ArrowShader = new ShaderResource(GetAssetFullPath(L"wireframe.hlsl").c_str(), "ArrowVS");

    GraphicsPSO& m_MainPSO = m_MainShader->GetPSO();
    m_MainPSO.SetRootSignature(m_RootSig);
    m_MainPSO.SetRasterizerState(Graphics::RasterizerDefault);
    m_MainPSO.SetBlendState(Graphics::BlendDisable);
    m_MainPSO.SetDepthStencilState(Graphics::DepthStateReadWrite);
    m_MainPSO.SetInputLayout(_countof(vertElem), vertElem);
    m_MainPSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    DXGI_FORMAT formats[]{ ColorFormat };
    m_MainPSO.SetRenderTargetFormats(_countof(formats), formats, DepthFormat);
    m_MainPSO.SetVertexShader(m_MainShader->GetVS());
    m_MainPSO.SetPixelShader(m_MainShader->GetPS());
    m_MainPSO.Finalize();

    D3D12_INPUT_ELEMENT_DESC vertElem1[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    GraphicsPSO& m_WireFramePSO = m_WireframeShader->GetPSO();
    m_WireFramePSO = m_MainPSO;
    m_WireFramePSO.SetBlendState(BlendTraditional);
    m_WireFramePSO.SetInputLayout(_countof(vertElem1), vertElem1);
    m_WireFramePSO.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
    m_WireFramePSO.SetVertexShader(m_WireframeShader->GetVS());
    m_WireFramePSO.SetPixelShader(m_WireframeShader->GetPS());
    m_WireFramePSO.Finalize();

    GraphicsPSO& m_ArrowPSO = m_ArrowShader->GetPSO();
    m_ArrowPSO = m_WireFramePSO;
    m_ArrowPSO.SetVertexShader(m_ArrowShader->GetVS());
    m_ArrowPSO.SetPixelShader(m_ArrowShader->GetPS());
    m_ArrowPSO.Finalize();

    m_EditorModel.Load();

    //#define ASSET_DIRECTORY "../../../../../MiniEngine/ModelViewer/"
#define ASSET_DIRECTORY "../../../../../../../../MiniEngine/ModelViewer/"
    TextureManager::Initialize(ASSET_DIRECTORY L"Textures/");
    //bool bModelLoadSuccess = m_Model.Load(ASSET_DIRECTORY "Models/sponza.h3d");
    bool bModelLoadSuccess = m_Model.LoadUnity(ASSET_DIRECTORY "Models/unity.bscene");

    ASSERT(bModelLoadSuccess, "Failed to load model" ASSET_DIRECTORY "Models/sponza.h3d");
    ASSERT(m_Model.m_Header.meshCount > 0, "Model contains no meshes");
    // The caller of this function can override which materials are considered cutouts
    m_pMaterialIsCutout.resize(m_Model.m_Header.materialCount);
    m_pMaterialIsReflective.resize(m_Model.m_Header.materialCount);
    for (uint32_t i = 0; i < m_Model.m_Header.materialCount; ++i)
    {
        const Model::Material& mat = m_Model.m_pMaterial[i];
        if (std::string(mat.texDiffusePath).find("thorn") != std::string::npos ||
            std::string(mat.texDiffusePath).find("plant") != std::string::npos ||
            std::string(mat.texDiffusePath).find("chain") != std::string::npos)
        {
            m_pMaterialIsCutout[i] = true;
        }
        else
        {
            m_pMaterialIsCutout[i] = false;
        }

        if (std::string(mat.texDiffusePath).find("floor") != std::string::npos)
        {
            m_pMaterialIsReflective[i] = true;
        }
        else
        {
            m_pMaterialIsReflective[i] = false;
        }
    }

    g_hitConstantBuffer.Create(L"Hit Constant Buffer", 1, sizeof(HitShaderConstants));
    g_dynamicConstantBuffer.Create(L"Dynamic Constant Buffer", 1, sizeof(DynamicCB));

    InitializeSceneInfo(m_Model);
    InitializeViews(m_Model);
    UINT numMeshes = m_Model.m_Header.meshCount;

    const UINT numBottomLevels = 1;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo;
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelAccelerationStructureDesc = {};
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& topLevelInputs = topLevelAccelerationStructureDesc.Inputs;
    topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    topLevelInputs.NumDescs = numBottomLevels;
    topLevelInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    topLevelInputs.pGeometryDescs = nullptr;
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    g_pRaytracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);

    const D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS buildFlag = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    std::vector<D3D12_RAYTRACING_GEOMETRY_DESC> geometryDescs(m_Model.m_Header.meshCount);
    UINT64 scratchBufferSizeNeeded = topLevelPrebuildInfo.ScratchDataSizeInBytes;
    for (UINT i = 0; i < numMeshes; i++)
    {
        auto& mesh = m_Model.m_pMesh[i];

        D3D12_RAYTRACING_GEOMETRY_DESC& desc = geometryDescs[i];
        desc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
        desc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

        D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC& trianglesDesc = desc.Triangles;
        trianglesDesc.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
        trianglesDesc.VertexCount = mesh.vertexCount;
        trianglesDesc.VertexBuffer.StartAddress = m_Model.m_VertexBuffer.GetGpuVirtualAddress() + (mesh.vertexDataByteOffset + mesh.attrib[Model::attrib_position].offset);
        trianglesDesc.IndexBuffer = m_Model.m_IndexBuffer.GetGpuVirtualAddress() + mesh.indexDataByteOffset;
        trianglesDesc.VertexBuffer.StrideInBytes = mesh.vertexStride;
        trianglesDesc.IndexCount = mesh.indexCount;
        trianglesDesc.IndexFormat = m_Model.m_IndexBuffer.GetElementSize() == 4 ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        trianglesDesc.Transform3x4 = 0;
    }

    std::vector<UINT64> bottomLevelAccelerationStructureSize(numBottomLevels);
    std::vector<D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC> bottomLevelAccelerationStructureDescs(numBottomLevels);
    for (UINT i = 0; i < numBottomLevels; i++)
    {
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC& bottomLevelAccelerationStructureDesc = bottomLevelAccelerationStructureDescs[i];
        D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS& bottomLevelInputs = bottomLevelAccelerationStructureDesc.Inputs;
        bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        bottomLevelInputs.NumDescs = numMeshes;
        bottomLevelInputs.pGeometryDescs = &geometryDescs[i];
        bottomLevelInputs.Flags = buildFlag;
        bottomLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

        D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelprebuildInfo;
        g_pRaytracingDevice->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelprebuildInfo);

        bottomLevelAccelerationStructureSize[i] = bottomLevelprebuildInfo.ResultDataMaxSizeInBytes;
        scratchBufferSizeNeeded = std::max(bottomLevelprebuildInfo.ScratchDataSizeInBytes, scratchBufferSizeNeeded);
    }

    ByteAddressBuffer scratchBuffer;
    scratchBuffer.Create(L"Acceleration Structure Scratch Buffer", (UINT)scratchBufferSizeNeeded, 1);

    D3D12_HEAP_PROPERTIES defaultHeapDesc = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto topLevelDesc = CD3DX12_RESOURCE_DESC::Buffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    g_Device->CreateCommittedResource(
        &defaultHeapDesc,
        D3D12_HEAP_FLAG_NONE,
        &topLevelDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_PPV_ARGS(&g_bvh_topLevelAccelerationStructure));

    topLevelAccelerationStructureDesc.DestAccelerationStructureData = g_bvh_topLevelAccelerationStructure->GetGPUVirtualAddress();
    topLevelAccelerationStructureDesc.ScratchAccelerationStructureData = scratchBuffer.GetGpuVirtualAddress();

    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> instanceDescs(numBottomLevels);
    g_bvh_bottomLevelAccelerationStructures.resize(numBottomLevels);
    for (UINT i = 0; i < bottomLevelAccelerationStructureDescs.size(); i++)
    {
        auto& bottomLevelStructure = g_bvh_bottomLevelAccelerationStructures[i];

        auto bottomLevelDesc = CD3DX12_RESOURCE_DESC::Buffer(bottomLevelAccelerationStructureSize[i], D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
        g_Device->CreateCommittedResource(
            &defaultHeapDesc,
            D3D12_HEAP_FLAG_NONE,
            &bottomLevelDesc,
            D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
            nullptr,
            IID_PPV_ARGS(&bottomLevelStructure));

        bottomLevelAccelerationStructureDescs[i].DestAccelerationStructureData = bottomLevelStructure->GetGPUVirtualAddress();
        bottomLevelAccelerationStructureDescs[i].ScratchAccelerationStructureData = scratchBuffer.GetGpuVirtualAddress();

        D3D12_RAYTRACING_INSTANCE_DESC& instanceDesc = instanceDescs[i];
        UINT descriptorIndex = g_pRaytracingDescriptorHeap->AllocateBufferUav(*bottomLevelStructure);

        // Identity matrix
        ZeroMemory(instanceDesc.Transform, sizeof(instanceDesc.Transform));
        instanceDesc.Transform[0][0] = 1.0f;
        instanceDesc.Transform[1][1] = 1.0f;
        instanceDesc.Transform[2][2] = 1.0f;

        instanceDesc.AccelerationStructure = g_bvh_bottomLevelAccelerationStructures[i]->GetGPUVirtualAddress();
        instanceDesc.Flags = 0;
        instanceDesc.InstanceID = 0;
        instanceDesc.InstanceMask = 1;
        instanceDesc.InstanceContributionToHitGroupIndex = i;
    }

    ByteAddressBuffer instanceDataBuffer;
    instanceDataBuffer.Create(L"Instance Data Buffer", numBottomLevels, sizeof(D3D12_RAYTRACING_INSTANCE_DESC), instanceDescs.data());

    topLevelInputs.InstanceDescs = instanceDataBuffer.GetGpuVirtualAddress();
    topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;

    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Create Acceleration Structure");
    ID3D12GraphicsCommandList* pCommandList = gfxContext.GetCommandList();

    CComPtr<ID3D12GraphicsCommandList4> pRaytracingCommandList;
    pCommandList->QueryInterface(IID_PPV_ARGS(&pRaytracingCommandList));

    ID3D12DescriptorHeap* descriptorHeaps[] = { &g_pRaytracingDescriptorHeap->GetDescriptorHeap() };
    pRaytracingCommandList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

    auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    for (UINT i = 0; i < bottomLevelAccelerationStructureDescs.size(); i++)
    {
        pRaytracingCommandList->BuildRaytracingAccelerationStructure(&bottomLevelAccelerationStructureDescs[i], 0, nullptr);
    }
    pCommandList->ResourceBarrier(1, &uavBarrier);

    pRaytracingCommandList->BuildRaytracingAccelerationStructure(&topLevelAccelerationStructureDesc, 0, nullptr);

    gfxContext.Finish(true);

    InitializeRaytracingStateObjects(m_Model, numMeshes);

    float modelRadius = Length(m_Model.m_Header.boundingBox.max - m_Model.m_Header.boundingBox.min) * .5f;
    const Vector3 eye = (m_Model.m_Header.boundingBox.min + m_Model.m_Header.boundingBox.max) * .5f + Vector3(modelRadius * .5f, 0.0f, 0.0f);
    m_Camera.SetEyeAtUp(eye, Vector3(kZero), Vector3(kYUnitVector));

    m_CameraPosArrayCurrentPosition = 0;

    // Original point
    m_CameraPosArray[0].position = Vector3(0, 1, -3.0f);
    m_CameraPosArray[0].heading = 0.0f;
    m_CameraPosArray[0].pitch = -0.5f;

    // View of columns
    m_CameraPosArray[1].position = Vector3(299.0f, 208.0f, -202.0f);
    m_CameraPosArray[1].heading = -3.1111f;
    m_CameraPosArray[1].pitch = 0.5953f;

    // Bottom-up view from the floor
    m_CameraPosArray[2].position = Vector3(-1237.61f, 80.60f, -26.02f);
    m_CameraPosArray[2].heading = -1.5707f;
    m_CameraPosArray[2].pitch = 0.268f;

    // Top-down view from the second floor
    m_CameraPosArray[3].position = Vector3(-977.90f, 595.05f, -194.97f);
    m_CameraPosArray[3].heading = -2.077f;
    m_CameraPosArray[3].pitch = -0.450f;

    // View of corridors on the second floor
    m_CameraPosArray[4].position = Vector3(-1463.0f, 600.0f, 394.52f);
    m_CameraPosArray[4].heading = -1.236f;
    m_CameraPosArray[4].pitch = 0.0f;

    // Lion's head
    m_CameraPosArray[5].position = Vector3(-1100.0f, 170.0f, -30.0f);
    m_CameraPosArray[5].heading = 1.5707f;
    m_CameraPosArray[5].pitch = 0.0f;

    m_Camera.SetZRange(1.0f, 10000.0f);

    m_CameraController.reset(new CameraController(m_Camera, Vector3(kYUnitVector)));
}

void DXRBaker::Cleanup(void)
{
    m_Model.Clear();
}

void DXRBaker::SetCameraToPredefinedPosition(int cameraPosition)
{
    if (cameraPosition < 0 || cameraPosition >= c_NumCameraPositions)
        return;

    m_CameraController->SetCurrentHeading(m_CameraPosArray[m_CameraPosArrayCurrentPosition].heading);
    m_CameraController->SetCurrentPitch(m_CameraPosArray[m_CameraPosArrayCurrentPosition].pitch);

    Matrix3 neworientation = Matrix3(m_CameraController->GetWorldEast(), m_CameraController->GetWorldUp(), -m_CameraController->GetWorldNorth())
        * Matrix3::MakeYRotation(m_CameraController->GetCurrentHeading())
        * Matrix3::MakeXRotation(m_CameraController->GetCurrentPitch());
    m_Camera.SetTransform(AffineTransform(neworientation, m_CameraPosArray[m_CameraPosArrayCurrentPosition].position));
    m_Camera.Update();
}

void DXRBaker::RefreshResource()
{
    m_MainShader->Reload();
    m_MainShader->GetPSO().Finalize();

    m_WireframeShader->Reload();
    m_WireframeShader->GetPSO().Finalize();
}

void DXRBaker::Update(float deltaT)
{
    ScopedTimer _prof(L"Update State");

    static bool freezeCamera = false;

    if (GameInput::IsFirstPressed(GameInput::kKey_f))
    {
        freezeCamera = !freezeCamera;
    }

    if (GameInput::IsFirstPressed(GameInput::kKey_left))
    {
        m_CameraPosArrayCurrentPosition = (m_CameraPosArrayCurrentPosition + c_NumCameraPositions - 1) % c_NumCameraPositions;
        SetCameraToPredefinedPosition(m_CameraPosArrayCurrentPosition);
    }
    else if (GameInput::IsFirstPressed(GameInput::kKey_right))
    {
        m_CameraPosArrayCurrentPosition = (m_CameraPosArrayCurrentPosition + 1) % c_NumCameraPositions;
        SetCameraToPredefinedPosition(m_CameraPosArrayCurrentPosition);
    }

    if (GameInput::IsFirstPressed(GameInput::kKey_minus))
    {
        hiddenMeshID--;
        hiddenMeshID = std::max(0, hiddenMeshID);
    }
    else if (GameInput::IsFirstPressed(GameInput::kKey_equals))
    {
        hiddenMeshID++;
    }

    if (GameInput::IsFirstPressed(GameInput::kKey_u))
    {
        RefreshResource();
    }
    if (GameInput::IsFirstPressed(GameInput::kKey_1))
    {
        ReloadModel(1);
    }
    if (GameInput::IsFirstPressed(GameInput::kKey_2))
    {
        ReloadModel(2);
    }
    if (GameInput::IsFirstPressed(GameInput::kKey_3))
    {
        ReloadModel(3);
    }
    if (GameInput::IsFirstPressed(GameInput::kKey_4))
    {
        ReloadModel(4);
    }

    if (!freezeCamera)
    {
        m_CameraController->Update(deltaT);
    }

    m_ViewProjMatrix = m_Camera.GetViewProjMatrix();

    float costheta = cosf(m_SunOrientation);
    float sintheta = sinf(m_SunOrientation);
    float cosphi = cosf(m_SunInclination * 3.14159f * 0.5f);
    float sinphi = sinf(m_SunInclination * 3.14159f * 0.5f);
    m_SunDirection = Normalize(Vector3(costheta * cosphi, sinphi, sintheta * cosphi));

    m_MainViewport.Width = (float)g_SceneColorBuffer.GetWidth();
    m_MainViewport.Height = (float)g_SceneColorBuffer.GetHeight();
    m_MainViewport.MinDepth = 0.0f;
    m_MainViewport.MaxDepth = 1.0f;

    m_MainScissor.left = 0;
    m_MainScissor.top = 0;
    m_MainScissor.right = (LONG)g_SceneColorBuffer.GetWidth();
    m_MainScissor.bottom = (LONG)g_SceneColorBuffer.GetHeight();
}

void DXRBaker::RenderScene(void)
{
    GraphicsContext& gfxContext = GraphicsContext::Begin(L"Scene Render");
    __declspec(align(16)) struct
    {
        Vector3 sunDirection;
        Vector3 sunLight;
        Vector3 ambientLight;
        float ShadowTexelSize[4];

        float InvTileDim[4];
        uint32_t TileCount[4];
        uint32_t FirstLightIndex[4];
        uint32_t FrameIndexMod2;
    } psConstants;

    psConstants.sunDirection = m_SunDirection;
    psConstants.sunLight = Vector3(1.0f, 1.0f, 1.0f) * m_SunLightIntensity;
    psConstants.ambientLight = Vector3(1.0f, 1.0f, 1.0f) * m_AmbientIntensity;
    psConstants.ShadowTexelSize[0] = 1.0f / g_ShadowBuffer.GetWidth();

    // Set the default state for command lists
    auto pfnSetupGraphicsState = [&](void)
    {
        gfxContext.SetRootSignature(m_RootSig);
        gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        gfxContext.SetIndexBuffer(m_Model.m_IndexBuffer.IndexBufferView());
        gfxContext.SetVertexBuffer(0, m_Model.m_VertexBuffer.VertexBufferView());
        gfxContext.SetVertexBuffer(1, m_Model.m_VertexExBuffer.VertexBufferView());
    };

    pfnSetupGraphicsState();


    {
        ScopedTimer _prof(L"Z PrePass", gfxContext);

        gfxContext.SetDynamicConstantBufferView(1, sizeof(psConstants), &psConstants);

        {
            ScopedTimer _prof(L"Opaque", gfxContext);
            {
                gfxContext.TransitionResource(g_SceneDepthBuffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
                gfxContext.TransitionResource(g_SceneColorBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, true);
                gfxContext.ClearDepth(g_SceneDepthBuffer);
                gfxContext.ClearColor(g_SceneColorBuffer);

                gfxContext.SetPipelineState(m_MainShader->GetPSO());
                gfxContext.SetRenderTarget(g_SceneColorBuffer.GetRTV(), g_SceneDepthBuffer.GetDSV());
                //gfxContext.SetDepthStencilTarget(g_SceneDepthBuffer.GetDSV());

                gfxContext.SetViewportAndScissor(m_MainViewport, m_MainScissor);
            }

            RenderNodes(gfxContext, m_ViewProjMatrix, kOpaque);
        }
    }
    //Raytrace(gfxContext);
    RenderEditorItems(gfxContext, m_ViewProjMatrix);
    gfxContext.Finish();
}
void DXRBaker::RenderEditorItems(GraphicsContext& gfxContext, const Matrix4& ViewProjMat)
{
    gfxContext.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
    gfxContext.SetVertexBuffer(0, m_EditorModel.m_VertexBuffer.VertexBufferView());
    gfxContext.SetIndexBuffer(m_EditorModel.m_IndexBuffer.IndexBufferView());

    struct VSConstants
    {
        Matrix4 modelToProjection;
        Matrix4 modelToShadow;
        Matrix4 modelToWorld;
        Matrix4 worldToModel;
        XMFLOAT3 viewerPos;
    } vsConstants;
    XMStoreFloat3(&vsConstants.viewerPos, m_Camera.GetPosition());

    //draw grid
    gfxContext.SetPipelineState(m_WireframeShader->GetPSO());
    Math::Matrix4 objectToWorld = AffineTransform(Vector3(0, 0, 0));
    vsConstants.modelToWorld = objectToWorld;
    vsConstants.worldToModel = Matrix4(::XMMatrixInverse(nullptr, objectToWorld));
    vsConstants.modelToProjection = ViewProjMat * objectToWorld;
    gfxContext.SetDynamicConstantBufferView(0, sizeof(vsConstants), &vsConstants);

    const Model::Mesh& mesh = m_EditorModel.m_pMesh[0];
    uint32_t count = mesh.vertexCount;
    uint32_t start = mesh.vertexDataByteOffset / mesh.vertexStride;
    gfxContext.Draw(count, start);

    //draw arrow
    gfxContext.SetPipelineState(m_ArrowShader->GetPSO());
    float fov = m_Camera.GetFOV();
    float asp = m_Camera.GetAspectRatio();
    float X = 1.0f / std::tanf(fov * 0.5f);
    float Y = X * asp;
    Vector3 offset = m_Camera.GetForwardVec() * 4;// -m_Camera.GetRightVec() * X + m_Camera.GetUpVec() * Y;
    objectToWorld = AffineTransform(m_Camera.GetPosition() + offset);
    vsConstants.modelToWorld = objectToWorld;
    vsConstants.worldToModel = Matrix4(::XMMatrixInverse(nullptr, objectToWorld));
    vsConstants.modelToProjection = ViewProjMat * objectToWorld;
    gfxContext.SetDynamicConstantBufferView(0, sizeof(vsConstants), &vsConstants);

    const Model::Mesh& mesh1 = m_EditorModel.m_pMesh[1];
    count = mesh1.vertexCount;
    start = mesh1.vertexDataByteOffset / mesh1.vertexStride;
    gfxContext.Draw(count, start);
}

void DXRBaker::RenderUI(GraphicsContext&)
{
}
void DXRBaker::RenderNodes(GraphicsContext& gfxContext, const Matrix4& ViewProjMat, eObjectFilter Filter)
{
    struct VSConstants
    {
        Matrix4 modelToProjection;
        Matrix4 modelToShadow;
        Matrix4 modelToWorld;
        Matrix4 worldToModel;
        XMFLOAT3 viewerPos;
    } vsConstants;
    XMStoreFloat3(&vsConstants.viewerPos, m_Camera.GetPosition());


    uint32_t materialIdx = 0xFFFFFFFFul;

    uint32_t VertexStride = m_Model.m_VertexStride;

    for (uint32_t nodeIndex = 0; nodeIndex < m_Model.m_BHeader.nodeCount; nodeIndex++)
    {
        if (nodeIndex == hiddenMeshID)
            continue;
        const BModel::Node& node = m_Model.m_pNode[nodeIndex];
        const BModel::BMesh& bmesh = m_Model.m_pBMesh[node.meshID];

        Math::Matrix4 objectToWorld = AffineTransform(node.rotation, node.position);
        objectToWorld = objectToWorld * Math::Matrix4::MakeScale(node.scale);
        vsConstants.modelToWorld = objectToWorld;
        vsConstants.worldToModel = Matrix4(::XMMatrixInverse(nullptr, objectToWorld));
        vsConstants.modelToProjection = ViewProjMat * objectToWorld;
        gfxContext.SetDynamicConstantBufferView(0, sizeof(vsConstants), &vsConstants);

        for (uint32_t i = 0; i < bmesh.subMeshCount; i++)
        {
            auto subMeshIndex = i + bmesh.subMeshOffset;
            const Model::Mesh& mesh = m_Model.m_pMesh[subMeshIndex];
            uint32_t indexCount = mesh.indexCount;
            uint32_t startIndex = mesh.indexDataByteOffset / m_Model.m_IndexBuffer.GetElementSize();
            uint32_t baseVertex = mesh.vertexDataByteOffset / VertexStride;
            gfxContext.DrawIndexed(indexCount, startIndex, baseVertex);

        }
        /*
        const Model::Mesh& mesh = m_Model.m_pMesh[node.meshID];

        uint32_t indexCount = mesh.indexCount;
        uint32_t startIndex = mesh.indexDataByteOffset / m_Model.m_IndexBuffer.GetElementSize();
        uint32_t baseVertex = mesh.vertexDataByteOffset / VertexStride;
        if (mesh.materialIndex != materialIdx)
        {
            if (m_pMaterialIsCutout[mesh.materialIndex] && !(Filter & kCutout) ||
                !m_pMaterialIsCutout[mesh.materialIndex] && !(Filter & kOpaque))
                continue;

            materialIdx = mesh.materialIndex;
            gfxContext.SetDynamicDescriptors(2, 0, 6, m_Model.GetSRVs(materialIdx));
        }
        uint32_t areNormalsNeeded = 1;// (rayTracingMode != RTM_REFLECTIONS) || m_pMaterialIsReflective[mesh.materialIndex];
        gfxContext.SetConstants(4, baseVertex, materialIdx);
        gfxContext.SetConstants(5, areNormalsNeeded);
        
        gfxContext.DrawIndexed(indexCount, startIndex, baseVertex);*/
    }
}

void DXRBaker::RenderObjects(GraphicsContext& gfxContext, const Matrix4& ViewProjMat, eObjectFilter Filter)
{
    struct VSConstants
    {
        Matrix4 modelToProjection;
        Matrix4 modelToShadow;
        XMFLOAT3 viewerPos;
    } vsConstants;
    vsConstants.modelToProjection = ViewProjMat;
    XMStoreFloat3(&vsConstants.viewerPos, m_Camera.GetPosition());

    gfxContext.SetDynamicConstantBufferView(0, sizeof(vsConstants), &vsConstants);

    uint32_t materialIdx = 0xFFFFFFFFul;

    uint32_t VertexStride = m_Model.m_VertexStride;

    for (uint32_t meshIndex = 0; meshIndex < m_Model.m_Header.meshCount; meshIndex++)
    {
        if (meshIndex == hiddenMeshID)
            continue;
        const Model::Mesh& mesh = m_Model.m_pMesh[meshIndex];

        uint32_t indexCount = mesh.indexCount;
        uint32_t startIndex = mesh.indexDataByteOffset / m_Model.m_IndexBuffer.GetElementSize();
        uint32_t baseVertex = mesh.vertexDataByteOffset / VertexStride;
        /*
        if (mesh.materialIndex != materialIdx)
        {
            if (m_pMaterialIsCutout[mesh.materialIndex] && !(Filter & kCutout) ||
                !m_pMaterialIsCutout[mesh.materialIndex] && !(Filter & kOpaque))
                continue;

            materialIdx = mesh.materialIndex;
            gfxContext.SetDynamicDescriptors(2, 0, 6, m_Model.GetSRVs(materialIdx));
        }
        uint32_t areNormalsNeeded = 1;// (rayTracingMode != RTM_REFLECTIONS) || m_pMaterialIsReflective[mesh.materialIndex];
        gfxContext.SetConstants(4, baseVertex, materialIdx);
        gfxContext.SetConstants(5, areNormalsNeeded);
        */
        gfxContext.DrawIndexed(indexCount, startIndex, baseVertex);
    }
}
void DXRBaker::Raytrace(class GraphicsContext& gfxContext)
{
    ScopedTimer _prof(L"Raytrace", gfxContext);

    gfxContext.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    RaytraceDiffuse(gfxContext, m_Camera, g_SceneColorBuffer);

    // Clear the gfxContext's descriptor heap since ray tracing changes this underneath the sheets
    gfxContext.SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, nullptr);
}

void DXRBaker::RaytraceDiffuse(
    GraphicsContext& context,
    const Math::Camera& camera,
    ColorBuffer& colorTarget)
{
    ScopedTimer _p0(L"RaytracingWithHitShader", context);

    // Prepare constants
    DynamicCB inputs = g_dynamicCb;
    auto m0 = camera.GetViewProjMatrix();
    auto m1 = Transpose(Invert(m0));
    memcpy(&inputs.cameraToWorld, &m1, sizeof(inputs.cameraToWorld));
    auto pos = camera.GetPosition();
    memcpy(&inputs.worldCameraPosition, &pos, sizeof(inputs.worldCameraPosition));
    inputs.resolution.x = (float)colorTarget.GetWidth();
    inputs.resolution.y = (float)colorTarget.GetHeight();

    HitShaderConstants hitShaderConstants = {};
    hitShaderConstants.sunDirection = m_SunDirection;
    hitShaderConstants.sunLight = Vector3(1.0f, 1.0f, 1.0f) * m_SunLightIntensity;
    hitShaderConstants.ambientLight = Vector3(1.0f, 1.0f, 1.0f) * m_AmbientIntensity;
    hitShaderConstants.ShadowTexelSize[0] = 1.0f / g_ShadowBuffer.GetWidth();
    hitShaderConstants.IsReflection = false;
    hitShaderConstants.UseShadowRays = 1;
    context.WriteBuffer(g_hitConstantBuffer, 0, &hitShaderConstants, sizeof(hitShaderConstants));
    context.WriteBuffer(g_dynamicConstantBuffer, 0, &inputs, sizeof(inputs));

    context.TransitionResource(g_dynamicConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    context.TransitionResource(g_SSAOFullScreen, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    context.TransitionResource(g_hitConstantBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
    context.TransitionResource(g_ShadowBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    context.TransitionResource(colorTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
    context.FlushResourceBarriers();

    ID3D12GraphicsCommandList* pCommandList = context.GetCommandList();

    CComPtr<ID3D12GraphicsCommandList4> pRaytracingCommandList;
    pCommandList->QueryInterface(IID_PPV_ARGS(&pRaytracingCommandList));

    ID3D12DescriptorHeap* pDescriptorHeaps[] = { &g_pRaytracingDescriptorHeap->GetDescriptorHeap() };
    pRaytracingCommandList->SetDescriptorHeaps(ARRAYSIZE(pDescriptorHeaps), pDescriptorHeaps);

    pCommandList->SetComputeRootSignature(g_GlobalRaytracingRootSignature);
    pCommandList->SetComputeRootDescriptorTable(0, g_SceneSrvs);
    pCommandList->SetComputeRootConstantBufferView(1, g_hitConstantBuffer.GetGpuVirtualAddress());
    pCommandList->SetComputeRootConstantBufferView(2, g_dynamicConstantBuffer.GetGpuVirtualAddress());
    pCommandList->SetComputeRootDescriptorTable(4, g_OutputUAV);
    pRaytracingCommandList->SetComputeRootShaderResourceView(7, g_bvh_topLevelAccelerationStructure->GetGPUVirtualAddress());

    D3D12_DISPATCH_RAYS_DESC dispatchRaysDesc = g_RaytracingInputs[DiffuseHitShader].GetDispatchRayDesc(colorTarget.GetWidth(), colorTarget.GetHeight());
    pRaytracingCommandList->SetPipelineState1(g_RaytracingInputs[DiffuseHitShader].m_pPSO);
    pRaytracingCommandList->DispatchRays(&dispatchRaysDesc);
}