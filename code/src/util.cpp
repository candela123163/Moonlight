#include "stdafx.h"
#include "util.h"
#include <comdef.h>
using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

std::wstring AnsiToWString(const std::string& str)
{
    WCHAR buffer[512];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
    return std::wstring(buffer);
}


DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
	ErrorCode(hr),
	FunctionName(functionName),
	Filename(filename),
	LineNumber(lineNumber)
{
}


std::wstring DxException::ToString()const
{
	// Get the string description of the error code.
	_com_error err(ErrorCode);
	std::wstring msg = err.ErrorMessage();

	return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}


Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION ;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if (errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}


D3D12_SHADER_BYTECODE LoadShader(std::wstring shaderCsoFile)
{
    ID3DBlob* shaderBlob;
    ThrowIfFailed(D3DReadFileToBlob(shaderCsoFile.c_str(), &shaderBlob));
    D3D12_SHADER_BYTECODE shaderBytecode = {};
    shaderBytecode.BytecodeLength = shaderBlob->GetBufferSize();
    shaderBytecode.pShaderBytecode = shaderBlob->GetBufferPointer();
    return shaderBytecode;
}


bool IsKeyDown(int vKeycode)
{
	return GetAsyncKeyState(vKeycode) & 0x8000;
}


ComPtr<ID3D12Resource> CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    ThrowIfFailed(device->CreateCommittedResource(
        get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT)),
        D3D12_HEAP_FLAG_NONE,
        get_rvalue_ptr(CD3DX12_RESOURCE_DESC::Buffer(byteSize)),
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    ThrowIfFailed(device->CreateCommittedResource(
        get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
        D3D12_HEAP_FLAG_NONE,
        get_rvalue_ptr(CD3DX12_RESOURCE_DESC::Buffer(byteSize)),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));


    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;
    
    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

    cmdList->ResourceBarrier(1, get_rvalue_ptr(CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ)));

    return defaultBuffer;
}


UINT CalcConstantBufferByteSize(UINT byteSize)
{
    return (byteSize + 255) & ~255;
}


float DegreeToRadians(float degree)
{
    return M_PI * degree / 180.0;
}

float RadiansToDegree(float radians)
{
    return radians * 180.0 * M_1_PI;
}


static FXMVECTOR looks[6] = {
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(1.0, 0.0, 0.0))),  // +X
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(-1.0, 0.0, 0.0))), // -X
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0, 1.0, 0.0))),  // +Y
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0, -1.0, 0.0))), // -Y
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0, 0.0, 1.0))),  // +Z
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0, 0.0, -1.0)))  // -Z
};
static FXMVECTOR ups[6] = {
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0, 1.0, 0.0))),  // +X
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0, 1.0, 0.0))),  // -X
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0, 0.0, -1.0))), // +Y
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0, 0.0, 1.0))),  // -Y
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0, 1.0, 0.0))),  // +Z
    XMLoadFloat3(get_rvalue_ptr(XMFLOAT3(0.0, 1.0, 0.0)))   // -Z
};

array<FXMMATRIX, 6> GenerateCubeViewMatrices(FXMVECTOR pos)
{
    return {
        XMMatrixLookToLH(pos, looks[0], ups[0]),
        XMMatrixLookToLH(pos, looks[1], ups[1]),
        XMMatrixLookToLH(pos, looks[2], ups[2]),
        XMMatrixLookToLH(pos, looks[3], ups[3]),
        XMMatrixLookToLH(pos, looks[4], ups[4]),
        XMMatrixLookToLH(pos, looks[5], ups[5])
    };

}

XMVECTOR GetCubeFaceNormal(UINT face)
{
    return looks[face];
}

double GetHalton(int index, int radix)
{
    double result = 0;
    double fraction = 1.0 / (double)radix;

    while (index > 0)
    {
        result += (double)(index % radix) * fraction;

        index /= radix;
        fraction /= (double)radix;
    }

    return result;
}