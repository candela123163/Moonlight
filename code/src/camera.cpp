#include "stdafx.h"
#include "camera.h"
#include "frameResource.h"
#include "renderOption.h"


using namespace DirectX;
using namespace std;

XMVECTOR Camera::GetPosition() const
{
	return XMVectorSetW(XMLoadFloat3(&mPosition), 1.0f);
}

XMFLOAT3 Camera::GetPosition3f() const
{
	return mPosition;
}

void Camera::SetPosition(float x, float y, float z)
{
	mPosition = XMFLOAT3(x, y, z);
	mViewDirty = true;
}

void Camera::SetPosition(const XMFLOAT3& v)
{
	mPosition = v;
	mViewDirty = true;
}

XMVECTOR Camera::GetRight() const
{
	return XMVector3Normalize(XMLoadFloat3(&mRight));
}

XMFLOAT3 Camera::GetRight3f() const
{
	return mRight;
}

XMVECTOR Camera::GetUp() const
{
	return XMVector3Normalize(XMLoadFloat3(&mUp));
}

XMFLOAT3 Camera::GetUp3f() const
{
	return mUp;
}

XMVECTOR Camera::GetLook() const
{
	return XMVector3Normalize(XMLoadFloat3(&mLook));
}

XMFLOAT3 Camera::GetLook3f() const
{
	return mLook;
}

float Camera::GetNearZ() const
{
	return mNearZ;
}

float Camera::GetFarZ() const
{
	return mFarZ;
}

float Camera::GetAspect() const
{
	return mAspect;
}

float Camera::GetFovY() const
{
	return mFovY;
}

float Camera::GetFovX() const
{
	float halfWidth = 0.5f * GetNearWindowWidth();
	return 2.0f * atan(halfWidth / mNearZ);
}

float Camera::GetNearWindowWidth() const
{
	return mAspect * mNearWindowHeight;
}

float Camera::GetNearWindowHeight() const
{
	return mNearWindowHeight;
}

float Camera::GetFarWindowWidth() const
{
	return mAspect * mFarWindowHeight;
}

float Camera::GetFarWindowHeight()const
{
	return mFarWindowHeight;
}

void Camera::SetLens(float fovY, float aspect, float zn, float zf)
{
	// cache properties
	mFovY = fovY;
	mAspect = aspect;
	mNearZ = zn;
	mFarZ = zf;

	mNearWindowHeight = 2.0f * mNearZ * tanf(0.5f * mFovY);
	mFarWindowHeight = 2.0f * mFarZ * tanf(0.5f * mFovY);
	
	mProj = XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mFarZ);
	BoundingFrustum::CreateFromMatrix(mFrustum, mProj);

#ifdef REVERSE_Z
	mProj = XMMatrixPerspectiveFovLH(mFovY, mAspect, mFarZ, mNearZ);
#endif

}

void Camera::LookAt(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR worldUp)
{
	XMVECTOR L = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR R = XMVector3Normalize(XMVector3Cross(worldUp, L));
	XMVECTOR U = XMVector3Cross(L, R);

	XMStoreFloat3(&mPosition, pos);
	XMStoreFloat3(&mLook, L);
	XMStoreFloat3(&mRight, R);
	XMStoreFloat3(&mUp, U);

	mViewDirty = true;
}

void Camera::LookAt(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up)
{
	XMVECTOR P = XMLoadFloat3(&pos);
	XMVECTOR T = XMLoadFloat3(&target);
	XMVECTOR U = XMLoadFloat3(&up);

	LookAt(P, T, U);

	mViewDirty = true;
}

XMMATRIX Camera::GetView() const
{
	assert(!mViewDirty);
	return mView;
}

XMMATRIX Camera::GetProj() const
{
	return mProj;
}

XMMATRIX Camera::GetInvView() const 
{
	assert(!mViewDirty);
	return mInvView;
}

BoundingFrustum Camera::GetFrustum() const
{
	return mFrustum;
}

void Camera::Strafe(float d)
{
	// mPosition += d*mRight
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR r = XMLoadFloat3(&mRight);
	XMVECTOR p = XMLoadFloat3(&mPosition);
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(s, r, p));

	mViewDirty = true;
}

void Camera::Walk(float d)
{
	// mPosition += d*mLook
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR l = XMLoadFloat3(&mLook);
	XMVECTOR p = XMLoadFloat3(&mPosition);
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(s, l, p));

	mViewDirty = true;
}

void Camera::VerticalShift(float d)
{
	// mPosition += d*mUp
	XMVECTOR s = XMVectorReplicate(d);
	XMVECTOR up = XMLoadFloat3(&mUp);
	XMVECTOR p = XMLoadFloat3(&mPosition);
	XMStoreFloat3(&mPosition, XMVectorMultiplyAdd(s, up, p));

	mViewDirty = true;
}

void Camera::Pitch(float angle)
{
	// Rotate up and look vector about the right vector.

	XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mRight), angle);

	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));

	mViewDirty = true;
}

void Camera::RotateY(float angle)
{
	// Rotate the basis vectors about the world y-axis.

	XMMATRIX R = XMMatrixRotationY(angle);

	XMStoreFloat3(&mRight, XMVector3TransformNormal(XMLoadFloat3(&mRight), R));
	XMStoreFloat3(&mUp, XMVector3TransformNormal(XMLoadFloat3(&mUp), R));
	XMStoreFloat3(&mLook, XMVector3TransformNormal(XMLoadFloat3(&mLook), R));

	mViewDirty = true;
}

