#pragma once
#include "stdafx.h"
#include "globals.h"

class Texture;
struct GraphicContext;
struct MaterialConstant;

class Material
{
public:
    Material(int _materialID, Texture* _albedo, DirectX::XMFLOAT4 _albedoFactor,
        Texture* _normal, float _normalScale, bool _useNormal,
        Texture* _metalRough, float _metalFactor, float _roughFactor):
        materialID(_materialID),
        albedoMap(_albedo), albedoFactor(_albedoFactor),
        normalMap(_normal), normalScale(_normalScale), normalMapped(_useNormal),
        metalRoughnessMap(_metalRough), metallicFactor(_metalFactor), roughnessFactor(_roughFactor)
    {}

    UINT GetID() const { return materialID; }

    bool StillDirty() const { return mDirtyCount > 0; }

    void UpdateConstant(const GraphicContext& context);

    static Material* GetOrLoad(int materialID, const aiScene* scene, const GraphicContext& context);

private:
    UINT materialID = 0;
    UINT mDirtyCount = FRAME_COUNT;

    Texture* albedoMap = nullptr;
    DirectX::XMFLOAT4 albedoFactor = DirectX::XMFLOAT4(1.0, 1.0, 1.0, 1.0);

    Texture* normalMap = nullptr;
    float normalScale = 1.0;
    bool normalMapped = false;

    Texture* metalRoughnessMap = nullptr;
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
};