//--------------------------------------------------------------------------------------
// Pixel Shader - PBR lighting (metalness/roughness) with normal mapping - 1 light source.
//--------------------------------------------------------------------------------------
// Physically based rendering using the metalness/roughness approach plus normal mapping.
// Uses albedo, roughness, normal, IBL environment map and (optional) metalness map
// Combines with diffuse colour setting from per-material constant buffer (see include file)

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// Textures - order of indexes (t0, t1 etc.) is specified in MeshTypes.h : TextureTypes
Texture2D AlbedoMap    : register(t0); // Base colour for surface - diffuse colour where non-metal, specular colour where metal
Texture2D MetalnessMap : register(t1); // Metalness map - 1 for metal, 0 for non-metal. In between values OK. Optional map - defaults to 0
Texture2D RoughnessMap : register(t2); // From 0 for smooth -> 1 for rough
Texture2D NormalMap    : register(t3); // Standard normal map

// Samplers used for above textures
SamplerState MapSampler : register(s0);

// Image-based lighting - additional texture for environment reflections, doesn't come from mesh but set globally at scene level instead
TextureCube  IBLMap     : register(t8); // A cube map is made of six internal images, but is can be sampled as one
SamplerState IBLSampler : register(s8);


//--------------------------------------------------------------------------------------
// Pixel Shader Input
//--------------------------------------------------------------------------------------

// Data coming in from the vertex shader
struct Input
{
	float4 clipPosition  : SV_Position;   // 2D position of pixel in clip space - this key field is identified by the special semantic "SV_Position" (SV = System Value)
	float3 worldPosition : worldPosition; // 3D position of pixel in world space - used for lighting
	float3 worldNormal   : worldNormal;   // The surface normal (in world space) for this pixel - used for lighting
	float3 worldTangent  : worldTangent;  // The surface tangent (in world space) for this pixel - used for normal/parallax mapping
	float2 uv            : uv;            // Texture coordinate for this pixel, used to sample textures
};


//--------------------------------------------------------------------------------------
// Pixel Shader Code
//--------------------------------------------------------------------------------------
// Pixel shader gets input from pixel shader output, see structure above
float4 main(Input input) : SV_Target // Output is a float4 RGBA - which has a special semantic SV_Target to indicate it goes to the render target
{
	/////////////////////////////
	// Parallax mapping

	// Renormalise pixel normal and tangent because interpolation from the vertex shader can introduce scaling (refer to Graphics module lecture notes)
	float3 worldNormal  = normalize(input.worldNormal);
	float3 worldTangent = normalize(input.worldTangent);

	// Create the bitangent - at right angles to normal and tangent. Then with these three axes, form the tangent matrix - the local axes for the current pixel
	float3 worldBitangent = cross(worldNormal, worldTangent);
	float3x3 tangentMatrix = float3x3(worldTangent, worldBitangent, worldNormal);

	// Sample the normal map - including conversion from 0->1 RGB values to -1 -> 1 XYZ values
	float3 tangentNormal = normalize(NormalMap.Sample(MapSampler, input.uv).xyz * 2.0f - 1.0f);

	// Convert the sampled normal from tangent space (as it was stored) into world space. This becomes the n, the world normal for the PBR equations below
	float3 n = mul(tangentNormal, tangentMatrix);


	///////////////////////
	// Sample PBR textures

    float4 albedo4 = AlbedoMap.Sample(MapSampler, input.uv).rgba;
    if (albedo4.a < 0.25f)  discard;
    float3 albedo = albedo4.rgb;

	float  roughness = RoughnessMap.Sample(MapSampler, input.uv).r;
	float  metalness = MetalnessMap.Sample(MapSampler, input.uv).r;

	// Diffuse colour from material/mesh
	float4 baseDiffuse = gMaterialDiffuseColour * gMeshColour;


	///////////////////////
	// Image-based global illumination

	// Surface normal dot view (camera) direction - don't allow it to become 0 or less
	float3 v = normalize(gCameraPosition - input.worldPosition); // Get normal to camera, called v for view vector in PBR equations
	float nDotV = max(dot(n, v), 0.001f);

	// Select specular color based on metalness
	float3 specularColour = lerp(0.04f, albedo, metalness);

	// Reflection vector for sampling the cubemap for specular reflections
	float3 r = reflect(-v, n);

	// Sample environment cubemap, use small mipmap for diffuse, use mipmap based on roughness for specular
	float3 diffuseIBL = IBLMap.SampleLevel(MapSampler, n, 9).rgb;             // Surface normal to sample diffuse environment light
	diffuseIBL = (diffuseIBL - 0.5f) * 0.333f + 0.2f;                         // Adjust contrast of environment map
	float roughnessMip = 8 * log2(roughness + 1);                             // Heuristic to convert roughness to mip-map. Rougher surfaces will use smaller (blurrier) mip-maps
	float3 specularIBL = IBLMap.SampleLevel(MapSampler, r, roughnessMip).rgb; // Reflected vector to sample specular environment light

	// Fresnel for IBL: when surface is at more of a glancing angle reflection of the scene increases
	float3 F_IBL = specularColour + (1 - specularColour) * pow(max(1.0f - nDotV, 0.0f), 5.0f);

	// Overall global illumination - rough approximation
	float3 colour = baseDiffuse.rgb * albedo *  diffuseIBL + (1 - roughness) * specularIBL * F_IBL;


	///////////////////////
	// Calculate Physically based Rendering BRDF

	float3 lightVector = gLight1Position - input.worldPosition; // Vector to light
	float  lightDistance = length(lightVector); // Distance to light
	float3 l = lightVector / lightDistance;     // Normal to light
	float3 h = normalize(l + v);                // Halfway normal is halfway between camera and light normals

	// Attenuate light colour (reduce strength based on its distance)
	float3 lc = gLight1Colour.rgb / lightDistance;

	// Various dot products used throughout
	float nDotL = max(dot(n, l), 0.001f);
	float nDotH = max(dot(n, h), 0.001f);
	float vDotH = max(dot(v, h), 0.001f);

	// Lambert diffuse
	float3 lambert = albedo / PI;

	// Microfacet specular - fresnel term
	float3 F = specularColour + (1 - specularColour) * pow(max(1.0f - vDotH, 0.0f), 5.0f);

	// Microfacet specular - normal distribution term
	float alpha = max(roughness * roughness, 2.0e-3f);
	float alpha2 = alpha * alpha;
	float nDotH2 = nDotH * nDotH;
	float dn = nDotH2 * (alpha2 - 1) + 1;
	float D = alpha2 / (PI * dn * dn);

	// Microfacet specular - geometry term
	float k = (roughness + 1);
	k = k * k / 8;
	float gV = nDotV / (nDotV * (1 - k) + k);
	float gL = nDotL / (nDotL * (1 - k) + k);
	float G = gV * gL;

	// Full brdf, diffuse + specular
	float3 brdf = baseDiffuse.rgb * lambert + F * G * D / (4 * nDotL * nDotV);

	// Accumulate punctual light equation for this light
	colour += PI * nDotL * lc * brdf;

	return float4(colour, baseDiffuse.a);
}
