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

    materialConstant.NormalMapped = (int)normalMapped;
    materialConstant.UseMetalRoughness = (int)useMetalRoughness;
    materialConstant.UseEmissive = (int)useEmissive;
    materialConstant.UseAO = (int)useAO;

    materialConstant.AlbedoMapIndex = albedoMap->GetSrvDescriptorData().HeapIndex;
    materialConstant.NormalMapIndex = normalMap->GetSrvDescriptorData().HeapIndex;
    materialConstant.MetalRoughMapIndex = metalRoughnessMap->GetSrvDescriptorData().HeapIndex;
    materialConstant.AOMapIndex = aoMap->GetSrvDescriptorData().HeapIndex;

    materialConstant.EmissiveMapIndex = emissiveMap->GetSrvDescriptorData().HeapIndex;
    materialConstant.EmissiveFactor = emissiveFactor;

    materialConstant.AlbedoFactor = albedoFactor;
    
    materialConstant.NormalScale = normalScale;
    materialConstant.MetallicFactor = metallicFactor;
    materialConstant.RoughnessFactor = roughnessFactor;

    context.frameResource->ConstantMaterial->CopyData(materialConstant, materialID);
}

Material::Material(int _materialID, const aiScene* scene, const GraphicContext& context)
{
    this->materialID = _materialID;

    const aiMaterial* material = scene->mMaterials[_materialID];
    aiString textureName;
    aiColor4D albedoFactor;

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
    material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, albedoFactor);
    this->albedoMap = Texture::GetOrLoad(texturePath, true, context);
    this->albedoFactor = XMFLOAT4(albedoFactor.r, albedoFactor.g, albedoFactor.b, albedoFactor.a);

    // normal
    result = material->GetTexture(aiTextureType::aiTextureType_NORMALS, 0, &textureName);
    if (result == aiReturn::aiReturn_SUCCESS)
    {
        texturePath = Globals::ImagePath / textureName.C_Str();
        this->normalMapped= true;
    }
    else
    {
        texturePath = Globals::ImagePath / DUMMY_TEXTURE;
        this->normalMapped = false;
    }
    material->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), this->normalScale);
    this->normalMap = Texture::GetOrLoad(texturePath, false, context);
    

    // metallic & roughness
    result = material->GetTexture(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLICROUGHNESS_TEXTURE, &textureName);
    if (result == aiReturn::aiReturn_SUCCESS)
    {
        texturePath = Globals::ImagePath / textureName.C_Str();
        this->useMetalRoughness = true;
    }
    else
    {
        texturePath = Globals::ImagePath / DUMMY_TEXTURE;
        this->useMetalRoughness = false;
    }
    this->metalRoughnessMap = Texture::GetOrLoad(texturePath, false, context);
    material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, this->metallicFactor);
    material->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, this->roughnessFactor);

    // emissive
    result = material->GetTexture(aiTextureType_EMISSIVE, 0, &textureName);
    if (result == aiReturn::aiReturn_SUCCESS)
    {
        texturePath = Globals::ImagePath / textureName.C_Str();
        this->useEmissive = true;
    }
    else
    {
        texturePath = Globals::ImagePath / DUMMY_TEXTURE;
        this->useEmissive = false;
    }
    this->emissiveMap = Texture::GetOrLoad(texturePath, true, context);
    aiColor3D emissiveColor;
    material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
    this->emissiveFactor = XMFLOAT3(emissiveColor.r, emissiveColor.g, emissiveColor.b);

    // ao
    result = material->GetTexture(aiTextureType_LIGHTMAP, 0, &textureName);
    if (result == aiReturn::aiReturn_SUCCESS)
    {
        texturePath = Globals::ImagePath / textureName.C_Str();
        this->useAO = true;
    }
    else
    {
        texturePath = Globals::ImagePath / DUMMY_TEXTURE;
        this->useAO = false;
    }
    this->aoMap = Texture::GetOrLoad(texturePath, false, context);
    
}

Material* Material::GetOrLoad(int materiaIndex, const aiScene* scene, const GraphicContext& context)
{
    if (!Globals::MaterialContainer.Contains(materiaIndex))
    {
        
        return Globals::MaterialContainer.Insert(
            materiaIndex,
            make_unique<Material>(
                materiaIndex,
                scene,
                context
                )
        );
    }

    return Globals::MaterialContainer.Get(materiaIndex);
}