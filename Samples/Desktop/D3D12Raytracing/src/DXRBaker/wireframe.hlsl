
#define ModelViewer_RootSig \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
    "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \
    "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL), " \
    "DescriptorTable(SRV(t0, numDescriptors = 6), visibility = SHADER_VISIBILITY_PIXEL)," \
    "DescriptorTable(SRV(t64, numDescriptors = 6), visibility = SHADER_VISIBILITY_PIXEL)," \
    "RootConstants(b1, num32BitConstants = 2, visibility = SHADER_VISIBILITY_VERTEX), " \
    "RootConstants(b1, num32BitConstants = 1, visibility = SHADER_VISIBILITY_PIXEL), " \
    "StaticSampler(s0, maxAnisotropy = 8, visibility = SHADER_VISIBILITY_PIXEL)," \
    "StaticSampler(s1, visibility = SHADER_VISIBILITY_PIXEL," \
        "addressU = TEXTURE_ADDRESS_CLAMP," \
        "addressV = TEXTURE_ADDRESS_CLAMP," \
        "addressW = TEXTURE_ADDRESS_CLAMP," \
        "comparisonFunc = COMPARISON_GREATER_EQUAL," \
        "filter = FILTER_MIN_MAG_LINEAR_MIP_POINT)"

cbuffer VSConstants : register(b0)
{
    float4x4 modelToProjection;
    float4x4 modelToShadow;
    float4x4 modelToWorld;
    float4x4 worldToModel;
    float3 ViewerPos;
};

struct VSInput
{
    float3 position : POSITION;
    float4 color : COLOR;
};

cbuffer StartVertex : register(b1)
{
    uint baseVertex;
    uint materialIdx;
};

struct VSOutput
{
    float4 position : SV_Position;
    float3 worldPos : WorldPos;
    float3 viewDir : TexCoord1;
    float4 color : Color;
};

[RootSignature(ModelViewer_RootSig)]
VSOutput WireframeVS(VSInput vsInput, uint vertexID : SV_VertexID)
{
    VSOutput vsOutput;

    vsOutput.position = mul(modelToProjection, float4(vsInput.position, 1.0));
    vsOutput.worldPos = vsInput.position;
    vsOutput.viewDir = vsInput.position - ViewerPos;
    vsOutput.color = vsInput.color;

    return vsOutput;
}

[RootSignature(ModelViewer_RootSig)]
VSOutput ArrowVS(VSInput vsInput, uint vertexID : SV_VertexID)
{
    VSOutput vsOutput;

    vsOutput.position = mul(modelToProjection, float4(vsInput.position, 1.0));
    vsOutput.position += float4(-0.9, 0.8, 0, 0) * vsOutput.position.w;
    vsOutput.worldPos = vsInput.position;
    vsOutput.viewDir = vsInput.position - ViewerPos;
    vsOutput.color = vsInput.color;

    return vsOutput;
}

Texture2D<float3> texDiffuse        : register(t0);
SamplerState sampler0 : register(s0);
[RootSignature(ModelViewer_RootSig)]
float4 PSMain(VSOutput vsOutput) :SV_Target0
{
    return vsOutput.color;
}