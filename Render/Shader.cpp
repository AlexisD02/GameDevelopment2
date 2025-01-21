//--------------------------------------------------------------------------------------
// ShaderManager class encapsulates the loading and destruction of shaders
//--------------------------------------------------------------------------------------
// Other code that requires a shader must pass the shader filename to the ShaderManager, if the ShaderManager has already
// loaded that shader the existing pointer is returned, otherwise the shader is loaded. The calling code is not responsible 
// for the lifetime of the shader and does not need to release it ever. The ShaderManager will release all shaders when
// it is destroyed. Practically that means all shaders exist until the app closes, but that is fine as they are small.

#include "Shader.h"

#include <d3dcompiler.h>
#include <fstream>
#include <vector>


//--------------------------------------------------------------------------------------
// Shader creation
//--------------------------------------------------------------------------------------

// Load shader with given filename. Do not include the extension in the filename
// Returns a DirectX shader object for use in d3dContext->?SSetShader. Returns nullptr on error
// Do not release the returned pointer as the ShaderManager object manages the shader object lifetime.
// If nullptr is returned, you can call GetLastError() for a string description of the error
// The function reads the compiled .cso file not the .hlsl files. All shader names should be unique
// ShaderManager stores previously loaded shaders and will return them if a shader is requested for a second time
ID3D11VertexShader* ShaderManager::LoadVertexShader(std::string shaderName)
{
	// If this shader has been loaded before, return existing shader object
	auto loadedShader = mVertexShaders.find(shaderName);
	if (loadedShader != mVertexShaders.end())  return loadedShader->second;
	
	// Load shader bytecode
	std::vector<char> byteCode;
	if (!LoadShaderByteCode(shaderName, byteCode))  return nullptr;


	// Create vertex shader object from loaded file
	CComPtr<ID3D11VertexShader> shader;
	HRESULT hr = mDXDevice->CreateVertexShader(byteCode.data(), byteCode.size(), nullptr, &shader);
	if (FAILED(hr))
	{
		mLastError = "Failure to create vertex shader from: " + shaderName + ".cso. Possibly running on low spec machine?";
		return nullptr;
	}


	// Enter shader object into map of existing shaders, then return to caller.
	mVertexShaders[shaderName] = shader;
	return shader;
}

// Load shader with given filename. Do not include the extension in the filename
// Returns a DirectX shader object for use in d3dContext->?SSetShader. Returns nullptr on error
// Do not release the returned pointer as the ShaderManager object manages the shader object lifetime.
// If nullptr is returned, you can call GetLastError() for a string description of the error
// The function reads the compiled .cso file not the .hlsl files. All shader names should be unique
// ShaderManager stores previously loaded shaders and will return them if a shader is requested for a second time
ID3D11HullShader* ShaderManager::LoadHullShader(std::string shaderName)
{
	// If this shader has been loaded before, return existing shader object
	auto loadedShader = mHullShaders.find(shaderName);
	if (loadedShader != mHullShaders.end())  return loadedShader->second;


	// Load shader bytecode
	std::vector<char> byteCode;
	if (!LoadShaderByteCode(shaderName, byteCode))  return nullptr;

	// Create hull shader object from loaded file
	CComPtr<ID3D11HullShader> shader;
	HRESULT hr = mDXDevice->CreateHullShader(byteCode.data(), byteCode.size(), nullptr, &shader);
	if (FAILED(hr))
	{
		mLastError = "Failure to create hull shader from: " + shaderName + ".cso. Possibly running on low spec machine?";
		return nullptr;
	}


	// Enter shader object into map of existing shaders, then return to caller.
	mHullShaders[shaderName] = shader;
	return shader;
}

// Load shader with given filename. Do not include the extension in the filename
// Returns a DirectX shader object for use in d3dContext->?SSetShader. Returns nullptr on error
// Do not release the returned pointer as the ShaderManager object manages the shader object lifetime.
// If nullptr is returned, you can call GetLastError() for a string description of the error
// The function reads the compiled .cso file not the .hlsl files. All shader names should be unique
// ShaderManager stores previously loaded shaders and will return them if a shader is requested for a second time
ID3D11DomainShader* ShaderManager::LoadDomainShader(std::string shaderName)
{
	// If this shader has been loaded before, return existing shader object
	auto loadedShader = mDomainShaders.find(shaderName);
	if (loadedShader != mDomainShaders.end())  return loadedShader->second;


	// Load shader bytecode
	std::vector<char> byteCode;
	if (!LoadShaderByteCode(shaderName, byteCode))  return nullptr;

	// Create domain shader object from loaded file
	CComPtr<ID3D11DomainShader> shader;
	HRESULT hr = mDXDevice->CreateDomainShader(byteCode.data(), byteCode.size(), nullptr, &shader);
	if (FAILED(hr))
	{
		mLastError = "Failure to create domain shader from: " + shaderName + ".cso. Possibly running on low spec machine?";
		return nullptr;
	}


	// Enter shader object into map of existing shaders, then return to caller.
	mDomainShaders[shaderName] = shader;
	return shader;
}

