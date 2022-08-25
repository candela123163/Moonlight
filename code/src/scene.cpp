#include "stdafx.h"
#include "scene.h"
#include "camera.h"
#include "util.h"
#include "globals.h"
using namespace std;
using namespace DirectX;
using namespace nlohmann;

void Instance::UpdateConstant(const GraphicContext& context)
{
    if (!StillDirty())
    {
        return;
    }
    mDirtyCount--;

    ObjectConstant objectConstant;
    XMMATRIX world = XMLoadFloat4x4(&mTransform);
    XMStoreFloat4x4(&objectConstant.World, XMMatrixTranspose(world));
    XMStoreFloat4x4(&objectConstant.InverseTransposedWorld, XMMatrixInverse(get_rvalue_ptr(XMMatrixDeterminant(world)), world));
    context.frameResource->ConstantObject->CopyData(objectConstant, mInstanceID);
}

bool Scene::Load(const filesystem::path& filePath, const GraphicContext& context)
{
    ifstream file(filePath.c_str(), fstream::in);
    assert(file.good());

    json sceneConfig = json::parse(file);

    return LoadSkyBox(sceneConfig, context) &&
        LoadLight(sceneConfig, context) &&
        LoadCamera(sceneConfig, context) &&
        LoadInstance(sceneConfig, context);
}

void Scene::OnLoadOver(const GraphicContext& context) 
{
    //OnUpdate(context);
}

void Scene::OnUpdate(const GraphicContext& context)
{
    GenerateRenderInstances();
    UpdateLightShadowConstant(context);
    mCamera->UpdateConstant(context);
    UpdateInstanceConstant(context);
    UpdateMaterialConstant(context);
}

void Scene::InitCamera(FXMVECTOR pos,
    FXMVECTOR target,
    FXMVECTOR worldUp,
    float fovY, float aspect, float zNear, float zFar)
{
    mCamera = make_unique<Camera>();
    mCamera->LookAt(pos, target, worldUp);
    mCamera->SetLens(fovY, aspect, zNear, zFar);
}

void Scene::OnMouseDown(WPARAM btnState, int x, int y)
{
    
}

void Scene::OnMouseUp(WPARAM btnState, int x, int y)
{
    
}

void Scene::OnMouseMove(WPARAM btnState, int x, int y, float dx, float dy)
{
    if (btnState & MK_LBUTTON)
    {
        // Make each pixel correspond to a quarter of a degree.
        mCamera->Pitch(XMConvertToRadians(CAMERA_TOUCH_SENSITIVITY * dy));
        mCamera->RotateY(XMConvertToRadians(CAMERA_TOUCH_SENSITIVITY * dx));
    }
}

void Scene::OnKeyboardInput(const Timer& timer)
{
    const float dt = timer.DeltaTime();
    float speed = CAMERA_MOVE_SPEED;
    float distance = speed * dt;

    if (IsKeyDown('W'))
        mCamera->Walk(distance);

    if (IsKeyDown('S'))
        mCamera->Walk(-distance);

    if (IsKeyDown('A'))
        mCamera->Strafe(-distance);

    if (IsKeyDown('D'))
        mCamera->Strafe(distance);

    if (IsKeyDown('E'))
        mCamera->VerticalShift(distance);

    if (IsKeyDown('Q'))
        mCamera->VerticalShift(-distance);

}

bool Scene::LoadSkyBox(const nlohmann::json& sceneConfig, const GraphicContext& context)
{
    auto& skyboxConfig = sceneConfig["Skybox"];
    string meshName = skyboxConfig["Mesh"];
    string cubemapName = skyboxConfig["Cubemap"];

    filesystem::path instancesFilePath = Globals::ModelPath / meshName;
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(instancesFilePath.string(), aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);

    mSkybox.mesh = Mesh::GetOrLoad(0, scene, Globals::MeshContainer.Size(), context);
    mSkybox.texture = Texture::GetOrLoad(Globals::CubemapPath / cubemapName, context);

    return true;
}

