//--------------------------------------------------------------------------------------
// RenderMethod-related types and the RenderState class
//--------------------------------------------------------------------------------------
// Render methods describe the process required to render a given material
// A render state is the DirectX objects and states that need to be set to support a given render method
// 
// Each submesh in a mesh holds a single RenderState, a combination of vertex shader, pixel shader, textures and constants.
// Enable a particular RenderState object on the GPU by calling its Apply method, which will select shaders, attach textures
// to them etc. Subsequent rendering will now use the appropriate render state.
// 
// RenderState objects are constructed from RenderMethod objects. A RenderMethod is a readable description of the processes
// and textures/constants needed to render a material. RenderMethods are created during the import process (see Mesh.h/cpp)
// by looking at the materials used by submeshes in the 3D files. 

#ifndef _RENDER_METHOD_H_INCLUDED_
#define _RENDER_METHOD_H_INCLUDED_

#include "MeshTypes.h"
#include "CBufferTypes.h"
#include "TextureTypes.h"

#define NOMINMAX // Use NOMINMAX to stop certain Microsoft headers defining "min" and "max", which breaks some libraries (e.g. assimp)
#include <d3d11.h>
#include <array>


//--------------------------------------------------------------------------------------
// Render methods
//--------------------------------------------------------------------------------------

// Geometry rendering method determines how the vertices of a mesh will be processed
enum class GeometryRenderMethod
{
	Static,  // Vertices are already in world space, no world matrix required
	Rigid,   // Mesh is control by a single matrix (which may be part of a hierarchy of matrices in a rigid animated mesh)
	Skinned, // Mesh is controlled by influencing bones (geometry requires per-vertex bone indexes and weights)
	Unknown,
};


// Surface rendering method determines how the pixels of a mesh will be rendered: what lighting model is used, and by implication what textures are needed
// There are four approaches supported: 
// - Unlit rendering -  plain colour objects (all one colour) or plain texture objects (e.g. skybox - doesn't receive lighting)
// - Blinn-Phong rendering (as in 2nd year labs, where we used simple lighting equations for diffuse + specular lighting)
// - Physically-Based-Rendering (PBR) with metalness/roughness textures
// - Physically-Based-Rendering (PBR) with specular/glossiness textures
enum class SurfaceRenderMethod
{
	UnlitColour,  // Material diffuse colour only - single colour object (no lighting)
	UnlitTexture, // Material diffuse colour * diffuse map - plain texture with no lighting

	// Blinn-Phong diffuse + specular lighting, what we used in the 2nd year Graphics module
	BlinnColour,          // Diffuse and specular lighting only, no textures
	BlinnTexture,         // Diffuse and specular lighting with at least a diffuse texture and optionally a specular texture
	BlinnNormalMapping,   // Same as BlinnTexture, but with additional normal map
	BlinnParallaxMapping, // Same as BlinnTexture, but with additional normal and displacement maps

	// Physically-based rendering with metalness/roughness route, requires albedo, roughness and normal maps at least, optional metalness & displacement maps
	PbrNormalMapping,
	PbrParallaxMapping,    // If displacement map is present, parallax mapping is used

	// Physically-based rendering with specular/glossiness route, requires diffuse/albedo, glossiness and normal maps at least, optional specular & displacement maps
	PbrAltNormalMapping,
	PbrAltParallaxMapping, // If displacement map is present, parallax mapping is used

	Unknown,
};


// A RenderMethod describes how the GPU will render a particular submesh with a particular material.
// RenderMethod objects are descriptions, built during the import process. Only when they are finalised are they converted
// into RenderState objects, which contain the actual DirectX objects (shaders, textures etc.) needed to prepare the GPU
// to render in the given manner
struct RenderMethod
{
	GeometryRenderMethod     geometryRenderMethod = GeometryRenderMethod::Unknown;
	SurfaceRenderMethod      surfaceRenderMethod = SurfaceRenderMethod::Unknown;
	std::vector<TextureDesc> textures;
	PerMaterialConstants     constants;
};


//--------------------------------------------------------------------------------------
// Render State Class
//--------------------------------------------------------------------------------------
class RenderState
{
	//--------------------------------------------------------------------------------------
	// Construction
	//--------------------------------------------------------------------------------------
public:
	// Construct GPU render states directly from the RenderMethod descriptor
	RenderState(const RenderMethod& renderMethod);


	//--------------------------------------------------------------------------------------
	// Usage
	//--------------------------------------------------------------------------------------
public:
	// Set up the GPU to use this render state
	void Apply();


	//--------------------------------------------------------------------------------------
	// Static public methods
	//--------------------------------------------------------------------------------------
public:
	// Set an environment map for all RenderStates
	static void SetEnvironmentMap(ID3D11ShaderResourceView* environmentMap);

	// Call if DirectX state may have been changed by a 3rd party library call - resets internal tracking of state
	static void Reset();


	//--------------------------------------------------------------------------------------
	// Private data
	//--------------------------------------------------------------------------------------
private:
	// Shaders required by this render method, unused shaders will be nullptr
	// This class does not have ownership of these objects, the ShaderManager does, so no need to release them
	ID3D11VertexShader* mVertexShader = {};
	ID3D11PixelShader*  mPixelShader  = {};

	// Textures and samplers required by this render method, this class does not own these objects, the TextureManager does, so no need to release them
	std::array<ID3D11ShaderResourceView*, NUM_TEXTURE_TYPES> mTextures = {};
	std::array<ID3D11SamplerState*,       NUM_TEXTURE_TYPES> mSamplers = {};

	// Shader constants required by this render method (colours, transparency, parallax mapping depth etc.)
	// Not all constants will be needed by all render methods (indeed, some methods require none)
	PerMaterialConstants mConstants;


	//--------------------------------------------------------------------------------------
	// Static private data
	//--------------------------------------------------------------------------------------
private:
	// Shaders, states, textures and samplers already set on the GPU
	// Uses these values to avoid sending requests to the GPU when the current GPU setting is already correct
	static ID3D11VertexShader* mCurrentVertexShader;
	static ID3D11PixelShader*  mCurrentPixelShader;

	static std::array<ID3D11ShaderResourceView*, NUM_TEXTURE_TYPES> mCurrentTextures;
	static std::array<ID3D11SamplerState*,       NUM_TEXTURE_TYPES> mCurrentSamplers;

	static ID3D11ShaderResourceView* mCurrentEnvironmentMap;
};


#endif //_RENDER_METHOD_H_INCLUDED_
