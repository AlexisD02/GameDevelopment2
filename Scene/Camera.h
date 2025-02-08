//--------------------------------------------------------------------------------------
// Class encapsulating a camera
//--------------------------------------------------------------------------------------
// Holds transformation matrix for camera position/rotation, near/far clip and field of view
// Can use these to create view and projection matrices as required for rendering
// Transformation can be manipulated in a similar way to any model
// Also supports functions for 3D->2D "picking" (world coords <-> pixel position) as discussed in lectures

#ifndef _CAMERA_H_INCLUDED_
#define _CAMERA_H_INCLUDED_

#include "Vector2.h"
#include "Vector3.h"
#include "Matrix4x4.h"
#include "Input.h"
#include <numbers>

class Camera
{
public:
	/*-----------------------------------------------------------------------------------------
	   Construction and Usage
	-----------------------------------------------------------------------------------------*/

	// Constructor - initialise all settings, sensible defaults provided for everything.
	Camera(Vector3 position = {0,0,0}, Vector3 rotation = {0,0,0}, float fov = std::numbers::pi_v<float>/3,
		   float aspectRatio = 4.0f / 3.0f, float nearClip = 0.1f, float farClip = 10000.0f)
        : mFOVx(fov), mAspectRatio(aspectRatio), mNearClip(nearClip), mFarClip(farClip)
    {
		mTransform.Position() = position;
		mTransform.SetRotation(rotation);
		UpdateMatrices();
    }


	// Control the camera's position and rotation using keys provided
	void Control( float frameTime, KeyCode turnUp,      KeyCode turnDown,     KeyCode turnLeft, KeyCode turnRight,  
	                               KeyCode moveForward, KeyCode moveBackward, KeyCode moveLeft, KeyCode moveRight,
		                           float movementSpeed, float rotationSpeed);


	/*-----------------------------------------------------------------------------------------
	   Data access
	-----------------------------------------------------------------------------------------*/

	// Direct access to camera's transformation matrix
	Matrix4x4& Transform()  { return mTransform; }

	// Other getters / setters
	float GetAspectRatio() { return mAspectRatio; }
	float GetFOV()         { return mFOVx;        }
	float GetNearClip()    { return mNearClip;    }
	float GetFarClip()     { return mFarClip;     }

	void SetAspectRatio (float aspectRatio)  { mAspectRatio = aspectRatio;}
	void SetFOV         (float fov        )  { mFOVx        = fov;        }
	void SetNearClip    (float nearClip   )  { mNearClip    = nearClip;   }
	void SetFarClip     (float farClip    )  { mFarClip     = farClip;    }

	// Camera matrices used for rendering. These getters use "lazy evaluation" - they only update the member variables when they are requested
	Matrix4x4 GetViewMatrix()            { UpdateMatrices(); return mViewMatrix;           }
	Matrix4x4 GetProjectionMatrix()      { UpdateMatrices(); return mProjectionMatrix;     }
	Matrix4x4 GetViewProjectionMatrix()  { UpdateMatrices(); return mViewProjectionMatrix; }


	/*-----------------------------------------------------------------------------------------
	   Camera Picking
	-----------------------------------------------------------------------------------------*/
	// Convert 2D pixel positions to and from 3D world positions

	// Return pixel coordinates corresponding to given world point when viewing from this
	// camera. Pass the viewport width and height. The returned Vector3 contains the pixel
	// coordinates in x and y and the Z distance to the world point in Z. If the Z distance
	// is less than the camera near clip (use NearClip() member function), then the world
	// point is behind the camera and the 2D x and y coordinates are to be ignored.
	Vector3 PixelFromWorldPt(Vector3 worldPoint, unsigned int viewportWidth, unsigned int viewportHeight);

	void GetPickRay(float pixelX, float pixelY, unsigned int viewportWidth, unsigned int viewportHeight, Vector3& outRayStart, Vector3& outRayDir);

	bool WorldPtFromPixel(float pixelX, float pixelY, unsigned int viewportWidth, unsigned int viewportHeight, Vector3& outWorldPos);

	// Return the size of a pixel in world space at the given Z distance. Allows us to convert the 2D size of areas on the screen to actual sizes in the world
	// Pass the viewport width and height
	Vector2 PixelSizeInWorldSpace(float Z, unsigned int viewportWidth, unsigned int viewportHeight);


	/*-----------------------------------------------------------------------------------------
	   Private data
	-----------------------------------------------------------------------------------------*/
private:
	// Update the matrices used for the camera in the rendering pipeline
	void UpdateMatrices();

	// Camera settings: field of view, aspect ratio, near and far clip plane distances.
	float mFOVx;        // FOVx is the viewing angle (radians = degrees * PI/180) from left->right (high values give a fish-eye look)
    float mAspectRatio; // Aspect ratio is screen width / height (like 4/3, 16/9)
	float mNearClip;    // Near and far clip are the range of z distances that can be rendered (world units)
	float mFarClip;     // --"--

	// Camera matrices for positioning and rendering
	Matrix4x4 mTransform = Matrix4x4::Identity; // Easiest to treat the camera like a model and give it a transformation ("world" matrix)...
	Matrix4x4 mViewMatrix;                      // ...then the view matrix used in the shaders is the inverse of this world matrix

	Matrix4x4 mProjectionMatrix;     // Projection matrix holds the field of view and near/far clip distances
	Matrix4x4 mViewProjectionMatrix; // Combine (multiply) the view and projection matrices together, which
	                                 // can sometimes save a matrix multiply in the shader (optional)
};


#endif //_CAMERA_H_INCLUDED_
