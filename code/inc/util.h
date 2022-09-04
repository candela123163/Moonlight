#pragma once
#include "stdafx.h"

std::wstring AnsiToWString(const std::string& str);


class DxException
{
public:
    DxException() = default;
    DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);

    std::wstring ToString()const;

    HRESULT ErrorCode = S_OK;
    std::wstring FunctionName;
    std::wstring Filename;
    int LineNumber = -1;
};


#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);                       \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif


Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
    const std::wstring& filename,
    const D3D_SHADER_MACRO* defines,
    const std::string& entrypoint,
    const std::string& target);


D3D12_SHADER_BYTECODE LoadShader(std::wstring shaderCsoFile);

bool IsKeyDown(int vKeycode);


template<typename T>
T* get_rvalue_ptr(T&& v)
{
    return &v;
}


Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer);


UINT CalcConstantBufferByteSize(UINT byteSize);


template<typename T, bool isConstant = false>
class UploadBuffer
{
public:
    UploadBuffer(ID3D12Device* device, UINT elementCount = 1):
        mElementCount(elementCount)
    {
       
        if constexpr(isConstant)
            mElementByteSize = CalcConstantBufferByteSize(sizeof(T));
        else
            mElementByteSize = sizeof(T);

        ThrowIfFailed(device->CreateCommittedResource(
            get_rvalue_ptr(CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD)),
            D3D12_HEAP_FLAG_NONE,
            get_rvalue_ptr(CD3DX12_RESOURCE_DESC::Buffer(mElementByteSize * elementCount)),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(mUploadBuffer.GetAddressOf())));

        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData)));
    }

    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer()
    {
        if (mUploadBuffer != nullptr)
            mUploadBuffer->Unmap(0, nullptr);

        mMappedData = nullptr;
    }

    ID3D12Resource* GetResource() const
    {
        return mUploadBuffer.Get();
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetElementGPUAddress(UINT index = 0) 
    {
        assert(index < mElementCount);
        return mUploadBuffer->GetGPUVirtualAddress() + index * mElementByteSize;
    }

    void CopyData(const T& data, UINT index = 0)
    {
        assert(index < mElementCount);
        memcpy(&mMappedData[index * mElementByteSize], &data, sizeof(T));
    }


private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE* mMappedData = nullptr;

    UINT mElementByteSize = 0;
    UINT mElementCount = 0;
};


float DegreeToRadians(float degree);

float RadiansToDegree(float radians);


std::array<DirectX::FXMMATRIX, 6> GenerateCubeViewMatrices(DirectX::FXMVECTOR pos);

DirectX::XMVECTOR GetCubeFaceNormal(UINT face);

template<typename T1, typename T2>
bool Intersects(
    const T1& boungdingVolume1,
    const DirectX::XMMATRIX& LtoW,
    const T2& boungdingVolume2,
    const DirectX::XMMATRIX& WtoL
)
{
    DirectX::XMMATRIX L1toL2 = XMMatrixMultiply(LtoW, WtoL);
    T1 localVolume;
    boungdingVolume1.Transform(localVolume, L1toL2);
    return localVolume.Intersects(boungdingVolume2);
}