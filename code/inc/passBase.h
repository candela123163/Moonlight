#pragma once

struct GraphicContext;

class PassBase
{
public:
    // ------------------------------------
    // -Initialize
    //      -Load Resource Start
    //      *PreparePass
    //      -Load Resource Over
    //      *PreprocessPass
    // 
    // -GameLoop
    //      -Handle Input              
    //      -Scene Update   
    //      *DrawPass
    //      
    // -End
    //      *ReleasePass
    //      -Release Resource 

    virtual void PreparePass(const GraphicContext& context) = 0;
    virtual void PreprocessPass(const GraphicContext& context) = 0;
    virtual void DrawPass(const GraphicContext& context) = 0;
    virtual void ReleasePass(const GraphicContext& context) = 0;
};