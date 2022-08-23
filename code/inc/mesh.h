#pragma once
#include "stdafx.h"
using Microsoft::WRL::ComPtr;

struct GraphicContext;

struct Vertex
{
    Vertex(
        const DirectX::XMFLOAT3& p,
        const DirectX::XMFLOAT3& n,
        const DirectX::XMFLOAT3& t,
        const DirectX::XMFLOAT3& b,
        const DirectX::XMFLOAT2& uv) :
        Position(p),
        Normal(n),
        TangentU(t),
        BiTangent(b),
        TexC(uv) {}

    Vertex(
        float px, float py, float pz,
        float nx, float ny, float nz,
        float tx, float ty, float tz,
        float bx, float by, float bz,
        float u, float v) :
        Position(px, py, pz),
        Normal(nx, ny, nz),
        TangentU(tx, ty, tz),
        BiTangent(bx, by, bz),
        TexC(u, v) {}

    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT3 TangentU;
    DirectX::XMFLOAT3 BiTangent;
    DirectX::XMFLOAT2 TexC;

    const static std::vector<D3D12_INPUT_ELEMENT_DESC> InputLayout;
};

class Mesh
{
public:
    
    Mesh(const GraphicContext& context, const std::vector<Vertex>& vertexs, const std::vector<UINT>& indices);

    D3D12_VERTEX_BUFFER_VIEW VertexBufferView() const { return mVertexBufferView; }
    D3D12_INDEX_BUFFER_VIEW IndexBufferView() const { return mIndexBufferView; }
    UINT IndexCount() const { return mIndexCount; }
    DirectX::BoundingBox BoundingBox() const { return mBoundingBox; }

    static Mesh* GetOrLoad(int meshID, const aiScene* scene, int globalMeshIndex, const GraphicContext& context);

private:
    ComPtr<ID3D12Resource> mVertexBuffer = nullptr;
    ComPtr<ID3D12Resource> mIndexBuffer = nullptr;

    ComPtr<ID3D12Resource> mVertexUploader = nullptr;
    ComPtr<ID3D12Resource> mIndexUploader = nullptr;

    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
    UINT mIndexCount = 0;

    DirectX::BoundingBox mBoundingBox;
};