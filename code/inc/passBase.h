#pragma once

struct GraphicContext;

class PassBase
{
public:
    virtual void PreparePass(const GraphicContext& context) = 0;
    virtual void DrawPass(const GraphicContext& context) = 0;
    virtual void ReleasePass(const GraphicContext& context) = 0;
};