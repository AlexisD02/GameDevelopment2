//--------------------------------------------------------------------------------------
// Pixel Shader - Unlit texture
//--------------------------------------------------------------------------------------
// Renders using a single texture with no lighting effects, combines with material and per-mesh colour

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// Textures - order of indexes (t0, t1 etc.) is specified in MeshTypes.h : TextureTypes
Texture2D DiffuseMap : register(t0);

// Samplers used for above textures
SamplerState DiffuseFilter : register(s0);


//--------------------------------------------------------------------------------------
// Pixel Shader Input
//--------------------------------------------------------------------------------------

// Data coming in from the vertex shader
struct Input
{
	float4 clipPosition  : SV_Position;   // 2D position of pixel in clip space - this key field is identified by the special semantic "SV_Position" (SV = System Value)
	float2 uv            : uv;            // Texture coordinate for this pixel, used to sample textures
};


//--------------------------------------------------------------------------------------
// Pixel Shader Code
//--------------------------------------------------------------------------------------

// Pixel shader gets input from pixel shader output, see structure above
float4 main(Input input) : SV_Target // Output is a float4 RGBA - which has a special semantic SV_Target to indicate it goes to the render target
{
	// Combines colour *and* alpha value from material diffuse colour, diffuse map and per-mesh colour
	return gMaterialDiffuseColour * DiffuseMap.Sample(DiffuseFilter, input.uv) * gMeshColour;
}
