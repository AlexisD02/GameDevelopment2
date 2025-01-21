//--------------------------------------------------------------------------------------
// RenderState class to encapsulate the GPU States needed to render a submesh
//--------------------------------------------------------------------------------------
// Each submesh in a mesh holds a single RenderState, a combination of vertex shader, pixel shader, textures and constants.
// Enable a particular RenderState object on the GPU by calling its Apply method, which will select shaders, attach textures
// to them etc. Subsequent rendering will now use the appropriate render state.
// 
// RenderState objects are constructed from RenderMethod objects. A RenderMethod is a readable description of the processes
// and textures/constants needed to render a material. RenderMethods are created during the import process (see Mesh.h/cpp)
// by looking at the materials used by submeshes in the 3D files. 

#include "RenderMethod.h"

#include "Shader.h"
#include "Texture.h"
#include "CBuffer.h"
#include "RenderGlobals.h"

#include <stdexcept>



//--------------------------------------------------------------------------------------
// Construction
//--------------------------------------------------------------------------------------

// Construct GPU render states directly from the RenderMethod descriptor
RenderState::RenderState(const RenderMethod& renderMethod)
{
	// Get shaders required to support given render method
	std::string vertexShaderName, pixelShaderName;
	if (renderMethod.geometryRenderMethod == GeometryRenderMethod::Rigid) // Vertex and pixel shader name for each surface render method, assuming GeometryRenderMethod::Rigid
	{
		switch (renderMethod.surfaceRenderMethod)
		{
			case SurfaceRenderMethod::UnlitColour:            vertexShaderName = "vs_p_p2c";                 pixelShaderName = "ps_colour-only";         break;
			case SurfaceRenderMethod::UnlitTexture:           vertexShaderName = "vs_puv_p2c_uv";            pixelShaderName = "ps_tex-only";            break;
			case SurfaceRenderMethod::BlinnColour:            vertexShaderName = "vs_pn_p2c_pn2w";           pixelShaderName = "ps_blinn-1";             break;
			case SurfaceRenderMethod::BlinnTexture:           vertexShaderName = "vs_pnuv_p2c_pn2w_uv";      pixelShaderName = "ps_blinn-1_tex-d";       break;
			case SurfaceRenderMethod::BlinnNormalMapping:     vertexShaderName = "vs_pntuv_p2c_pnt2w_uv";    pixelShaderName = "ps_blinn-1n_tex-dn";     break;
			case SurfaceRenderMethod::BlinnParallaxMapping:   vertexShaderName = "vs_pntuv_p2c_pnt2w_uv";    pixelShaderName = "ps_blinn-1p_tex-dnh";    break;
			case SurfaceRenderMethod::PbrNormalMapping:       vertexShaderName = "vs_pntuv_p2c_pnt2w_uv";    pixelShaderName = "ps_pbr1-1n";             break;
			case SurfaceRenderMethod::PbrParallaxMapping:     vertexShaderName = "vs_pntuv_p2c_pnt2w_uv";    pixelShaderName = "ps_pbr1-1p";             break; 
			case SurfaceRenderMethod::PbrAltNormalMapping:    vertexShaderName = "vs_pntuv_p2c_pnt2w_uv";    pixelShaderName = "ps_pbr2-1n";             break; // TODO ps
			case SurfaceRenderMethod::PbrAltParallaxMapping:  vertexShaderName = "vs_pntuv_p2c_pnt2w_uv";    pixelShaderName = "ps_pbr2-1p";             break; // TODO ps
			default: throw std::runtime_error("RenderState: Unsupported surface render method");
		}
	}
	else if (renderMethod.geometryRenderMethod == GeometryRenderMethod::Skinned) // Vertex and pixel shader name for each surface render method, assuming GeometryRenderMethod::Skinned
	{
		switch (renderMethod.surfaceRenderMethod)
		{
			case SurfaceRenderMethod::UnlitColour:            vertexShaderName = "vs_p_skp2c";                 pixelShaderName = "ps_colour-only";         break; // TODO all vs
			case SurfaceRenderMethod::UnlitTexture:           vertexShaderName = "vs_puv_skp2c_uv";            pixelShaderName = "ps_tex-only";            break;
			case SurfaceRenderMethod::BlinnColour:            vertexShaderName = "vs_pn_skp2c_pn2w";           pixelShaderName = "ps_blinn-1";             break;
			case SurfaceRenderMethod::BlinnTexture:           vertexShaderName = "vs_pnuv_skp2c_pn2w_uv";      pixelShaderName = "ps_blinn-1_tex-d";       break;
			case SurfaceRenderMethod::BlinnNormalMapping:     vertexShaderName = "vs_pntuv_skp2c_pnt2w_uv";    pixelShaderName = "ps_blinn-1n_tex-dn";     break;
			case SurfaceRenderMethod::BlinnParallaxMapping:   vertexShaderName = "vs_pntuv_skp2c_pnt2w_uv";    pixelShaderName = "ps_blinn-1p_tex-dnh";    break;
			case SurfaceRenderMethod::PbrNormalMapping:       vertexShaderName = "vs_pntuv_skp2c_pnt2w_uv";    pixelShaderName = "ps_pbr1-1n";             break;
			case SurfaceRenderMethod::PbrParallaxMapping:     vertexShaderName = "vs_pntuv_skp2c_pnt2w_uv";    pixelShaderName = "ps_pbr1-1p";             break;
			case SurfaceRenderMethod::PbrAltNormalMapping:    vertexShaderName = "vs_pntuv_skp2c_pnt2w_uv";    pixelShaderName = "ps_pbr2-1n";             break;
			case SurfaceRenderMethod::PbrAltParallaxMapping:  vertexShaderName = "vs_pntuv_skp2c_pnt2w_uv";    pixelShaderName = "ps_pbr2-1p";             break;
			default: throw std::runtime_error("RenderState: Unsupported surface render method");
		}
	}
	else throw std::runtime_error("RenderState: Unsupported render method");

	// Load required shaders (shader manager will ensure the same shader isn't loaded twice)
	mVertexShader = DX->Shaders()->LoadVertexShader(vertexShaderName);
	if (mVertexShader == nullptr)  throw std::runtime_error("RenderState: " + DX->Shaders()->GetLastError());
	mPixelShader  = DX->Shaders()->LoadPixelShader (pixelShaderName);
	if (mPixelShader == nullptr)   throw std::runtime_error("RenderState: " + DX->Shaders()->GetLastError());


	// Load all textures and samplers indicated by the render method. They are loaded into the array slot indicated by
	// the texture type and will appear in shaders on that same slot (see comment on TextureType)
	mTextures.fill(nullptr);
	mSamplers.fill(nullptr);
	for (int i = 0; i < renderMethod.textures.size(); ++i)
	{
		// Support null textures (filename = ""). Set nullptr as texture, but set sampler as normal. DirectX debugger issues warnings if there are null samplers
		if (renderMethod.textures[i].filename != "")
		{
			bool allowSRGB = renderMethod.textures[i].type != TextureType::Roughness && // Some data maps should not be in SRGB format 
			                 renderMethod.textures[i].type != TextureType::Normal    &&
			                 renderMethod.textures[i].type != TextureType::Displacement;
			auto [texture, textureSRV] = DX->Textures()->LoadTexture(renderMethod.textures[i].filename, allowSRGB);
			if (texture == nullptr)  throw std::runtime_error("RenderState: Failed to load texture: " + renderMethod.textures[i].filename + " - " + DX->Textures()->GetLastError());
			mTextures[static_cast<int>(renderMethod.textures[i].type)] = textureSRV;
		}
		else
		{
			mTextures[static_cast<int>(renderMethod.textures[i].type)] = nullptr;
		}
		auto sampler = DX->Textures()->CreateSampler(renderMethod.textures[i].samplerState);
		if (sampler == nullptr)  throw std::runtime_error("RenderState: Failed to load sampler - " + DX->Textures()->GetLastError());
		mSamplers[static_cast<int>(renderMethod.textures[i].type)] = sampler;
	}

	
	// Initialise CPU-side constants (to be sent to GPU each time this material is used)
	mConstants = renderMethod.constants;
}


