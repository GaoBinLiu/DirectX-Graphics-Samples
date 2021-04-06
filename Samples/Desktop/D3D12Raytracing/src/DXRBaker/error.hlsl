
#define ModelViewer_RootSig \
    "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), " \
    "CBV(b0, visibility = SHADER_VISIBILITY_VERTEX), " \
    "CBV(b0, visibility = SHADER_VISIBILITY_PIXEL)" 

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
};

struct VSOutput
{
    float4 position : SV_Position;
};

[RootSignature(ModelViewer_RootSig)]
VSOutput VSMain(VSInput vsInput, uint vertexID : SV_VertexID)
{
    VSOutput vsOutput;

    vsOutput.position = mul(modelToProjection, float4(vsInput.position, 1.0));
    return vsOutput;
}

[RootSignature(ModelViewer_RootSig)]
float4 PSMain(VSOutput vsOutput) :SV_Target0
{
	return float4(1,0,1,1);
}