#pragma once
#include "stdafx.h"
#include "texture.h"
#include "globals.h"
#include "util.h"

struct DirectionalLight
{
    DirectX::XMFLOAT3 Color;
    float Intensity;
    DirectX::XMFLOAT3 Direction;
    std::unique_ptr<RenderTexture> ShadowMaps[MAX_CASCADE_COUNT];

    float ShadowBias = 1.5f;
    bool castShadow = true;
};


struct PointLight
{
    DirectX::XMFLOAT3 Color;
    float Intensity;
    DirectX::XMFLOAT3 Position;
    float Near = 0.1f;
    float Range = 10.0f;
    std::unique_ptr<RenderTexture> ShadowMap = nullptr;

    DirectX::XMMATRIX Views[6];
    DirectX::XMMATRIX Project;
    DirectX::BoundingFrustum Frustum;

    DirectX::BoundingSphere BoundingSphere;

    bool castShadow = true;

    float ShadowBias = 2.0f;

    DirectX::XMMATRIX GetViewProject(UINT face) const
    {
        return XMMatrixMultiply(Views[(int)face], Project);
    }

    DirectX::XMMATRIX GetInvViewProject(UINT face) const
    {
        DirectX::XMMATRIX viewProject = GetViewProject(face);
        DirectX::XMVECTOR determinant;
        return XMMatrixInverse(&determinant, viewProject);
    }

    DirectX::XMMATRIX GetInvView(UINT face) const
    {
        DirectX::XMVECTOR determinant;
        return XMMatrixInverse(&determinant, Views[(int)face]);
    }

};

struct SpotLight
{
    DirectX::XMFLOAT3 Color;
    float Intensity;
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Direction;
    float Near = 0.1f;
    float Range = 10.0f;
    float OutterAngle = 60.0f;
    float InnerAngle = 45.0f;
    std::unique_ptr<RenderTexture> ShadowMap = nullptr;

    DirectX::XMMATRIX View;
    DirectX::XMMATRIX InvView;
    DirectX::XMMATRIX Project;
    DirectX::XMMATRIX ViewProject;
    DirectX::XMMATRIX InvViewProject;
    DirectX::BoundingFrustum Frustum;

    float ShadowBias = 1.5f;
    bool castShadow = true;
};