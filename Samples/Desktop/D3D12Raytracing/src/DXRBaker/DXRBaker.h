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
#include "CompiledShaders/MissShadowsLib.h"
#include <MotionBlur.h>
#include <FXAA.h>
#include <TemporalEffects.h>
#include <MotionBlur.h>
#include <SSAO.h>
//#include "RaytracingHlslCompat.h"
//#include "ModelViewerRayTracing.h"
#include "Global.h"

using namespace GameCore;
using namespace Math;
using namespace Graphics;
class DXRBaker : public GameCore::IGameApp
{
public:
    DXRBaker(void);
    ~DXRBaker();
    virtual void Startup(void) override;
    virtual void Cleanup(void) override;

    virtual void Update(float deltaT) override;
    virtual void RenderScene(void) override;
    virtual void RenderUI(class GraphicsContext&) override;
    void Test();
private:
    void SetCameraToPredefinedPosition(int cameraPosition);
    void RefreshResource();
    void ReloadModel(int id);
    void RenderEditorItems(GraphicsContext& gfxContext, const Matrix4& ViewProjMat);

    enum eObjectFilter { kOpaque = 0x1, kCutout = 0x2, kTransparent = 0x4, kAll = 0xF, kNone = 0x0 };
    void RenderObjects(GraphicsContext& Context, const Matrix4& ViewProjMat, eObjectFilter Filter = kAll);
    void RenderNodes(GraphicsContext& Context, const Matrix4& ViewProjMat, eObjectFilter Filter = kAll);
    void Raytrace(class GraphicsContext& gfxContext);

    void RaytraceDiffuse(GraphicsContext& context, const Math::Camera& camera, ColorBuffer& colorTarget);

    Camera m_Camera;
    std::unique_ptr<CameraController> m_CameraController;
    Matrix4 m_ViewProjMatrix;
    D3D12_VIEWPORT m_MainViewport;
    D3D12_RECT m_MainScissor;
    Vector3 m_SunDirection;

    struct CameraPosition
    {
        Vector3 position;
        float heading;
        float pitch;
    };

    CameraPosition m_CameraPosArray[c_NumCameraPositions];
    UINT m_CameraPosArrayCurrentPosition;

    RootSignature m_RootSig;

    ShaderResource* m_MainShader = nullptr;
    ShaderResource* m_WireframeShader = nullptr;
    ShaderResource* m_ArrowShader = nullptr;
    BModel m_Model;
    EditorModel m_EditorModel;
    std::vector<bool> m_pMaterialIsCutout;
    std::vector<bool> m_pMaterialIsReflective;

    int hiddenMeshID = -1;
};

