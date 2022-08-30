#pragma once
#include "stdafx.h"
#include "globals.h"

struct GraphicContext;

class Camera
{
public:
	// Get/Set world camera position.
	DirectX::XMVECTOR GetPosition() const;
	DirectX::XMFLOAT3 GetPosition3f() const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const DirectX::XMFLOAT3& v);

	// Get camera basis vectors.
	DirectX::XMVECTOR GetRight() const;
	DirectX::XMFLOAT3 GetRight3f() const;
	DirectX::XMVECTOR GetUp() const;
	DirectX::XMFLOAT3 GetUp3f() const;
	DirectX::XMVECTOR GetLook() const;
	DirectX::XMFLOAT3 GetLook3f() const;

	// Get frustum properties.
	float GetNearZ() const;
	float GetFarZ() const;
	float GetAspect() const;
	float GetFovY() const;
	float GetFovX() const;

	// Get near and far plane dimensions in view space coordinates.
	float GetNearWindowWidth() const;
	float GetNearWindowHeight() const;
	float GetFarWindowWidth() const;
	float GetFarWindowHeight() const;

	// Set frustum.
	void SetLens(float fovY, float aspect, float zn, float zf);

	// Define camera space via LookAt parameters.
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

	// Get View/Proj matrices.
	DirectX::XMMATRIX GetView() const;
	DirectX::XMMATRIX GetProj() const;
	DirectX::XMMATRIX GetInvView() const;
	DirectX::BoundingFrustum GetFrustum() const;

	// Strafe/Walk the camera a distance d.
	void Strafe(float d);
	void Walk(float d);
	void VerticalShift(float d);

	// Rotate the camera.
	void Pitch(float angle);
	void RotateY(float angle);

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix();

	// determine whether to update camera constant
	bool StillDirty() const { return mDirtyCount > 0; }

	void MarkConstantDirty() { mDirtyCount = FRAME_COUNT; }

	void UpdateConstant(const GraphicContext& context);

	// shadow related
	void SetShadowMaxDistance(float distance);
	void SetShadowCascadeRatio(std::array<float, MAX_CASCADE_COUNT> cascadeRatio);

	float GetShadowMaxDistance() const { return mMaxShadowDistance; }
	std::array<float, MAX_CASCADE_COUNT> GetShadowCascadeDistance() const { return mShadowCascadeDistance; }
	DirectX::BoundingFrustum GetShadowCullFrustum() const { return mShadowCullFrustum; }

private:
	void UpdateShadowConfig();

private:
	// Camera coordinate system with coordinates relative to world space.
	DirectX::XMFLOAT3 mPosition = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mRight = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 mUp = { 0.0f, 1.0f, 0.0f };
	DirectX::XMFLOAT3 mLook = { 0.0f, 0.0f, 1.0f };

	// Cache frustum properties.
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	// Cache View/Proj matrices.
	DirectX::XMMATRIX mView;
	DirectX::XMMATRIX mInvView;
	
	DirectX::XMMATRIX mProj;
	DirectX::BoundingFrustum mFrustum;

	// determine whether to update constant buffer
	UINT mDirtyCount = FRAME_COUNT;

	// shadow related
	float mMaxShadowDistance = 10.0f;
	std::array<float, MAX_CASCADE_COUNT> mShadowCascadeRatio;
	std::array<float, MAX_CASCADE_COUNT> mShadowCascadeDistance;
	DirectX::BoundingFrustum mShadowCullFrustum;
};