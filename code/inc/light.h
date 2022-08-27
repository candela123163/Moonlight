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
};

enum class CubeFaceID
{
    PositiveX = 0,
    NegativeX = 1,
    PositiveY = 2,
    NegativeY = 3,
    PositiveZ = 4,
    NegativeZ = 5,

    COUNT = 6
};

struct PointLight
{
    DirectX::XMFLOAT3 Color;
    float Intensity;
    DirectX::XMFLOAT3 Position;
    float Near = 0.1f;
    float Range = 10.0f;
    std::unique_ptr<RenderTexture> ShadowMap = nullptr;

    DirectX::XMMATRIX Views[(int)CubeFaceID::COUNT];
    DirectX::XMMATRIX Project;
    DirectX::BoundingFrustum Frustum;

    DirectX::BoundingSphere BoundingSphere;

    DirectX::XMMATRIX GetViewProject(CubeFaceID face)
    {
        return XMMatrixMultiply(Views[(int)face], Project);
    }

    DirectX::XMMATRIX GetInvViewProject(CubeFaceID face)
    {
        DirectX::XMMATRIX viewProject = GetViewProject(face);
        return XMMatrixInverse(get_rvalue_ptr(XMMatrixDeterminant(viewProject)), viewProject);
    }

    DirectX::XMMATRIX GetInvView(CubeFaceID face)
    {
        return XMMatrixInverse(get_rvalue_ptr(XMMatrixDeterminant(Views[(int)face])), Views[(int)face]);
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
    DirectX::XMMATRIX InViewProject;
    DirectX::BoundingFrustum Frustum;
};