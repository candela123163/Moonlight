#ifndef _STATIC_SAMPLERS_INCLUDE
#define _STATIC_SAMPLERS_INCLUDE

SamplerState _SamplerPointWrap : register(s0);
SamplerState _SamplerPointClamp : register(s1);
SamplerState _SamplerLinearWrap : register(s2);
SamplerState _SamplerLinearClamp : register(s3);
SamplerState _SamplerAnisotropicWrap : register(s4);
SamplerState _SamplerAnisotropicClamp : register(s5);
SamplerComparisonState _SamplerShadowLess : register(s6);
SamplerComparisonState _SamplerShadowGreater : register(s7);
#endif