#ifndef _FULLSCREEN_VS_INCLUDE
#define _FULLSCREEN_VS_INCLUDE

struct PostProc_VSOut
{
    float4 pos : SV_Position;
    float2 uv : TEXCOORD0;
    float3 ray : VAR_RAY;
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
    
#ifdef INV_PROJECT
    // Transform quad corners to view space near plane.
	// Point(view) * Matrix(project) = Point(homogenous)
	// Point(NDC) = Point(homogenous) / Point(homogenous).w
	// Point(view) = Point(homogenous) * Matrix(inv_project)
	//			   = Point(NDC) * Matrix(inv_project) * Point(homogenous).w
	// ph = Point(view) / Point(homogenous).w = Point(NDC) * Matrix(inv_project)
	// ph.w = 1 / Point(homogenous).w
	// Point(view) = ph * Point(homogenous).w = ph / ph.w

    float4 ph = mul(output.pos, INV_PROJECT);
    output.ray = ph.xyz / ph.w;
#endif
    
    return output;
}

#endif