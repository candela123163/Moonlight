#pragma once
#include "stdafx.h"

struct RenderOption
{
    // light & shadow
    bool IBLEnable;
    DirectX::XMFLOAT3 SunDirection;
    DirectX::XMFLOAT3 SunColor;
    float SunIntensity;

    // SSAO
    bool SSAOEnable;
    float SSAORadius;
    float SSAOPower;

    // Bloom
    bool BloomEnable;
    float BloomThreshold;
    float BloomCurveKnee;
    float BloomIntensity;

    // TAA
    bool TAAEnable;
    DirectX::XMFLOAT2 TAAClipBound;
    float TAAMotionWeight;
    float TAASharpness;
};