bool Scene::LoadLight(const nlohmann::json& sceneConfig, const GraphicContext& context)
{
    // directional light
    auto& sunConfig = sceneConfig["Sun"];

    auto& colorConfig = sunConfig["Color"];
    mDirectionalLight.Color = XMFLOAT3(
        static_cast<float>(colorConfig[0]),
        static_cast<float>(colorConfig[1]),
        static_cast<float>(colorConfig[2])
    );

    mDirectionalLight.Intensity = static_cast<float>(sunConfig["Intensity"]);

    auto& directionConfig = sunConfig["Direction"];
    mDirectionalLight.Direction = XMFLOAT3(
        static_cast<float>(directionConfig[0]),
        static_cast<float>(directionConfig[1]),
        static_cast<float>(directionConfig[2])
    );

    for (size_t i = 0; i < MAX_CASCADE_COUNT; i++)
    {
        mDirectionalLight.ShadowMaps[i] = make_unique<RenderTexture>(
            context.device, context.descriptorHeap,
            DIRECTION_LIGHT_SHADOWMAP_RESOLUTION, DIRECTION_LIGHT_SHADOWMAP_RESOLUTION,
            1, 1, TextureDimension::Tex2D,
            RenderTextureUsage::DepthBuffer, DXGI_FORMAT_D32_FLOAT
            );
    }

    // point light
    auto& pointLightConfig = sceneConfig["PointLights"];
    auto pointLightCount = min(MAX_POINT_LIGHT_COUNT, (int)pointLightConfig.size());
    mPointLights.resize(pointLightCount);

    for (size_t i = 0; i < pointLightCount; i++)
    {
        auto& currLightConfig = pointLightConfig[i];
        PointLight pointLight;
        
        auto& colorConfig = currLightConfig["Color"];
        pointLight.Color = XMFLOAT3(
            static_cast<float>(colorConfig[0]),
            static_cast<float>(colorConfig[1]),
            static_cast<float>(colorConfig[2])
        );

        pointLight.Intensity = static_cast<float>(currLightConfig["Intensity"]);

        auto& positionConfig = currLightConfig["Position"];
        pointLight.Position = XMFLOAT3(
            static_cast<float>(positionConfig[0]),
            static_cast<float>(positionConfig[1]),
            static_cast<float>(positionConfig[2])
        );

        pointLight.Near = static_cast<float>(currLightConfig["Near"]);
        pointLight.Range = static_cast<float>(currLightConfig["Range"]);

        pointLight.ShadowMap = make_unique<RenderTexture>(
            context.device, context.descriptorHeap,
            POINT_LIGHT_SHADOWMAP_RESOLUTION, POINT_LIGHT_SHADOWMAP_RESOLUTION,
            6, 1, TextureDimension::CubeMap,
            RenderTextureUsage::DepthBuffer, DXGI_FORMAT_D32_FLOAT
            );

        mPointLights[i] = move(pointLight);
    }

    // spot light
    auto& spotLightConfig = sceneConfig["SpotLights"];
    auto spotLightCount = min(MAX_SPOT_LIGHT_COUNT, (int)spotLightConfig.size());
    mSpotLights.resize(spotLightCount);

    for (size_t i = 0; i < spotLightCount; i++)
    {
        auto& currLightConfig = spotLightConfig[i];
        SpotLight spotLight;

        auto& colorConfig = currLightConfig["Color"];
        spotLight.Color = XMFLOAT3(
            static_cast<float>(colorConfig[0]),
            static_cast<float>(colorConfig[1]),
            static_cast<float>(colorConfig[2])
        );

        spotLight.Intensity = static_cast<float>(currLightConfig["Intensity"]);

        auto& positionConfig = currLightConfig["Position"];
        spotLight.Position = XMFLOAT3(
            static_cast<float>(positionConfig[0]),
            static_cast<float>(positionConfig[1]),
            static_cast<float>(positionConfig[2])
        );

        auto& directionConfig = currLightConfig["Direction"];
        spotLight.Direction = XMFLOAT3(
            static_cast<float>(directionConfig[0]),
            static_cast<float>(directionConfig[1]),
            static_cast<float>(directionConfig[2])
        );

        spotLight.Near = static_cast<float>(currLightConfig["Near"]);
        spotLight.Range = static_cast<float>(currLightConfig["Range"]);
        spotLight.OutterAngle = DegreeToRadians(static_cast<float>(currLightConfig["OutterAngle"]));
        spotLight.InnerAngle = DegreeToRadians(static_cast<float>(currLightConfig["InnerAngle"]));

        spotLight.ShadowMap = make_unique<RenderTexture>(
            context.device, context.descriptorHeap,
            SPOT_LIGHT_SHADOWMAP_RESOLUTION, SPOT_LIGHT_SHADOWMAP_RESOLUTION,
            1, 1, TextureDimension::Tex2D,
            RenderTextureUsage::DepthBuffer, DXGI_FORMAT_D32_FLOAT
            );

        mSpotLights[i] = move(spotLight);
    }

    return true;
}