//--------------------------------------------------------------------------------------
// Usage
//--------------------------------------------------------------------------------------

// Set up the GPU to use this render state
void RenderState::Apply()
{
	// Set each shader on GPU, don't do anything if currently selected shader is already the correct one
	// Note: the ShaderManager ensures that different meshes using the same shader get the same shader objects so this will work across different meshes
	if (mVertexShader != mCurrentVertexShader)
	{
		DX->Context()->VSSetShader(mVertexShader, nullptr, 0);
		mCurrentVertexShader = mVertexShader;
	}
	if (mPixelShader != mCurrentPixelShader)
	{
		DX->Context()->PSSetShader(mPixelShader, nullptr, 0);
		mCurrentPixelShader = mPixelShader;
	}

	// Set textures and samplers on GPU, don't change them if current ones are already correct
	for (int i = 0; i < mTextures.size(); ++i)
		if (mTextures[i] != mCurrentTextures[i])
		{
			DX->Context()->PSSetShaderResources(i, 1, &mTextures[i]);
			mCurrentTextures[i] = mTextures[i];
		}

	for (int i = 0; i < mSamplers.size(); ++i)
		if (mSamplers[i] != mCurrentSamplers[i])
		{
			DX->Context()->PSSetSamplers(i, 1, &mSamplers[i]);
			mCurrentSamplers[i] = mSamplers[i];
		}

	DX->CBuffers()->UpdateCBuffer(gPerMaterialConstantBuffer, mConstants);
}


// Set an environment map for all RenderStates
void RenderState::SetEnvironmentMap(ID3D11ShaderResourceView* environmentMap)
{
	if (mCurrentEnvironmentMap == environmentMap)  return;

	DX->Context()->PSSetShaderResources(static_cast<int>(TextureType::Environment), 1, &environmentMap);
	mCurrentEnvironmentMap = environmentMap;
}


// Call if DirectX state may have been changed by a 3rd party library call - resets internal tracking of state
void RenderState::Reset()
{
	mCurrentVertexShader = {};
	mCurrentPixelShader = {};
	mCurrentTextures.fill(nullptr);
	mCurrentSamplers.fill(nullptr);
	mCurrentEnvironmentMap = {};
}


//--------------------------------------------------------------------------------------
// Static private data
//--------------------------------------------------------------------------------------

// Shaders, states, textures and samplers already set on the GPU
// Use these values to avoid sending requests to the GPU when the current GPU setting is already correct
// These are static members of the RenderState class (class global). Statics need to be initialised outside the class, here in the cpp file
ID3D11VertexShader*   RenderState::mCurrentVertexShader = {};
ID3D11PixelShader*    RenderState::mCurrentPixelShader  = {};

std::array<ID3D11ShaderResourceView*, NUM_TEXTURE_TYPES> RenderState::mCurrentTextures = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
std::array<ID3D11SamplerState*,       NUM_TEXTURE_TYPES> RenderState::mCurrentSamplers = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

ID3D11ShaderResourceView* RenderState::mCurrentEnvironmentMap = {};
