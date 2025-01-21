//--------------------------------------------------------------------------------------
// CBufferManager encapsulates the creation/destruction of constant buffers
//--------------------------------------------------------------------------------------
// Some general constant buffer support functions are here too
// 
// Constant buffers help pass data from C++ to support shaders. Most commonly matrices and light information,
// but also anything required aside from the vertex buffer data. Constant buffers appear as small structs, with
// matching definitions in C++ (CPU) and HLSL (GPU). The CBufferManager helps create, maintain and destroy
// constant buffers. Any code that requires a constant buffer should request it from this class and the calling
// code will not be  responsible for the lifetime of returned buffer. The CBufferManager will release all
// its buffers when destroyed. Practically that means that all constant buffers will exist until the app closes

#ifndef _C_BUFFER_H_INCLUDED_
#define _C_BUFFER_H_INCLUDED_

#define NOMINMAX // Use NOMINMAX to stop certain Microsoft headers defining "min" and "max", which breaks some libraries (e.g. assimp)
#include <d3d11.h>
#include <atlbase.h> // For CComPtr (see member variables)

#include <vector>
#include <string>
#include <cstddef>


//--------------------------------------------------------------------------------------
// Constant Buffer Manager Class
//--------------------------------------------------------------------------------------
class CBufferManager
{
	//--------------------------------------------------------------------------------------
	// Construction
	//--------------------------------------------------------------------------------------
public:
	// Create the constant buffer manager, pass DirectX device and context
	CBufferManager(ID3D11Device* device, ID3D11DeviceContext* context)
		: mDXDevice(device), mDXContext(context) {}


	//--------------------------------------------------------------------------------------
	// Constant buffer creation
	//--------------------------------------------------------------------------------------
public:
	// Create a constant buffer of the given size (in bytes). Typically pass sizeof the struct that the C++ 
	// code will use for the contents of the constant buffer. Returns a DirectX constant buffer object
	// or nullptr on error. Do not release the returned pointer as the CBufferManager object manages buffer
	// lifetimes. If nullptr is returned, you can call GetLastError() for a string description of the error.
	ID3D11Buffer* CreateCBuffer(std::size_t bufferSize);


	//--------------------------------------------------------------------------------------
	// Helper functions
	//--------------------------------------------------------------------------------------

	// Enable the given constant buffer on the given slot for all shaders - i.e. all shaders will get access to it
	// Pass the DirectX constant buffer object and the slot index to use it on. The matching constant buffer declataion
	// in the the HLSL code should be declared:    cbuffer bufferName : register(b?)   where ? is the slot number
	void EnableCBuffer(ID3D11Buffer* buffer, unsigned int slot);

	// Template function to update a constant buffer. Pass the DirectX constant buffer object and the C++ data structure
	// you want to update it with. The structure will be copied in full over to the GPU constant buffer, where it will
	// be available to shaders. This is used to update model and camera positions, lighting data etc.
	template <class T>
	void UpdateCBuffer(ID3D11Buffer* buffer, const T& bufferData)
	{
		D3D11_MAPPED_SUBRESOURCE cb;
		mDXContext->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &cb);
		memcpy(cb.pData, &bufferData, sizeof(T));
		mDXContext->Unmap(buffer, 0);
	}


	//--------------------------------------------------------------------------------------
	// Private Data
	//--------------------------------------------------------------------------------------
private:
	ID3D11Device*        mDXDevice;
	ID3D11DeviceContext* mDXContext;

	// Vector of DirectX constant buffer objects
	// CComPtr is a smart pointer that can be used for DirectX objects (similar to unique_ptr). Using CComPtr means we 
	// don't need a destructor to release everything from DirectX, it will happen automatically when the CBufferManger
	// is destroyed. CComPtr requires <atlbase.h> to be included (and the ATL component to be installed as part of Visual Studio)
	std::vector<CComPtr<ID3D11Buffer>> mCBuffers;

	// Description of the most recent error from CreateCBuffer
	std::string mLastError;
};


#endif //_C_BUFFER_H_INCLUDED_
