//--------------------------------------------------------------------------------------
// Pixel Shader - Blinn-Phong Texture Mapping - 1 light souce
//--------------------------------------------------------------------------------------
// Blinn-Phong lighting with 1 light source and simple texture mapping. Uses diffuse and (optional) specular map
// Combines with diffuse / specular colour settings from per-material constant buffer (see include file)

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// Textures - order of indexes (t0, t1 etc.) is specified in MeshTypes.h : TextureTypes
Texture2D DiffuseMap  : register(t0);
Texture2D SpecularMap : register(t1);

// Samplers used for above textures
SamplerState DiffuseFilter  : register(s0);
SamplerState SpecularFilter : register(s1);


//--------------------------------------------------------------------------------------
// Pixel Shader Input
//--------------------------------------------------------------------------------------

// Data coming in from the vertex shader
struct Input
{
	float4 clipPosition  : SV_Position;   // 2D position of pixel in clip space - this key field is identified by the special semantic "SV_Position" (SV = System Value)
	float3 worldPosition : worldPosition; // 3D position of pixel in world space - used for lighting
	float3 worldNormal   : worldNormal;   // The surface normal (in world space) for this pixel - used for lighting
	float2 uv            : uv;            // Texture coordinate for this pixel, used to sample textures
};


//--------------------------------------------------------------------------------------
// Pixel Shader Code
//--------------------------------------------------------------------------------------

// Pixel shader gets input from pixel shader output, see structure above
float4 main(Input input) : SV_Target // Output is a float4 RGBA - which has a special semantic SV_Target to indicate it goes to the render target
{
	// Renormalise pixel normal and tangent because interpolation from the vertex shader can introduce scaling (refer to Graphics module lecture notes)
	float3 worldNormal = normalize(input.worldNormal);


	// Blinn-Phong lighting using a single light (same as 2nd year Graphics module)
	float3 lightVector = gLight1Position - input.worldPosition; // Vector to light
	float  lightDistance = length(lightVector);                   // Distance to light
	float3 lightNormal = lightVector / lightDistance;           // Normal to light
	float3 cameraNormal = normalize(gCameraPosition - input.worldPosition); // Normal to camera
	float3 halfwayNormal = normalize(cameraNormal + lightNormal);            // Halfway normal is halfway between camera and light normal - used for specular lighting

	// Attenuate light colour (reduce strength based on its distance)
	float3  attenuatedLightColour = gLight1Colour.rgb / lightDistance;

	// Diffuse lighting
	float  lightDiffuseLevel = saturate(dot(worldNormal, lightNormal));
	float3 lightDiffuseColour = attenuatedLightColour * lightDiffuseLevel;

	// Specular lighting
	float  lightSpecularLevel = pow(saturate(dot(worldNormal, halfwayNormal)), gMaterialSpecularPower);
	float3 lightSpecularColour = lightDiffuseColour * lightSpecularLevel; // Using diffuse light colour rather than plain attenuated colour - own adjustment to Blinn-Phong model


	// Material colours are taken from material colours combined with texture colours
	// Diffuse colour combines alpha from material, texture and mesh colour, and the result will be the alpha of the rendered pixel
	float4 materialDiffuseColour  = gMaterialDiffuseColour *  DiffuseMap. Sample(DiffuseFilter,  input.uv) * gMeshColour; // Allow per-mesh tint;
	float3 materialSpecularColour = gMaterialSpecularColour;// *SpecularMap.Sample(SpecularFilter, input.uv).rgb;

	// Combmine all colours, adding ambient light as part of the process
	float3 finalColour = materialDiffuseColour.rgb * (gAmbientColour + lightDiffuseColour) + materialSpecularColour * lightSpecularColour;

	return float4(finalColour, materialDiffuseColour.a);
}
