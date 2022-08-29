#pragma once
#include "stdafx.h"

class Texture;
class RenderTexture;
class Mesh;
class Material;

namespace Globals 
{
    // window related
    #define WINDOW_WIDTH 1400
    #define WINDOW_HEIGHT 800
    #define WINDOW_TITLE L"Tiny Renderer"
    #define SWAP_CHAIN_COUNT 2
    #define FRAME_COUNT 3

    // heap related
    #define HEAP_COUNT_RTV 256
    #define HEAP_COUNT_DSV 256
    #define HEAP_COUNT_SRV 256

    // constant buffer max size
    #define OBJECT_MAX_SIZE 128
    #define MATERIAL_MAX_SIZE 128

    // camera related
    #define CAMERA_MOVE_SPEED 3.0f
    #define CAMERA_TOUCH_SENSITIVITY 0.1f

    // light related
    #define MAX_POINT_LIGHT_COUNT 4
    #define MAX_SPOT_LIGHT_COUNT 4
    #define POINT_LIGHT_SHADOWMAP_RESOLUTION 512
    #define SPOT_LIGHT_SHADOWMAP_RESOLUTION 512
    #define DIRECTION_LIGHT_SHADOWMAP_RESOLUTION 1024
    #define MAX_CASCADE_COUNT 4
    
    // dummy texture
    #define DUMMY_TEXTURE "white.png"

    // static samplers
    extern const std::array<const CD3DX12_STATIC_SAMPLER_DESC, 8> StaticSamplers;

    // path related
    extern std::filesystem::path ShaderPath;
    extern std::filesystem::path AssetPath;
    extern std::filesystem::path ImagePath;
    extern std::filesystem::path CubemapPath;
    extern std::filesystem::path ScenePath;
    extern std::filesystem::path ModelPath;

    // resources
    extern ResourceContainer<Texture> TextureContainer;
    extern ResourceContainer<Mesh> MeshContainer;
    extern ResourceContainer<Material> MaterialContainer;
    extern ResourceContainer<RenderTexture> RenderTextureContainer;
}