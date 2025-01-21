//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------

#ifndef _SCENE_H_INCLUDED_
#define _SCENE_H_INCLUDED_

#include "EntityTypes.h"
#include "ColourTypes.h"

#define NOMINMAX // Use NOMINMAX to stop certain Microsoft headers defining "min" and "max", which breaks some libraries (e.g. assimp)
#include <d3d11.h>
#include <SpriteFont.h> // DirectXTK helper library for text drawing

#include <string>
#include <memory>

// Forward declarations of various classes allows us to use pointers to those classes before those classes have been fully declared
// This help us reduce the number of include files here, which in turn minimises dependencies and speeds up compilation
//
// Note: if you have a class containing a unique_ptr to a forward declaration (that is the case here) then this class must also have a destructor
// signature here and declaration in the cpp file - this is required even if the destructor is default/empty. Otherwise you get compile errors.
class EntityManager;
class Entity;
class Camera;
class Mesh;


//--------------------------------------------------------------------------------------
// Scene Class
//--------------------------------------------------------------------------------------
class Scene
{
	//--------------------------------------------------------------------------------------
	// Construction
	//--------------------------------------------------------------------------------------
public:
	// Constructs a demo scene ready to be rendered/updated
	Scene();

	// No destruction required but see comment on forward declarations above
	~Scene(); 


	//--------------------------------------------------------------------------------------
	// Usage
	//--------------------------------------------------------------------------------------
public:
	// Render entire scene
	void Render();

	// Update entire scene. frameTime is the time passed since the last frame
	void Update(float frameTime);


	//--------------------------------------------------------------------------------------
	// Private helper functions
	//--------------------------------------------------------------------------------------
private:
	// Render one viewpoint of the scene from the given camera, helper function for Scene::Render
	void RenderFromCamera(Camera* camera);

	// Draw given text at the given 3D point, also pass camera in use. Optionally centre align and colour the text
	void DrawTextAtWorldPt(const Vector3& point, std::string text, Camera* camera, bool centreAlign = false, ColourRGB colour = { 1, 1, 1 });


	//--------------------------------------------------------------------------------------
	// Private Data
	//--------------------------------------------------------------------------------------
private:

	std::unique_ptr<Camera> mCamera;

	// Entities in the demo scene
	EntityID mLight = {};
	EntityID mBoat1 = {};
	EntityID mBoat2 = {};
	EntityID mBoat3 = {};

	// Additional light information
	ColourRGB mAmbientColour = { 0, 0, 0 };
	ID3D11ShaderResourceView* mEnvironmentMap = {};

	// Lock FPS to monitor refresh rate, which will set it to 60/120/144/240fps
	bool mLockFPS = true;

	// DirectXTK SpriteFont text drawing variables
	std::unique_ptr<DirectX::DX11::SpriteBatch> mSpriteBatch;
	std::unique_ptr<DirectX::DX11::SpriteFont>  mSmallFont;
	std::unique_ptr<DirectX::DX11::SpriteFont>  mMediumFont;
};


#endif //_SCENE_H_INCLUDED_
