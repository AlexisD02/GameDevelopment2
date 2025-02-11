//--------------------------------------------------------------------------------------
// Scene geometry and layout preparation
// Scene rendering & update
//--------------------------------------------------------------------------------------

#ifndef _SCENE_H_INCLUDED_
#define _SCENE_H_INCLUDED_

#include "Boat.h"
#include "ReloadStation.h"
#include "Obstacle.h"
#include "SeaMine.h"
#include "RandomCrate.h"
#include "EntityTypes.h"
#include "ColourTypes.h"

#define NOMINMAX // Use NOMINMAX to stop certain Microsoft headers defining "min" and "max", which breaks some libraries (e.g. assimp)
#include <d3d11.h>
#include <SpriteFont.h> // DirectXTK helper library for text drawing

#include <string>
#include <memory>
#include <vector>

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
    void DrawTextAtWorldPt(const Vector3& point, std::string text, Camera* camera, bool centreAlign = false);

    // Handle camera picking and selection
    void HandleMousePicking();

    // Chase camera helpers
    void UpdateChaseCameras(float frameTime);

    void DrawGUI();

    //--------------------------------------------------------------------------------------
    // Private Data
    //--------------------------------------------------------------------------------------
private:
    // Cameras
    std::unique_ptr<Camera> mCamera; // User-controlled camera
    std::vector<std::unique_ptr<Camera>> mChaseCameras; // Chase cameras for each boat
    int mActiveCameraIndex = -1; // -1 indicates the main camera is active

    std::vector<BoatTemplate*> boatTemplates;
    std::vector<Boat*> allBoats;
    std::vector<EntityID> allBoatIDS;
    std::vector<ReloadStation*> reloadStations;
    std::vector<RandomCrate*> existingCrates;
    std::vector<SeaMine*> existingMines;

    // Entities in the demo scene
    EntityID mLight = {};

    // Additional light information
    ColourRGB mAmbientColour = { 0, 0, 0 };
    ColourRGB colour = ColourRGB(0xffffff);

    ID3D11ShaderResourceView* mEnvironmentMap = {};

    // Lock FPS to monitor refresh rate, which will set it to 60/120/144/240fps
    bool mLockFPS = true;
    bool mGamePaused = false;
    float mRandomCrateTimer = Random(3.0f, 6.0f);
    float mRandomMineTimer = Random(5.0f, 8.0f);

    // DirectXTK SpriteFont text drawing variables
    std::unique_ptr<DirectX::DX11::SpriteBatch> mSpriteBatch;
    std::unique_ptr<DirectX::DX11::SpriteFont>  mSmallFont;
    std::unique_ptr<DirectX::DX11::SpriteFont>  mMediumFont;

    bool mShowExtendedBoatUI = false;

    // Variables for camera picking
    Boat* mNearestEntity = nullptr;    // The entity closest to the mouse cursor
    Boat* mSelectedBoat = nullptr;     // The currently selected boat
    Boat* mSelectedUIBoat = nullptr;     // The currently selected boat
    float PickDist = 100.0f;             // Distance to place the boat when moving

    const float mChaseDistance = 40.0f;
    const float mChaseHeight = 20.0f;
    const float mChasePitch = ToRadians(15.0f);

    const unsigned int mMaxCrates = 8;
    const unsigned int mMaxMines = 10;

public:
    void SetPauseState(bool pause) { mGamePaused = pause; }
};


#endif //_SCENE_H_INCLUDED_
