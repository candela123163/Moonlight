#ifndef _CONSTANT_INCLUDE
#define _CONSTANT_INCLUDE

#define POINT_LIGHT_MAX_COUNT 4
#define SPOT_LIGHT_MAX_COUNT 4
#define SHADOW_CASCADE_MAX_COUNT 4


#define DEFINE_OBJ_CONSTANT(bx)             \
cbuffer ObjectConstant : register(bx)       \
{                                           \
    float4x4 _World;                        \
    float4x4 _InvTransposeWorld;            \
}                                         

#define DEFINE_MATERIAL_CONSTANT(bx)        \
cbuffer MaterialConstant : register(bx)     \
{                                           \
    float4 _AlbedoFactor;                   \
                                            \
    float _NormalScale;                     \
    int _NormalMapped;                      \
    float _MetallicFactor;                  \
    float _RoughnessFactor;                 \
                                            \
    int _AlbedoMapIndex;                    \
    int _NormalMapIndex;                    \
    int _MetalRoughMapIndex;                \
    int MaterialConstant_Pad;               \
}

#define DEFINE_CAMERA_CONSTANT(bx)          \
cbuffer CameraConstant : register(bx)       \
{                                           \
    float4x4 _View;                         \
    float4x4 _InvView;                      \
    float4x4 _Proj;                         \
    float4x4 _InvProj;                      \
    float4x4 _ViewProj;                     \
    float4x4 _InvViewProj;                  \
    float4 _EyePosW;                        \
    float4 _NearFar;                        \
}

struct PointLight
{
    float3 Color;
    float Intensity;
    
    float3 Position;
    float InvSquareRange;
};

struct SpotLight
{
    float3 Color;
    float InvSquareRange;
    
    float3 Position;
    float AttenuationFactorA;
    
    float3 Direction;
    float AttenuationFactorB;
    
    float Intensity;
    float3 pad;
};

#define DEFINE_LIGHT_CONSTANT(bx)                           \
cbuffer LightConstant : register(bx)                        \
{                                                           \
    float3 _SunColor;                                       \
    int _PointLightCount;                                   \
                                                            \
    float3 _SunDirection;                                   \
    int _SpotLightCount;                                    \
                                                            \
    float _SunIntensity;                                    \
    float3 LightConstant_pad;                                          \
                                                            \
    PointLight _PointLights[POINT_LIGHT_MAX_COUNT];         \
    SpotLight _SpotLights[SPOT_LIGHT_MAX_COUNT];            \
}

#define DEFINE_SHADOW_CASTER_CONSTANT(bx)                   \
cbuffer ShadowCasterConstant : register(bx)                 \
{                                                           \
    float4x4 _LightViewProject;                             \
    float3 _LightPosition;                                  \
    float _LightInvRange;                                   \
}

struct CascadeShadow
{
    float4x4 shadowTransform;
        
    int shadowMapIndex;
    float cascadeDistance;
    float normalBias;
    float softValue;
};

struct PointShadow
{
    int shadowMapIndex;
    float normalBias;
    float2 pad;
};

struct SpotShadow
{
    float4x4 shadowTransform;

    int shadowMapIndex;
    float normalBias;
    float2 pad;
};

#define DEFINE_SHADOW_CONSTANT(bx)                              \
cbuffer ShadowConstant : register(bx)                           \
{                                                               \
    CascadeShadow _ShadowCascade[SHADOW_CASCADE_MAX_COUNT];     \
    PointShadow _ShadowPoint[POINT_LIGHT_MAX_COUNT];            \
    SpotShadow _ShadowSpot[SPOT_LIGHT_MAX_COUNT];               \
                                                                \
    float _ShadowMaxDistance;                                   \
    float3 ShadowConstant_pad;                                  \
}

#define DEFINE_RENDER_TARGET_SIZE_CONSTANT(bx)                  \
cbuffer RenderTargetSizeConstant : register(bx)                 \
{                                                               \
    float2 _RenderTargetSize;                                   \
}                               
#endif