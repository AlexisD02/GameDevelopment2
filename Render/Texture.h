//--------------------------------------------------------------------------------------
// TextureManager class encapsulates the loading and destruction of textures and texture samplers
//--------------------------------------------------------------------------------------
// Other code that requires a texture or sampler must pass the filename/settings to the TextureManager, if the
// ShaderManager has already loaded/created that texture/sampler the existing pointer is returned, otherwise it
// is created. The calling code is not responsible for the lifetime of textures/samplers and do not need to release
// them ever. The ShaderManager will release all textures and samplers when it is destroyed. Practically that means
// all textures and samplers exist until the app closes. That is fine for samplers (they are small), but a production
// app would need a way to release textures that are no longer in use. Some reference counting connected with the
// mesh class could handle that (when meshes are destroyed, they release their textures and this class keeps count)

#ifndef _TEXTURE_H_INCLUDED_
#define _TEXTURE_H_INCLUDED_

#include "TextureTypes.h"

#define NOMINMAX // Use NOMINMAX to stop certain Microsoft headers defining "min" and "max", which breaks some libraries (e.g. assimp)
#include <d3d11.h>
#include <atlbase.h> // For CComPtr (see member variables)

#include <string>
#include <map>
#include <vector>


//--------------------------------------------------------------------------------------
// Texture Manager Class
//--------------------------------------------------------------------------------------
class TextureManager
{
	//--------------------------------------------------------------------------------------
	// Construction
	//--------------------------------------------------------------------------------------
public:
	// Create the texture manager, pass DirectX device and context
	TextureManager(ID3D11Device* device, ID3D11DeviceContext* context)
		: mDXDevice(device), mDXContext(context) {}


	//--------------------------------------------------------------------------------------
	// Usage
	//--------------------------------------------------------------------------------------
public:
	// Load texture with given filename into DirectX objects
	// Returns both a DirectX resource object (the texture data) and a shader resource view (SRV - an object required for shaders to access
	// a texture). Use the SRV in d3dContext->?SSetShaderResources.
	// Returns a pair of values, use it like this:
	//    auto [newTexture, newextureSRV] = LoadTexture("Brick.jpg");  // Version 1: Creates two new variables newTexture and newTextureSRV
	//    std::tie(texture, textureSRV)   = LoadTexture("Brick.jpg");  // Version 2: Assigns to two *existing* variables texture and textureSRV
	// Both return values will be nullptr on error
	// Do not release the returned pointers as the TextureManager object manages their lifetimes.
	// If nullptr is returned, you can call GetLastError() for a string description of the error.
	// TextureManager stores previously loaded textures and will return the existing one if the same texture is requested for a second time
	std::pair<ID3D11Resource*, ID3D11ShaderResourceView*> LoadTexture(std::string textureName, bool allowSRGB = true);


	// Create a DirectX sampler object from the given sampler definition
	// Do not release the returned pointer as the TextureManager object manages its lifetime.
	// If nullptr is returned, you can call GetLastError() for a string description of the error.
	// TextureManager stores previously created samplers and will return the existing one if the same sampler type is requested
	ID3D11SamplerState* CreateSampler(const SamplerState& samplerState);


	// If the LoadTexture or CreateSampler functions return nullptr to indicate an error, the text description
	// of the (most recent) error can be fetched with this function
	std::string GetLastError() { return mLastError; }


	//--------------------------------------------------------------------------------------
	// Private Data
	//--------------------------------------------------------------------------------------
private:
	ID3D11Device*        mDXDevice;
	ID3D11DeviceContext* mDXContext;

	// Map of texture filenames to a pair of DirectX texture objects - the texture resource (the texture data) & shader
	// resource view (object required for shaders to access a texture). Given a new texture filename to load, can quickly
	// check to see if it has already been loaded and if so return the existing pointer.
	// CComPtr is a smart pointer that can be used for DirectX objects (similar to unique_ptr). Using CComPtr means we 
	// don't need a destructor to release everything from DirectX, it will happen automatically when the TextureManger
	// is destroyed. CComPtr requires <atlbase.h> to be included (and the ATL component to be installed as part of Visual Studio)
	std::map<std::string, std::pair<CComPtr<ID3D11Resource>, CComPtr<ID3D11ShaderResourceView>>> mTextures;

	// Map of sampler descriptions to DirectX sampler objects. Given a new sampler to create, can quickly check to see if
	// an identical sampler has already been created and if so return the existing pointer.
	// Samplers are held by CComPtr again (see above) and will all be detroyed automatically when the TextureManager is destroyed
	std::map<SamplerState, CComPtr<ID3D11SamplerState>> mSamplers;

	// Description of the most recent error from LoadTexture or CreateSampler
	std::string mLastError;
};


#endif // _TEXTURE_H_INCLUDED_