bool Scene::LoadCamera(const nlohmann::json& sceneConfig, const GraphicContext& context)
{
    // load camera
    auto& cameraConfig = sceneConfig["Camera"];
    auto& positionConfig = cameraConfig["Position"];
    XMFLOAT3 pos(
        static_cast<float>(positionConfig[0]),
        static_cast<float>(positionConfig[1]),
        static_cast<float>(positionConfig[2])
    );

    auto& targetConfig = cameraConfig["Target"];
    XMFLOAT3 target(
        static_cast<float>(targetConfig[0]),
        static_cast<float>(targetConfig[1]),
        static_cast<float>(targetConfig[2])
    );

    XMFLOAT3 up(0.0f, 1.0f, 0.0f);
    
    float fovY = DegreeToRadians(static_cast<float>(cameraConfig["FOVY"]));
    float aspect = static_cast<float>(context.screenWidth) / context.screenHeight;
    float zNear = static_cast<float>(cameraConfig["Near"]);
    float zFar = static_cast<float>(cameraConfig["Far"]);

    InitCamera(XMLoadFloat3(&pos),
        XMLoadFloat3(&target),
        XMLoadFloat3(&up),
        fovY, aspect, zNear, zFar);

    return true;
}

bool Scene::LoadInstance(const nlohmann::json& sceneConfig, const GraphicContext& context)
{
    filesystem::path instancesFilePath = Globals::ScenePath / sceneConfig["Instances"];
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(instancesFilePath.string(), aiProcess_Triangulate | aiProcess_OptimizeMeshes | aiProcess_CalcTangentSpace | aiProcess_FlipUVs);

    return LoadInstance(scene->mRootNode, scene, context);
}

bool Scene::LoadInstance(const aiNode* node, const aiScene* scene, const GraphicContext& context)
{
    XMFLOAT4X4 transform;
    const aiMatrix4x4& t = node->mTransformation;
    transform._11 = t.a1; transform._12 = t.a2; transform._13 = t.a3; transform._14 = t.a4;
    transform._21 = t.b1; transform._22 = t.b2; transform._23 = t.b3; transform._24 = t.b4;
    transform._31 = t.c1; transform._32 = t.c2; transform._33 = t.c3; transform._34 = t.c4;
    transform._41 = t.d1; transform._42 = t.d2; transform._43 = t.d3; transform._44 = t.d4;

    for (size_t i = 0; i < node->mNumMeshes; i++)
    {
        Mesh* mesh = Mesh::GetOrLoad(i, scene, Globals::MeshContainer.Size(), context);
        Material* material = Material::GetOrLoad(scene->mMeshes[i]->mMaterialIndex, scene, context);
        mInstances.push_back(
            make_unique<Instance>(i, transform, material, mesh)
        );
    }

    for (size_t i = 0; i < node->mNumChildren; i++)
    {
        if (!LoadInstance(node->mChildren[i], scene, context)) 
        {
            return false;
        }
    }
    return true;
}

void Scene::GenerateRenderInstances()
{
    mSortedInstances.resize(mInstances.size());
    for (size_t i = 0; i < mInstances.size(); i++)
    {
        mSortedInstances[i] = mInstances[i].get();
    }
}

void Scene::UpdateInstanceConstant(const GraphicContext& context)
{
    for (auto instance : mSortedInstances)
    {
        instance->UpdateConstant(context);
    }
}

void Scene::UpdateMaterialConstant(const GraphicContext& context)
{
    for (auto instance : mSortedInstances)
    {
        instance->GetMaterial()->UpdateConstant(context);
    }
}

void Scene::UpdateLightShadowConstant(const GraphicContext& context)
{
    if (!StillLightDirty())
        return;

    mLightDirtyCount--;

    LightConstant lightConstant;
    lightConstant.SunColor = mDirectionalLight.Color;
    lightConstant.SunDirection = mDirectionalLight.Direction;
    lightConstant.SunIntensity = mDirectionalLight.Intensity;

    lightConstant.PointLightCount = mPointLights.size();
    for (size_t i = 0; i < mPointLights.size(); i++)
    {
        const PointLight& light = mPointLights[i];
        lightConstant.PointLights[i] = {
            light.Color,
            light.Intensity,
            light.Position,
            1.0f / max(0.0001f,light.Range * light.Range)
        };
    }

    lightConstant.SpotLightCount = mSpotLights.size();
    for (size_t i = 0; i < mSpotLights.size(); i++)
    {
        const SpotLight& light = mSpotLights[i];
        float innerCos = cosf(light.InnerAngle);
        float outterCos = cosf(light.OutterAngle);
        float invAngleRange = 1.0f / max(innerCos - outterCos, 0.0001f);

        lightConstant.SpotLights[i] = {
            light.Color,
            1.0f / max(0.0001f, light.Range * light.Range),
            light.Position,
            invAngleRange,
            light.Direction,
            -outterCos * invAngleRange,
            light.Intensity,
            XMFLOAT3(0, 0, 0)
        };
    }

    context.frameResource->ConstantLight->CopyData(lightConstant);
}