void Camera::UpdateViewMatrix()
{
	if (mViewDirty)
	{
		mViewDirty = false;

		XMVECTOR U = XMLoadFloat3(&mUp);
		XMVECTOR L = XMLoadFloat3(&mLook);
		XMVECTOR P = XMLoadFloat3(&mPosition);

		mView = XMMatrixLookToLH(P, L, U);
		XMVECTOR determinant;
		mInvView = XMMatrixInverse(&determinant, mView);
	}
}

void Camera::UpdateConstant(const GraphicContext& context)
{
	UpdateViewMatrix();

	CameraConstant cameraConstant;

	XMMATRIX view = GetView();
	XMMATRIX unjitteredProj = GetProj();

	// jitter
	int jitterIndex = 1 + (context.frameCount % mJitterPeriod);
	XMFLOAT2 jitter = XMFLOAT2(0.0f, 0.0f);
	if(context.renderOption->TAAEnable){
		jitter = XMFLOAT2(
			(GetHalton(jitterIndex, 2) - 0.5) / context.screenWidth,
			(GetHalton(jitterIndex, 3) - 0.5) / context.screenHeight
		);
	}
	
	XMMATRIX proj = unjitteredProj;
	proj.r[2].m128_f32[0] = jitter.x * 2.0;
	proj.r[2].m128_f32[1] = jitter.y * -2.0;

	XMVECTOR determinant;
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMMATRIX invView = XMMatrixInverse(&determinant, view);
	XMMATRIX invProj = XMMatrixInverse(&determinant, proj);
	XMMATRIX invViewProj = XMMatrixInverse(&determinant, viewProj);
	
	XMMATRIX unjitteredViewProj = XMMatrixMultiply(view, unjitteredProj);

	if (context.frameCount == 0) {
		mUnjitteredPreViewProj = unjitteredViewProj;
	}
	

	XMStoreFloat4x4(&cameraConstant.View, XMMatrixTranspose(view));
	XMStoreFloat4x4(&cameraConstant.Proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&cameraConstant.InvView, XMMatrixTranspose(invView));
	XMStoreFloat4x4(&cameraConstant.InvProj, XMMatrixTranspose(invProj));
	XMStoreFloat4x4(&cameraConstant.ViewProj, XMMatrixTranspose(viewProj));
	XMStoreFloat4x4(&cameraConstant.InvViewProj, XMMatrixTranspose(invViewProj));

	cameraConstant.EyePosW = XMFLOAT4(mPosition.x, mPosition.y, mPosition.z, 1.0f);
	cameraConstant.NearFar = XMFLOAT4(mNearZ, mFarZ, 0.0, 0.0);

	XMStoreFloat4x4(&cameraConstant.UnjitteredViewProj, XMMatrixTranspose(unjitteredViewProj));
	XMStoreFloat4x4(&cameraConstant.UnjitteredPreViewProj, XMMatrixTranspose(mUnjitteredPreViewProj));
	cameraConstant.Jitter = jitter;

	context.frameResource->ConstantCamera->CopyData(cameraConstant);

	// update last frame matrix
	mUnjitteredPreViewProj = unjitteredViewProj;
}

void Camera::SetShadowMaxDistance(float distance)
{
	mMaxShadowDistance = distance;
}

void Camera::SetShadowCascadeRatio(std::array<float, MAX_CASCADE_COUNT> cascadeRatio, int cascadeCount)
{
	mCascadeCount = min(MAX_CASCADE_COUNT, cascadeCount);
	mShadowCascadeRatio = cascadeRatio;
}

void Camera::UpdateShadowConfig()
{
	mMaxShadowDistance = min(mMaxShadowDistance, mFarZ);
	BoundingFrustum::CreateFromMatrix(mShadowCullFrustum, XMMatrixPerspectiveFovLH(mFovY, mAspect, mNearZ, mMaxShadowDistance));
	for (size_t i = 0; i < mCascadeCount - 1; i++)
	{
		mShadowCascadeDistance[i] = mMaxShadowDistance * mShadowCascadeRatio[i];
	}
	mShadowCascadeDistance[0] = max(mNearZ + 1.0f, mShadowCascadeDistance[0]);
	mShadowCascadeDistance[mCascadeCount - 1] = mMaxShadowDistance;
}

array<XMVECTOR, 4> Camera::GetFrustumPlaneConers(float distance) const
{
	XMVECTOR pos = GetPosition();
	XMVECTOR up = GetUp();
	XMVECTOR forward = GetLook();
	XMVECTOR right = GetRight();
	float upLength = distance * tan(mFovY * 0.5);
	float rightLength = upLength * mAspect;
	XMVECTOR center = pos + distance * forward;
	XMVECTOR upVector = up * upLength;
	XMVECTOR rightVector = right * rightLength;

	array<XMVECTOR, 4> corners;
	corners[0] = center - rightVector + upVector;
	corners[1] = center + rightVector + upVector;
	corners[2] = center + rightVector - upVector;
	corners[3] = center - rightVector - upVector;

	return corners;
}