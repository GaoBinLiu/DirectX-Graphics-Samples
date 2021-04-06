#include "ShaderResource.h"

std::wstring ShaderResource::rootPath;
ComPtr<ID3DBlob> ShaderResource::errorVertexShader;
ComPtr<ID3DBlob> ShaderResource::errorPixelShader;

ShaderResource::ShaderResource(LPCWSTR fileName, LPCSTR pVSEntrypoint, LPCSTR pPSEntrypoint)
{
    vsFilename = std::wstring(fileName);
    psFilename = std::wstring(fileName);
    vsEntrypoint = pVSEntrypoint == nullptr ? "VSMain" : std::string(pVSEntrypoint);
    psEntrypoint = pPSEntrypoint == nullptr ? "PSMain" : std::string(pPSEntrypoint);
    Reload();
}

ShaderResource::ShaderResource(LPCWSTR vsfileName, LPCWSTR psfileName, LPCSTR pVSEntrypoint, LPCSTR pPSEntrypoint)
{
    vsFilename = std::wstring(vsfileName);
    psFilename = std::wstring(psfileName);
    vsEntrypoint = pVSEntrypoint == nullptr ? "VSMain" : std::string(pVSEntrypoint);
    psEntrypoint = pPSEntrypoint == nullptr ? "PSMain" : std::string(pPSEntrypoint);
    Reload();
}

ComPtr<ID3DBlob> ShaderResource::LoadVS(LPCWSTR fileName, LPCSTR pVSEntrypoint)
{
#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    static const std::string defaultEntrypoint = "VSMain";
    pVSEntrypoint = pVSEntrypoint == nullptr ? defaultEntrypoint.c_str() : pVSEntrypoint;
    ComPtr<ID3DBlob> vertexShader; 
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3DCompileFromFile(fileName, nullptr, nullptr, pVSEntrypoint, "vs_5_0", compileFlags, 0, &vertexShader, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
            return errorVertexShader;
        }
    }
    return vertexShader;
}

ComPtr<ID3DBlob> ShaderResource::LoadPS(LPCWSTR fileName, LPCSTR pPSEntrypoint)
{
#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    static const std::string defaultEntrypoint = "PSMain";
    pPSEntrypoint = pPSEntrypoint == nullptr ? defaultEntrypoint.c_str() : pPSEntrypoint;
    ComPtr<ID3DBlob> pixelShader;
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3DCompileFromFile(fileName, nullptr, nullptr, pPSEntrypoint, "ps_5_0", compileFlags, 0, &pixelShader, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
            return errorPixelShader;
        }
    }
    return pixelShader;
}

void ShaderResource::Reload()
{
    vertexShader = LoadVS(vsFilename.c_str(), vsEntrypoint.c_str());
    pixelShader = LoadPS(psFilename.c_str(), psEntrypoint.c_str());
    pso.SetVertexShader(GetVS());
    pso.SetPixelShader(GetPS());
}

CD3DX12_SHADER_BYTECODE ShaderResource::GetVS()
{
    return CD3DX12_SHADER_BYTECODE(vertexShader.Get());
}

CD3DX12_SHADER_BYTECODE ShaderResource::GetPS()
{
    return CD3DX12_SHADER_BYTECODE(pixelShader.Get());
}

void ShaderResource::Init(LPCWSTR path)
{
    rootPath = path;

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compileFlags = 0;
#endif
    ComPtr<ID3DBlob> error;
    HRESULT hr = D3DCompileFromFile((rootPath + L"error.hlsl").c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &errorVertexShader, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
            throw HrException(hr);
        }
    }
    hr = D3DCompileFromFile((rootPath + L"error.hlsl").c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &errorPixelShader, &error);
    if (FAILED(hr))
    {
        if (error)
        {
            OutputDebugStringA((char*)error->GetBufferPointer());
            throw HrException(hr);
        }
    }
}
