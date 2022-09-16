#pragma once
#include "stdafx.h"

struct RenderOption
{
    // light & shadow
    DirectX::XMFLOAT3 SunDirection;
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
};