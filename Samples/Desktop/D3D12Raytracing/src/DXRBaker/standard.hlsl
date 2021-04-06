
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
    float2 texcoord0 : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float2 texcoord1 : TEXCOORD1;
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
    float2 uv : TexCoord0;
    float3 viewDir : TexCoord1;
    float3 shadowCoord : TexCoord2;
    float2 uv1 : TexCoord4;
    float3 normal : Normal;
    float3 tangent : Tangent;
    float3 bitangent : Bitangent;
#if ENABLE_TRIANGLE_ID
    uint vertexID : TexCoord3;
#endif
};

[RootSignature(ModelViewer_RootSig)]
VSOutput VSMain(VSInput vsInput, uint vertexID : SV_VertexID)
{
    VSOutput vsOutput;

    vsOutput.position = mul(modelToProjection, float4(vsInput.position, 1.0));
    vsOutput.worldPos = vsInput.position;
    vsOutput.uv = vsInput.texcoord0;
    vsOutput.uv1 = vsInput.texcoord1;
    vsOutput.viewDir = vsInput.position - ViewerPos;
    vsOutput.shadowCoord = mul(modelToShadow, float4(vsInput.position, 1.0)).xyz;

    //vsOutput.normal = mul(vsInput.normal,(float3x3)worldToModel);
    vsOutput.normal = normalize(mul(vsInput.normal, (float3x3)worldToModel));
    vsOutput.tangent = vsInput.tangent;
    vsOutput.bitangent = vsInput.bitangent;

#if ENABLE_TRIANGLE_ID
    //vsOutput.vertexID = (baseVertex & 0xFFFF) << 16 | (vertexID & 0xFFFF);
    vsOutput.vertexID = materialIdx << 24 | (vertexID & 0xFFFF);
#endif

    return vsOutput;
}

Texture2D<float3> texDiffuse        : register(t0);
SamplerState sampler0 : register(s0);
[RootSignature(ModelViewer_RootSig)]
float4 PSMain(VSOutput vsOutput) :SV_Target0
{
    uint2 pixelPos = uint2(vsOutput.position.xy);
# define SAMPLE_TEX(texName) texName.Sample(sampler0, vsOutput.uv)

    float3 diffuseAlbedo = SAMPLE_TEX(texDiffuse);
    float3 col = 1-(vsOutput.normal*0.5+0.5);
    float z = saturate((vsOutput.position.y-50) * 0.1f)*0.5+0.5;
    return col.rgbr;
    //return float4(vsOutput.uv1, 0, 1);
    //return 1;
}