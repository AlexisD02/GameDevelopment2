//--------------------------------------------------------------------------------------
// Texture related types
//--------------------------------------------------------------------------------------

#ifndef _TEXTURE_TYPES_H_INCLUDED_
#define _TEXTURE_TYPES_H_INCLUDED_

#include <string>


//--------------------------------------------------------------------------------------
// Textures
//--------------------------------------------------------------------------------------

// There are four rendering approaches supported, each needing different texture types: 
// - Unlit rendering -  plain colour objects (all one colour) or plain texture objects (e.g. skybox - doesn't receive lighting)
// - Blinn-Phong rendering (as in 2nd year labs, where we used simple lighting equations for diffuse + specular lighting)
// - Physically-Based-Rendering (PBR) with metalness/roughness textures
// - Physically-Based-Rendering (PBR) with specular/glossiness textures
//
// Each texture type is given a specific slot in the shaders: the values in the enum below
// Slots have been arranged so the different rendering approaches above share slots where possible
// Not supporting situations where two maps are stored in a single texture (e.g. diffuse+specular map, normal+height map, metalness+roughness map),
// so each texture must be standalone in this app
enum class TextureType
{
	// Blinn-Phong rendering approach (like 2nd year Graphics labs)
	Diffuse       = 0, // Basic colour of object, affected by diffuse light, also used for plain texture rendering
	Specular      = 1, // Shininess of object, affected by specular light
	SpecularPower = 2, // We never stored this in a texture in previous labs but it is used sometimes: the exponent used in the specular equation
	Normal        = 3, // For normal mapping
	Displacement  = 4, // aka Height, for parallax mapping or tesellation displacement

	// PBR rendering (metalness/roughness approach)
	Albedo    = 0, // Base colour of object
	Metalness = 1, // 0 = non-metal, 1 = metal, values inbetween supported
	Roughness = 2, // 0 = smooth to 1 = rough. Same as (1 - Gloss map)
	// Slots 3 and 4 will be normal and displacement

	// PBR rendering (specular/glossiness approach)
	// Slot 0 will be albedo/diffuse
	// Slot 1 will be specular
	Gloss = 2, // 0 = rough to 1 = smooth. Same as (1 - Roughness map)
	// Slots 3 and 4 will be normal and displacement

	// Optional maps
	AO       = 5, // Ambient occlusion map - 0->1 values, recessed areas get lower values, affects ambient light
	Cavity   = 6, // Cavity map - 0->1 values, highly recessed areas get lower values, affects direct light
	Emissive = 7, // Emissive map is colour *emitted* by object. Added to final calculated colour value

	Environment = 8, // Environment map is reflected in shiny surfaces in PBR rendering, should be a cube map (load from a DDS file)

	Unknown,
};
const int NUM_TEXTURE_TYPES = 8; // Total number of slots used in enum above - maximum number of texture slots that will be used in shaders


//--------------------------------------------------------------------------------------
// Samplers
//--------------------------------------------------------------------------------------

// How to filter large/small textures
enum class TextureFilter
{
	FilterPoint,
	FilterBilinear,
	FilterTrilinear,
	FilterAnisotropic,
};

// How to access UVs outside the 0->1 range
enum class TextureAddressingMode
{
	AddressingWrap,
	AddressingMirror,
	AddressingClamp,
	AddressingBorder,
};

// A sampler describes how a specific texture is accessed in a shader. Whenever a texture is used in a
// shader, a sampler must also have been set to go with it
struct SamplerState
{
	TextureFilter         filter;
	TextureAddressingMode addressingMode;
};

// Less-than operator for SamplerState objects
// The TextureManager class  uses a map of SamplerStates to DirectX samplers. However, std::map requires that
// the key (SamplerState) has a less-than operator. This is because std::map keeps its contents ordered by key.
// There are several map objects in this app but they all have keys with less-than already defined (e.g. string)
// SamplerState doesn't have a < operator so we define one here so we can use it as the key in a map.
// 
// An alternative would be to use std::unordered_map, which doesn't need < but does need == so provides no great benefit
// (Practically we don't need the ordering in any of the maps in this app and unordered_map might be the more logical
// choice, however it's a marginal call for our purposes and map reads more nicely...)
inline bool operator<(const SamplerState& a, const SamplerState& b)
{
	if      (a.filter < b.filter) return true;
	else if (a.filter > b.filter) return false;
	else return (a.addressingMode < b.addressingMode);
}


//--------------------------------------------------------------------------------------
// Other
//--------------------------------------------------------------------------------------

// Collated data to describe a texture used in a submesh
struct TextureDesc
{
	TextureType  type = TextureType::Unknown;
	std::string  filename;
	SamplerState samplerState;
};


#endif //_TEXTURE_TYPES_H_INCLUDED_
