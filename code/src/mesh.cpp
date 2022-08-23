#include "stdafx.h"
#include "mesh.h"
#include "util.h"
#include "globals.h"
using namespace std;

const std::vector<D3D12_INPUT_ELEMENT_DESC> Vertex::InputLayout =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

    { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

    { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

    { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }

};

Mesh::Mesh(const GraphicContext& context, const std::vector<Vertex>& vertexs, const std::vector<UINT>& indices)
{
    UINT vertexByteStride = sizeof(Vertex);
    UINT vertexByteSize = vertexByteStride * vertexs.size();
    mVertexBuffer = CreateDefaultBuffer(context.device, context.commandList, vertexs.data(), vertexByteSize, mVertexUploader);
    mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
    mVertexBufferView.StrideInBytes = vertexByteStride;
    mVertexBufferView.SizeInBytes = vertexByteSize;

    UINT indexByteStride = sizeof(UINT);
    UINT indexByteSize = indexByteStride * indices.size();
    mIndexBuffer = CreateDefaultBuffer(context.device, context.commandList, indices.data(), indexByteSize, mIndexUploader);
    mIndexBufferView.BufferLocation = mIndexBuffer->GetGPUVirtualAddress();
    mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mIndexBufferView.SizeInBytes = indexByteSize;
    
    mIndexCount = indices.size();

    // leave boundingbox for now
}

Mesh* Mesh::GetOrLoad(int meshIndex, const aiScene* scene, int globalMeshIndex, const GraphicContext& context)
{
    if (!Globals::MeshContainer.Contains(globalMeshIndex))
    {
        const aiMesh* mesh = scene->mMeshes[meshIndex];
        vector<Vertex> vertices;
        vector<UINT> indices;

        for (size_t i = 0; i < mesh->mNumVertices; i++)
        {
            vertices.push_back(
                Vertex(
                    mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z,
                    mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z,
                    mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z,
                    mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z,
                    mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y
                )
            );
        }

        for (size_t i = 0; i < mesh->mNumFaces; i++)
        {
            const aiFace& face = mesh->mFaces[i];
            for (size_t j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(static_cast<UINT>(face.mIndices[j]));
            }
        }

        Globals::MeshContainer.Insert(globalMeshIndex, make_unique<Mesh>(context, vertices, indices));
    }

    return Globals::MeshContainer.Get(globalMeshIndex);
}