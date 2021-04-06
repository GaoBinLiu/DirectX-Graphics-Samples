#pragma once
#include "stdafx.h"

class ShaderResource
{
public:
	ShaderResource() {}
	ShaderResource(ComPtr<ID3DBlob> vs, ComPtr<ID3DBlob> ps) :vertexShader(vs), pixelShader(ps) {}
	ShaderResource(LPCWSTR fileName, LPCSTR pVSEntrypoint = nullptr, LPCSTR pPSEntrypoint = nullptr);
	ShaderResource(LPCWSTR vsfileName, LPCWSTR psfileName, LPCSTR pVSEntrypoint = nullptr, LPCSTR pPSEntrypoint = nullptr);
	static ComPtr<ID3DBlob> LoadVS(LPCWSTR fileName, LPCSTR pVSEntrypoint = nullptr);
	static ComPtr<ID3DBlob> LoadPS(LPCWSTR fileName, LPCSTR pPSEntrypoint = nullptr);
	void Reload();
	CD3DX12_SHADER_BYTECODE GetVS();
	CD3DX12_SHADER_BYTECODE GetPS();
	GraphicsPSO& GetPSO() { return pso; }
	static void Init(LPCWSTR);
private:
	std::wstring vsFilename;
	std::wstring psFilename;
	std::string vsEntrypoint;
	std::string psEntrypoint;
	ComPtr<ID3DBlob> vertexShader;
	ComPtr<ID3DBlob> pixelShader;
	GraphicsPSO pso;
	static std::wstring rootPath;
	static ComPtr<ID3DBlob> errorVertexShader;
	static ComPtr<ID3DBlob> errorPixelShader;
};

