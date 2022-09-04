#ifndef _FULLSCREEN_VS_INCLUDE
#define _FULLSCREEN_VS_INCLUDE

struct PostProc_VSOut
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
};

//----------------------------------------------------------------------------------
// Vertex shader that generates a full-screen triangle with texcoords
// Assuming a Draw(3,0) call.
//----------------------------------------------------------------------------------
PostProc_VSOut FullScreenTriangle_VS(uint VertexId : SV_VertexID)
{
    PostProc_VSOut output = (PostProc_VSOut) 0.0f;
    output.uv = float2((VertexId << 1) & 2, VertexId & 2);
    output.pos = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
#ifdef REVERSE_Z
    output.pos.z = 1.0f;
#endif
    return output;
}

#endif