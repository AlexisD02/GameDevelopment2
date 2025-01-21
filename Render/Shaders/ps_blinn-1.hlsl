//--------------------------------------------------------------------------------------
// Pixel Shader - Blinn-Phong with no textures, 1 light source
//--------------------------------------------------------------------------------------
// Blinn-Phong lighting with 1 light source and plain colours (no textures) taken from 
// diffuse / specular colour settings from per-material constant buffer (see include file)

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Pixel Shader Input
//--------------------------------------------------------------------------------------

// Data coming in from the vertex shader
struct Input
{
	float4 clipPosition  : SV_Position;   // 2D position of pixel in clip space - this key field is identified by the special semantic "SV_Position" (SV = System Value)
	float3 worldPosition : worldPosition; // 3D position of pixel in world space - used for lighting
	float3 worldNormal   : worldNormal;   // The surface normal (in world space) for this pixel - used for lighting
};


//--------------------------------------------------------------------------------------
// Pixel Shader Code
//--------------------------------------------------------------------------------------

// Pixel shader gets input from pixel shader output, see structure above
float4 main(Input input) : SV_Target // Output is a float4 RGBA - which has a special semantic SV_Target to indicate it goes to the render target
{
	// Renormalise pixel normal because interpolation from the vertex shader can introduce scaling (refer to Graphics module lecture notes)
	float3 worldNormal = normalize(input.worldNormal);

	// Blinn-Phong lighting using a single light (same as 2nd year Graphics module)
	float3 lightVector   = gLight1Position - input.worldPosition; // Vector to light
	float  lightDistance = length(lightVector);                   // Distance to light
	float3 lightNormal   = lightVector / lightDistance;           // Normal to light
	float3 cameraNormal  = normalize(gCameraPosition - input.worldPosition); // Normal to camera
	float3 halfwayNormal = normalize(cameraNormal + lightNormal);            // Halfway normal is halfway between camera and light normal - used for specular lighting

	// Attenuate light colour (reduce strength based on its distance)
	float3  attenuatedLightColour = gLight1Colour.rgb / lightDistance;

	// Diffuse lighting
	float  lightDiffuseLevel = saturate(dot(worldNormal, lightNormal));
	float3 lightDiffuseColour = attenuatedLightColour * lightDiffuseLevel;

	// Specular lighting
	float  lightSpecularLevel = pow(saturate(dot(worldNormal, halfwayNormal)), gMaterialSpecularPower);
	float3 lightSpecularColour = lightDiffuseColour * lightSpecularLevel; // Using diffuse light colour rather than plain attenuated colour - own adjustment to Blinn-Phong model


	// Diffuse colour combines material colour with per-mesh colour - alpha will also be combined here and used for output pixel alpha
	float4 materialDiffuseColour  = gMaterialDiffuseColour * gMeshColour;

	// Combmine all colours, adding ambient light as part of the process
	float3 finalColour = materialDiffuseColour.rgb * (gAmbientColour + lightDiffuseColour) + gMaterialSpecularColour * lightSpecularColour;

	return float4(finalColour, materialDiffuseColour.a);
}