// Load shader with given filename. Do not include the extension in the filename
// Returns a DirectX shader object for use in d3dContext->?SSetShader. Returns nullptr on error
// Do not release the returned pointer as the ShaderManager object manages the shader object lifetime.
// If nullptr is returned, you can call GetLastError() for a string description of the error
// The function reads the compiled .cso file not the .hlsl files. All shader names should be unique
// ShaderManager stores previously loaded shaders and will return them if a shader is requested for a second time
ID3D11GeometryShader* ShaderManager::LoadGeometryShader(std::string shaderName)
{
	// If this shader has been loaded before, return existing shader object
	auto loadedShader = mGeometryShaders.find(shaderName);
	if (loadedShader != mGeometryShaders.end())  return loadedShader->second;


	// Load shader bytecode
	std::vector<char> byteCode;
	if (!LoadShaderByteCode(shaderName, byteCode))  return nullptr;

	// Create geometry shader object from loaded file
	CComPtr<ID3D11GeometryShader> shader;
	HRESULT hr = mDXDevice->CreateGeometryShader(byteCode.data(), byteCode.size(), nullptr, &shader);
	if (FAILED(hr))
	{
		mLastError = "Failure to create geometry shader from: " + shaderName + ".cso. Possibly running on low spec machine?";
		return nullptr;
	}


	// Enter shader object into map of existing shaders, then return to caller.
	mGeometryShaders[shaderName] = shader;
	return shader;
}

// Load shader with given filename. Do not include the extension in the filename
// Returns a DirectX shader object for use in d3dContext->?SSetShader. Returns nullptr on error
// Do not release the returned pointer as the ShaderManager object manages the shader object lifetime.
// If nullptr is returned, you can call GetLastError() for a string description of the error
// The function reads the compiled .cso file not the .hlsl files. All shader names should be unique
// ShaderManager stores previously loaded shaders and will return them if a shader is requested for a second time
ID3D11PixelShader* ShaderManager::LoadPixelShader(std::string shaderName)
{
	// If this shader has been loaded before, return existing shader object
	auto loadedShader = mPixelShaders.find(shaderName);
	if (loadedShader != mPixelShaders.end())  return loadedShader->second;


	// Load shader bytecode
	std::vector<char> byteCode;
	if (!LoadShaderByteCode(shaderName, byteCode))  return nullptr;

	// Create pixel shader object from loaded file
	CComPtr<ID3D11PixelShader> shader;
	HRESULT hr = mDXDevice->CreatePixelShader(byteCode.data(), byteCode.size(), nullptr, &shader);
	if (FAILED(hr))
	{
		mLastError = "Failure to create pixel shader from: " + shaderName + ".cso. Possibly running on low spec machine?";
		return nullptr;
	}


	// Enter shader object into map of existing shaders, then return to caller.
	mPixelShaders[shaderName] = shader;
	return shader;
}

//--------------------------------------------------------------------------------------
// Special shaders
//--------------------------------------------------------------------------------------

// Special method to load a geometry shader that can use the stream-out stage, Use like other shader loading functions 
// except also pass the stream out declaration, number of entries in the declaration and total size of the output data
// Stores the shader name and returns the same shader pointer if the same name is requested again - for that reason
// you should not use the same shader name with different stream out specifications.
ID3D11GeometryShader* ShaderManager::LoadStreamOutGeometryShader(std::string shaderName, D3D11_SO_DECLARATION_ENTRY* soDecl,
                                                                 unsigned int soNumEntries, unsigned int soStride)
{
	// If this shader has been loaded before, return existing shader object
	auto loadedShader = mGeometryShaders.find(shaderName);
	if (loadedShader != mGeometryShaders.end())  return loadedShader->second;

	// Load shader bytecode
	std::vector<char> byteCode;
	if (!LoadShaderByteCode(shaderName, byteCode))  return nullptr;

	// Create shader object from loaded file
	CComPtr<ID3D11GeometryShader> shader;
	HRESULT hr = 
		mDXDevice->CreateGeometryShaderWithStreamOutput(byteCode.data(), byteCode.size(), soDecl, soNumEntries, &soStride, 1,
		                                                 D3D11_SO_NO_RASTERIZED_STREAM, nullptr, &shader);
	if(FAILED(hr))
	{
		mLastError = "Failure to create stream-out geometry shader from: " + shaderName + 
			         ".cso. Error in stream out specification or possibly running on low spec machine?";
		return nullptr;
	}
	mGeometryShaders[shaderName] = shader;

	return shader;
}


