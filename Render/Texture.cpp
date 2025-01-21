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

#include "Texture.h"

#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>
#include <filesystem>
#include <cmath>


//--------------------------------------------------------------------------------------
// Texture Loading
//--------------------------------------------------------------------------------------

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
std::pair<ID3D11Resource*, ID3D11ShaderResourceView*> TextureManager::LoadTexture(std::string textureName, bool allowSRGB /*= true*/)
{
    // If this texture has been loaded before, return existing texture objects
    auto loadedTexture = mTextures.find(textureName);
    if (loadedTexture != mTextures.end())  return loadedTexture->second;


    CComPtr<ID3D11Resource>           textureResource;
    CComPtr<ID3D11ShaderResourceView> textureSRV;

    // The functions used below require a std::wstring filename (wstring uses "wide characters" - a different variation of character
    // encoding and not a good one, so don't use them). This line converts an ordinary std::string filename to std::wstring (C++17)
    auto wTextureName = std::filesystem::path(textureName).wstring();

    // DDS files need a different function from other files so check the filename extension (case insensitive)
    HRESULT hr;
    std::string dds = ".dds";
    if (textureName.size() >= 4 &&
        std::equal(dds.rbegin(), dds.rend(), textureName.rbegin(), [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); })) // Case insensitive check
    {
        hr = DirectX::CreateDDSTextureFromFileEx(mDXDevice, wTextureName.c_str(), 0,
                                                 D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                                                 (DirectX::DX11::DDS_LOADER_FLAGS)(allowSRGB ? DirectX::DX11::DDS_LOADER_DEFAULT : DirectX::DX11::DDS_LOADER_IGNORE_SRGB),
                                                 &textureResource, &textureSRV);
    }
    else
    {
        hr = DirectX::CreateWICTextureFromFileEx(mDXDevice, mDXContext, wTextureName.c_str(), 0,
                                                 D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0,
                                                 (DirectX::DX11::WIC_LOADER_FLAGS)(allowSRGB ? DirectX::DX11::WIC_LOADER_SRGB_DEFAULT : DirectX::DX11::WIC_LOADER_IGNORE_SRGB),
                                                 &textureResource, &textureSRV );
    }
    if (FAILED(hr))
    {
        mLastError = "Failure to load texture: " + textureName;
        return { nullptr, nullptr };
    }

    // Enter DirectX objects into map of loaded textures, then return to caller.
    mTextures[textureName] = { textureResource, textureSRV };
    return { textureResource, textureSRV };
}


//--------------------------------------------------------------------------------------
// Sampler Creation
//--------------------------------------------------------------------------------------

// Create a DirectX sampler object from the given sampler definition
// Do not release the returned pointer as the TextureManager object manages its lifetime.
// If nullptr is returned, you can call GetLastError() for a string description of the error.
// TextureManager stores previously created samplers and will return the existing one if the same sampler type is requested
ID3D11SamplerState* TextureManager::CreateSampler(const SamplerState& samplerState)
{
    // If this texture has been loaded before, return existing texture objects
    auto existingSampler = mSamplers.find(samplerState);
    if (existingSampler != mSamplers.end())  return existingSampler->second;


    // Prepare DirectX sampler settings, first convert filter and addressing mode to DirectX types
    D3D11_SAMPLER_DESC samplerDesc = {};
    switch (samplerState.filter) 
    {
        case TextureFilter::FilterPoint:       samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;         break;
        case TextureFilter::FilterBilinear:    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;  break;
        case TextureFilter::FilterTrilinear:   samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;        break;
        case TextureFilter::FilterAnisotropic: samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;               break;
        default: samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    }
    switch (samplerState.addressingMode)
    {
        case TextureAddressingMode::AddressingWrap:   samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;    break;
        case TextureAddressingMode::AddressingMirror: samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;  break;
        case TextureAddressingMode::AddressingClamp:  samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;   break;
        case TextureAddressingMode::AddressingBorder: samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;  break;
        default: samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    }
    samplerDesc.AddressV = samplerDesc.AddressW = samplerDesc.AddressU; // Assume addressing mode is same in U & V, rare to do otherwise
    samplerDesc.MaxAnisotropy = 8;                 // Number of samples used if using anisotropic filtering, more is better but max value depends on GPU
    samplerDesc.MaxLOD        = D3D11_FLOAT32_MAX; // Controls how much mip-mapping can be used. These settings are full mip-mapping, the usual values
    samplerDesc.MinLOD        = 0;                 // --"--

    // Create sampler
    CComPtr<ID3D11SamplerState> pointSampler;
    if (FAILED(mDXDevice->CreateSamplerState(&samplerDesc, &pointSampler)))
    {
        mLastError = "Failure creating point sampler";
        return nullptr;
    }


    // Enter DirectX sampler into map of existing samplers, then return to caller.
    mSamplers[samplerState] = pointSampler;
    return pointSampler;
}
