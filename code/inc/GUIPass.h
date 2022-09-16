#pragma once
#include "stdafx.h"
#include "passBase.h"
#include "util.h"
using Microsoft::WRL::ComPtr;

class GUIPass final : public PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) override;
    virtual void PreprocessPass(const GraphicContext& context) override;
    virtual void DrawPass(const GraphicContext& context) override;
    virtual void ReleasePass(const GraphicContext& context) override;



};