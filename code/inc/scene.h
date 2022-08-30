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
    Instance(UINT id, DirectX::XMFLOAT4X4 ts, Material* material, Mesh* mesh);

    Material* GetMaterial() const { return mMaterial; }
    Mesh* GetMesh() const { return mMesh; }

    UINT GetID() const { return mInstanceID; }

    bool StillDirty() const { return mDirtyCount > 0; }
    
    void UpdateConstant(const GraphicContext& context);

    DirectX::XMMATRIX Transform() const { return mTransform; }
    DirectX::XMMATRIX InvTransform() const { return mInvTransform; }

private:
    DirectX::XMMATRIX mTransform;
    DirectX::XMMATRIX mInvTransform;
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

    Camera* GetCamera() { return mCamera.get(); }
    const DirectionalLight& GetDirectionalLight() const { return mDirectionalLight; }
    
    std::vector<const PointLight*> GetVisiblePointLights() const { return mVisiblePointLights; }
    std::vector<const SpotLight*> GetVisibleSpotLights() const { return mVisibleSpotLights; }

    std::vector<Instance*> GetVisibleRenderInstances(
        const DirectX::BoundingFrustum& frustum,
        const DirectX::XMMATRIX& LtoW,
        const DirectX::XMVECTOR& pos,
        const DirectX::XMVECTOR& direction
        ) const;

    const SkyBox& GetSkybox() const { return mSkybox; }

private:

    bool LoadSkyBox(const nlohmann::json& sceneConfig, const GraphicContext& context);
    bool LoadLight(const nlohmann::json& sceneConfig, const GraphicContext& context);
    bool LoadCamera(const nlohmann::json& sceneConfig, const GraphicContext& context);
    bool LoadInstance(const nlohmann::json& sceneConfig, const GraphicContext& context);
    bool LoadInstance(const aiNode* node, const aiScene* scene, const GraphicContext& context);
    
    void UpdateInstanceConstant(const GraphicContext& context);
    void UpdateMaterialConstant(const GraphicContext& context);
    void UpdateLightConstant(const GraphicContext& context);

    void GenerateVisiblePointLights() ;
    void GenerateVisibleSpotLights();

private:
    std::unique_ptr<Camera> mCamera;

    // lights
    DirectionalLight mDirectionalLight;

    std::vector<PointLight> mPointLights;
    std::vector<const PointLight*> mVisiblePointLights;

    std::vector<SpotLight> mSpotLights;
    std::vector<const SpotLight*> mVisibleSpotLights;
    
    std::vector<std::unique_ptr<Instance>> mInstances;
    
    SkyBox mSkybox;
};