//--------------------------------------------------------------------------------------
// Helper functions
//--------------------------------------------------------------------------------------

// Load the cso (compiled shader object) file of the given shader (any type) and put the bytecode in the
// given vector. Used by all the shader loading functions above. Returns true on success
bool ShaderManager::LoadShaderByteCode(std::string shaderName, std::vector<char>& byteCode)
{
	// Open compiled shader object file
	std::ifstream shaderFile(shaderName + ".cso", std::ios::in | std::ios::binary | std::ios::ate);
	if (!shaderFile.is_open())
	{
		mLastError = "Failure to open file: " + shaderName + ".cso. Ensure the file exists in the working folder (typically with the exectable).";
		return false;
	}

	// Read file into vector of chars
	std::streamoff fileSize = shaderFile.tellg();
	shaderFile.seekg(0, std::ios::beg);
	byteCode.resize(fileSize);
	shaderFile.read(&byteCode[0], fileSize);
	if (shaderFile.fail())
	{
		mLastError = "File read error in file: " + shaderName + ".cso. Possible corrupt file or disk space problem.";
		return false;
	}

	return true;
}


// Helper function that creates a shader signature to assist in the creation of vertex layouts for vertex buffers
// Returned object should be released when it has been finished with. Returns nullptr on failure
// Very advanced topic: When creating a vertex layout for geometry (see Mesh.cpp), you need the signature
// (bytecode) of a shader that uses that vertex layout. This is an annoying requirement and tends to create
// unnecessary coupling between shaders and vertex buffers.
// This is a trick to simplify things - pass a vertex layout to this function and it will write and compile
// a temporary shader to match. You don't need to know about the actual shaders in use in the app.
// Requires the shader compiler DLL in the same folder as the executable
ID3DBlob* CreateSignatureForVertexLayout(const D3D11_INPUT_ELEMENT_DESC vertexLayout[], int numElements)
{
	std::string shaderSource = "float4 main(";
	for (int elt = 0; elt < numElements; ++elt)
	{
		auto& format = vertexLayout[elt].Format;
		// This list should be more complete for production use
		if      (format == DXGI_FORMAT_R32G32B32A32_FLOAT) shaderSource += "float4";
		else if (format == DXGI_FORMAT_R32G32B32_FLOAT)    shaderSource += "float3";
		else if (format == DXGI_FORMAT_R32G32_FLOAT)       shaderSource += "float2";
		else if (format == DXGI_FORMAT_R32_FLOAT)          shaderSource += "float";
		else if (format == DXGI_FORMAT_R8G8B8A8_UINT)      shaderSource += "uint4";
		else if (format == DXGI_FORMAT_R8G8B8A8_UNORM)     shaderSource += "float4"; // 4-byte colour in data mapped to 4 (0-1) floats in the shader (UNORM indicates this mapping)
		else return nullptr; // Unsupported type in layout

		uint8_t index = static_cast<uint8_t>(vertexLayout[elt].SemanticIndex);
		std::string semanticName = vertexLayout[elt].SemanticName;
		semanticName += ('0' + index);

		shaderSource += " ";
		shaderSource += semanticName;
		shaderSource += " : ";
		shaderSource += semanticName;
		if (elt != numElements - 1)  shaderSource += " , ";
	}
	shaderSource += ") : SV_Position {return 0;}";

	ID3DBlob* compiledShader;
	HRESULT hr = D3DCompile(shaderSource.c_str(), shaderSource.length(), NULL, NULL, NULL, "main",
		                    "vs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL0, 0, &compiledShader, NULL);
	if (FAILED(hr))
	{
		return nullptr;
	}

	return compiledShader;
}
