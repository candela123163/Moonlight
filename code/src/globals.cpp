#include "stdafx.h"
#include "globals.h"
#include "material.h"
#include "mesh.h"
#include "texture.h"

namespace Globals
{
	const std::array<const CD3DX12_STATIC_SAMPLER_DESC, 7> StaticSamplers =
	{
		CD3DX12_STATIC_SAMPLER_DESC(
			0,
			D3D12_FILTER_MIN_MAG_MIP_POINT,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP),

		CD3DX12_STATIC_SAMPLER_DESC(
			1,
			D3D12_FILTER_MIN_MAG_MIP_POINT,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP),

		CD3DX12_STATIC_SAMPLER_DESC(
			2, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP),

		CD3DX12_STATIC_SAMPLER_DESC(
			3, // shaderRegister
			D3D12_FILTER_MIN_MAG_MIP_LINEAR,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP),

		CD3DX12_STATIC_SAMPLER_DESC(
			4, // shaderRegister
			D3D12_FILTER_ANISOTROPIC,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			0.0f,
			8),

		CD3DX12_STATIC_SAMPLER_DESC(
			5,
			D3D12_FILTER_ANISOTROPIC,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
			0.0f,
			8),

		CD3DX12_STATIC_SAMPLER_DESC(
			6,
			D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			D3D12_TEXTURE_ADDRESS_MODE_BORDER,
			0.0f,
			16,
			D3D12_COMPARISON_FUNC_LESS_EQUAL,
			D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)
	};

	std::filesystem::path ShaderPath("code/shader");
	std::filesystem::path AssetPath("asset");
	std::filesystem::path ImagePath(AssetPath / "images");
	std::filesystem::path CubemapPath(AssetPath / "cubemaps");
	std::filesystem::path ScenePath(AssetPath / "scenes");
	std::filesystem::path ModelPath(AssetPath / "models");

	ResourceContainer<Texture> TextureContainer;
	ResourceContainer<Mesh> MeshContainer;
	ResourceContainer<Material> MaterialContainer;
	ResourceContainer<RenderTexture> RenderTextureContainer;
}