#include "stdafx.h" 
#include "material.h"
#include "frameResource.h"
#include "texture.h"
using namespace std;
using namespace DirectX;

void Material::UpdateConstant(const GraphicContext& context)
{
    if (!StillDirty())
        return;
    
    mDirtyCount--;

    MaterialConstant materialConstant;
    materialConstant.AlbedoFactor = albedoFactor;
    
    materialConstant.NormalScale = normalScale;
    materialConstant.NormalMapped = static_cast<int>(normalMapped);
    materialConstant.MetallicFactor = metallicFactor;
    materialConstant.RoughnessFactor = roughnessFactor;

    materialConstant.AlbedoMapIndex = albedoMap->GetSrvDescriptorData().HeapIndex;
    materialConstant.NormalMapIndex = normalMap->GetSrvDescriptorData().HeapIndex;
    materialConstant.MetalRoughMapIndex = metalRoughnessMap->GetSrvDescriptorData().HeapIndex;

    context.frameResource->ConstantMaterial->CopyData(materialConstant, materialID);
}

Material* Material::GetOrLoad(int materiaIndex, const aiScene* scene, const GraphicContext& context)
{
    if (!Globals::MaterialContainer.Contains(materiaIndex))
    {
        const aiMaterial* material = scene->mMaterials[materiaIndex];
        aiString textureName;
        aiColor4D albedoFactor;
        float normalScale = 1.0, metalFactor = 0.0, roughFactor = 1.0;
        bool useNormal = true;
        Texture* albedo = nullptr, * normal = nullptr, * metalicRough = nullptr;
        aiReturn result;

        // albeo
        result = material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_TEXTURE, &textureName);
        filesystem::path texturePath;

        if (result == aiReturn::aiReturn_SUCCESS)
        {
            texturePath = Globals::ImagePath / textureName.C_Str();
        }
        else
        {
            texturePath = Globals::ImagePath / DUMMY_TEXTURE;
        }
        albedo = Texture::GetOrLoad(texturePath, context);
        material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, albedoFactor);

        // normal
        result = material->GetTexture(aiTextureType::aiTextureType_NORMALS, 0, &textureName);
        if (result == aiReturn::aiReturn_SUCCESS)
        {
            texturePath = Globals::ImagePath / textureName.C_Str();
            useNormal = true;
        }
        else
        {
            texturePath = Globals::ImagePath / DUMMY_TEXTURE;
            useNormal = false;
        }
        normal = Texture::GetOrLoad(texturePath, context);
        material->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), normalScale);

        // metallic & roughness
        result = material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &textureName);
        if (result == aiReturn::aiReturn_SUCCESS)
        {
            texturePath = Globals::ImagePath / textureName.C_Str();
        }
        else
        {
            texturePath = Globals::ImagePath / DUMMY_TEXTURE;
        }
        metalicRough = Texture::GetOrLoad(texturePath, context);
        material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, metalFactor);
        material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, roughFactor);

        return Globals::MaterialContainer.Insert(
            materiaIndex,
            make_unique<Material>(
                materiaIndex,
                albedo, XMFLOAT4(albedoFactor.r, albedoFactor.g, albedoFactor.b, albedoFactor.a),
                normal, normalScale, useNormal,
                metalicRough, metalFactor, roughFactor
                )
        );
    }

    return Globals::MaterialContainer.Get(materiaIndex);
}