//--------------------------------------------------------------------------------------
// Pixel Shader - Unlit plain colour
//--------------------------------------------------------------------------------------
// Renders in a single colour coming from material colour and per-mesh colour

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Pixel Shader Code
//--------------------------------------------------------------------------------------

float4 main() : SV_Target // Output is a float4 RGBA - which has a special semantic SV_Target to indicate it goes to the render target
{
	// Combines colour *and* alpha value from material diffuse colour and per-mesh colour
	return gMaterialDiffuseColour * gMeshColour;
}