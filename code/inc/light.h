#pragma once
#include "stdafx.h"
#include "texture.h"
#include "globals.h"

struct DirectionalLight
{
    DirectX::XMFLOAT3 Color;
    float Intensity;
    DirectX::XMFLOAT3 Direction;
    std::unique_ptr<RenderTexture> ShadowMaps[MAX_CASCADE_COUNT];
};

struct PointLight
{
    DirectX::XMFLOAT3 Color;
    float Intensity;
    DirectX::XMFLOAT3 Position;
    float Near = 0.1f;
    float Range = 10.0f;
    std::unique_ptr<RenderTexture> ShadowMap = nullptr;
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
};