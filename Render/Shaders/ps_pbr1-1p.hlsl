//--------------------------------------------------------------------------------------
// Pixel Shader - PBR lighting (metalness/roughness) with parallax mapping - 1 light source.
//--------------------------------------------------------------------------------------
// Physically based rendering using the metalness/roughness approach plus parallax mapping.
// Uses albedo, roughness, normal, displacement, IBL environment map and (optional) metalness map
// Combines with diffuse colour setting from per-material constant buffer (see include file)

#include "Common.hlsli"


//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// Textures - order of indexes (t0, t1 etc.) is specified in MeshTypes.h : TextureTypes
Texture2D AlbedoMap       : register(t0); // Base colour for surface - diffuse colour where non-metal, specular colour where metal
Texture2D MetalnessMap    : register(t1); // Metalness map - 1 for metal, 0 for non-metal. In between values OK. Optional map - defaults to 0
Texture2D RoughnessMap    : register(t2); // From 0 for smooth -> 1 for rough
Texture2D NormalMap       : register(t3); // Standard normal map
Texture2D DisplacementMap : register(t4); // Standard displacement (or height) map

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

	// Prepare for parallax mapping
	float3 v = normalize(gCameraPosition - input.worldPosition); // Get normal to camera, called v for view vector in PBR equations
	float3 vt = mul(tangentMatrix, v);                           // Transform camera normal into tangent space (so it is local to texture)
    float2 uv = input.uv;

	/////////////////////////////
    // Linear search for parallax occlusion mapping

    // Viewing ray descends at angle through the height map layer. Take several samples of the height map along this
    // section to find where the ray intersects the texture surface. The ray starts at the top of the layer, above the
    // surface, step it along by increments until the ray goes below the surface. This initial linear search to finds
    // the rough intersection, the last two points (one above, one below the surface) are then used to refine the search

    // Determine number of samples based on viewing angle. A more shallow angle needs more samples
    const float minSamples = 5;
    const float maxSamples = 20;
    float numSamples = lerp(maxSamples, minSamples, abs(vt.z)); // The view vector is in tangent space, so its z value indicates
                                                                // how much it is pointing directly away from the polygon

    // For each step along the ray direction, find the amount to move the UVs and the amount to descend in the height layer
    float rayHeight = 0.5f; // Current height of ray, 0->1 in the height map layer. Start at the top of the layer
    float heightStep = 1.0 / numSamples; // Amount the ray descends for each step
    float2 uvStep = (gParallaxDepth * vt.xy / vt.z) / numSamples; // Ray UV offset for each step. Can also remove the / v.z here
                                                                  // to add limiting, which will reduce artefacts at glancing angles
                                                                  // but will also reduce the depth at those angles

    // Sample height map at intial UVs (top of layer)
    float surfaceHeight = DisplacementMap.Sample(MapSampler, uv).r;
    float prevSurfaceHeight = surfaceHeight;

    // Technical point: when sampling a texture DirectX needs the rate of change of the U and V coordinates (called the x and y
    // gradients). This is used to select a mip-map - if the UVs are changing a lot between each pixel then the texture must be
    // far away and so a small mip-map is chosen. However, you cannot use the gradient values in a loop (unless it can be unrolled).
    // So normal texture sampling often cannot be used in a loop. However we can fetch the gradient values before the loop (these
    // two lines) then use the SampleGrad function, where we can pass them as parameters.
    float2 dx = ddx(uv);
    float2 dy = ddy(uv);

    // While ray is above the surface
    while (rayHeight > surfaceHeight)
    {
        // Make short step along ray - move UVs and descend ray height
        rayHeight -= heightStep;
        uv -= uvStep;

        // Sample height map again
        prevSurfaceHeight = surfaceHeight;
        surfaceHeight = DisplacementMap.SampleGrad(MapSampler, uv, dx, dy).r;
    }


	/////////////////////////////
    // Parallax occulusion mapping

    // Final linear interpolation between last two points in linear search

    // Calculate how much the current step is below surface, and how much previous step was above the surface
    float currDiff = surfaceHeight - rayHeight;
    float prevDiff = (rayHeight + heightStep) - prevSurfaceHeight;

    // Use linear interpolation to estimate how far back to retrace the previous step to the instersection with the surface
    float weight = currDiff / (currDiff + prevDiff); // 0->1 value, how much to backtrack

    // Final interpolation of UVs
    uv += uvStep * weight;


	///////////////////////
	// Get normal from normal map

	// Sample the normal map - including conversion from 0->1 RGB values to -1 -> 1 XYZ values
	float3 tangentNormal = normalize(NormalMap.Sample(MapSampler, uv).xyz * 2.0f - 1.0f);

	// Convert the sampled normal from tangent space (as it was stored) into world space. This becomes the n, the world normal for the PBR equations below
	float3 n = mul(tangentNormal, tangentMatrix);


	///////////////////////
	// Sample PBR textures

	float4 albedo4 = AlbedoMap.Sample(MapSampler, uv).rgba;
    if (albedo4.a < 0.25f)  discard;
    float3 albedo = albedo4.rgb;

	float  roughness = RoughnessMap.Sample(MapSampler, uv).r;
	float  metalness = MetalnessMap.Sample(MapSampler, uv).r;

	// Diffuse colour from material/mesh
	float4 baseDiffuse = gMaterialDiffuseColour * gMeshColour;


	///////////////////////
	// Image-based global illumination

	// Surface normal dot view (camera) direction - don't allow it to become 0 or less
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
	float3 colour = baseDiffuse.rgb * albedo * diffuseIBL + (1 - roughness) * specularIBL * F_IBL;


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
