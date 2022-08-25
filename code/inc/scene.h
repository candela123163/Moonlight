#pragma once
#include "stdafx.h"
#include "camera.h"
#include "light.h"
#include "frameResource.h"
#include "texture.h"
#include "mesh.h"
#include "material.h"

struct GraphicContext;

class Instance
{
public:
    Instance(UINT id, DirectX::XMFLOAT4X4 ts, Material* material, Mesh* mesh):
        mInstanceID(id),
        mTransform(ts), mMaterial(material), mMesh(mesh) {}

    Material* GetMaterial() const { return mMaterial; }
    Mesh* GetMesh() const { return mMesh; }

    UINT GetID() const { return mInstanceID; }

    bool StillDirty() const { return mDirtyCount > 0; }
    
    void UpdateConstant(const GraphicContext& context);

private:
    DirectX::XMFLOAT4X4 mTransform;
    Material* mMaterial;
    Mesh* mMesh;

    UINT mInstanceID = 0;
    UINT mDirtyCount = FRAME_COUNT;
};

struct SkyBox
{
    Mesh* mesh;
    Texture* texture;
};

class Scene
{
public:
    bool Load(const std::filesystem::path& filePath, const GraphicContext& context);
    void OnLoadOver(const GraphicContext& context);
    void OnUpdate(const GraphicContext& context);

    void OnMouseDown(WPARAM btnState, int x, int y);
    void OnMouseUp(WPARAM btnState, int x, int y);
    void OnMouseMove(WPARAM btnState, int x, int y, float dx, float dy);
    void OnKeyboardInput(const Timer& timer);

    const Camera* GetCamera() const { return mCamera.get(); }
    const DirectionalLight& GetDirectionalLight() const { return mDirectionalLight; }
    const std::vector<PointLight>& GetPointLights() const { return mPointLights; }
    const std::vector<SpotLight>& GetSpotLights() const { return mSpotLights; }

    const std::vector<Instance*>& GetRenderInstances() const { return mSortedInstances; }
    const SkyBox& GetSkybox() const { return mSkybox; }

private:

    bool LoadSkyBox(const nlohmann::json& sceneConfig, const GraphicContext& context);
    bool LoadLight(const nlohmann::json& sceneConfig, const GraphicContext& context);
    bool LoadCamera(const nlohmann::json& sceneConfig, const GraphicContext& context);
    bool LoadInstance(const nlohmann::json& sceneConfig, const GraphicContext& context);
    bool LoadInstance(const aiNode* node, const aiScene* scene, const GraphicContext& context);
    
    void InitCamera(DirectX::FXMVECTOR pos,
        DirectX::FXMVECTOR target, 
        DirectX::FXMVECTOR worldUp,
        float fovY, float aspect, float zNear, float zFar);

    void GenerateRenderInstances();

    void UpdateInstanceConstant(const GraphicContext& context);
    void UpdateMaterialConstant(const GraphicContext& context);
    void UpdateLightShadowConstant(const GraphicContext& context);

    bool StillLightDirty() const { return mLightDirtyCount > 0; }
    void MarkLightDirty() { mLightDirtyCount = FRAME_COUNT; }

private:
    std::unique_ptr<Camera> mCamera;

    // lights
    DirectionalLight mDirectionalLight;
    std::vector<PointLight> mPointLights;
    std::vector<SpotLight> mSpotLights;
    UINT mLightDirtyCount = FRAME_COUNT;
    
    std::vector<std::unique_ptr<Instance>> mInstances;
    std::vector<Instance*> mSortedInstances;

    SkyBox mSkybox;
};