//--------------------------------------------------------------------------------------
// Class encapsulating a camera
//--------------------------------------------------------------------------------------
// Holds position, rotation, near/far clip and field of view. These to a view and projection matrices as required

#include "Camera.h"

#include <cmath>

// Control the camera's position and rotation using keys provided
void Camera::Control(float frameTime, KeyCode turnUp,      KeyCode turnDown,     KeyCode turnLeft, KeyCode turnRight,
                                      KeyCode moveForward, KeyCode moveBackward, KeyCode moveLeft, KeyCode moveRight,
	                                  float movementSpeed, float rotationSpeed)
{
	//Mouse smoothing
	const float cameraSmoothing = 0.15f ; // Mouse smoothing strength (0-1)
	const float pixelsToRadians = 0.002f; // Converting pixels of mouse movement to rotations in radians
	static Vector2i mouse = GetMouse();
	static Vector2f mouseMove = {0, 0};

	Vector2i newMouse = GetMouse();
	mouseMove.x += (newMouse - mouse).x;
	mouseMove.y += (newMouse - mouse).y;
	mouse = newMouse;

	Vector2f mouseDelta;
	mouseDelta.x = mouseMove.x * frameTime / (0.1f + cameraSmoothing * 0.25f);
	mouseDelta.y = mouseMove.y * frameTime / (0.1f + cameraSmoothing * 0.25f);
	mouseMove -= mouseDelta;

	auto tempPosition = mTransform.Position(); // Store position before doing rotations allowing us to use world rotations even
	                                           // if the camera is not at the origin (world rotations occur around origin)
	
	// Mouse rotation
	mTransform = MatrixRotationX(mouseDelta.y * rotationSpeed * pixelsToRadians) * mTransform; // Pre-multiply = local X rotation
	mTransform = mTransform * MatrixRotationY(mouseDelta.x * rotationSpeed * pixelsToRadians); // Post-multiply = world Y rotation

	// Keyboard rotation
	if (KeyHeld(turnUp))
	{
		mTransform = MatrixRotationX(-rotationSpeed * frameTime) * mTransform; // Pre-multiply = local X rotation
	}
	if (KeyHeld(turnDown))
	{
		mTransform = MatrixRotationX(rotationSpeed * frameTime) * mTransform;
	}
	if (KeyHeld(turnRight))
	{
		mTransform = mTransform * MatrixRotationY( rotationSpeed * frameTime); // Post-multiply = world Y rotation
	}
	if (KeyHeld(turnLeft))
	{
		mTransform = mTransform * MatrixRotationY(-rotationSpeed * frameTime);
	}
	mTransform.Position() = tempPosition; // Restore position

	// Keyboard movement
	if (KeyHeld(moveRight))
	{
		mTransform.Position() += movementSpeed * mTransform.XAxis() * frameTime;
	}
	if (KeyHeld(moveLeft))
	{
		mTransform.Position() -= movementSpeed * mTransform.XAxis() * frameTime;
	}
	if (KeyHeld(moveForward) || KeyHeld(Mouse_RButton))
	{
		mTransform.Position() += movementSpeed * mTransform.ZAxis() * frameTime;
	}
	if (KeyHeld(moveBackward))
	{
		mTransform.Position() -= movementSpeed * mTransform.ZAxis() * frameTime;
	}
}


// Update the matrices used for the camera in the rendering pipeline
void Camera::UpdateMatrices()
{
    // View matrix is the usual matrix used for the camera in shaders, it is the inverse of the world matrix (see lectures)
    mViewMatrix = InverseAffine(mTransform);

    // Projection matrix, how to flatten the 3D world onto the screen.
	// Maths was covered mid-module CO3301 - it's an optional topic (not examinable). See header file for descriptions of the values used here
    float tanFOVx = std::tan(mFOVx * 0.5f);
    float scaleX = 1.0f / tanFOVx;
    float scaleY = mAspectRatio / tanFOVx;
    float scaleZa = mFarClip / (mFarClip - mNearClip);
    float scaleZb = -mNearClip * scaleZa;
    mProjectionMatrix = { scaleX,   0.0f,    0.0f,   0.0f,
                            0.0f, scaleY,    0.0f,   0.0f,
                            0.0f,   0.0f, scaleZa,   1.0f,
                            0.0f,   0.0f, scaleZb,   0.0f };

    // The view-projection matrix combines the two matrices above into one, which can save a multiply in the shaders (optional)
    mViewProjectionMatrix = mViewMatrix * mProjectionMatrix;
}


//-----------------------------------------------------------------------------
// Camera picking
//-----------------------------------------------------------------------------

// Return pixel coordinates corresponding to given world point when viewing from this
// camera. Pass the viewport width and height. The returned Vector3 contains the pixel
// coordinates in x and y and the Z-distance to the world point in z. If the Z-distance
// is less than the camera near clip (use NearClip() member function), then the world
// point is behind the camera and the 2D x and y coordinates are to be ignored.
Vector3 Camera::PixelFromWorldPt(Vector3 worldPoint, unsigned int viewportWidth, unsigned int viewportHeight)
{
	Vector3 pixelPoint;

	UpdateMatrices();

	// Transform world point into camera space and return immediately if point is behind camera near clip (it won't be on screen - no 2D pixel position)
	Vector4 cameraPt = mViewMatrix.TransformPoint(worldPoint);
	if (cameraPt.z < mNearClip)
	{
		return { 0, 0, cameraPt.z };
	}

	// Now transform into viewport (2D) space
	Vector4 viewportPt = cameraPt;
	viewportPt = cameraPt * mProjectionMatrix;

	viewportPt.x /= viewportPt.w;
	viewportPt.y /= viewportPt.w;

	float x = (viewportPt.x + 1.0f) * viewportWidth  * 0.5f;
	float y = (1.0f - viewportPt.y) * viewportHeight * 0.5f;

	return { x, y, cameraPt.z };
}


// Return the size of a pixel in world space at the given Z distance. Allows us to convert the 2D size of areas on the screen to actualy sizes in the world
// Pass the viewport width and height
Vector2 Camera::PixelSizeInWorldSpace(float Z, unsigned int viewportWidth, unsigned int viewportHeight)
{
	Vector2 size;

	// Size of the entire viewport in world space at the near clip distance - uses same geometry work that was shown in the camera picking lecture
	Vector2 viewportSizeAtNearClip;
    viewportSizeAtNearClip.x = 2 * mNearClip * std::tan(mFOVx * 0.5f);
    viewportSizeAtNearClip.y = viewportSizeAtNearClip.x /  mAspectRatio;

	// Size of the entire viewport in world space at the given Z distance
	Vector2 viewportSizeAtZ = viewportSizeAtNearClip * (Z / mNearClip);
	
	// Return world size of single pixel at given Z distance
	return { viewportSizeAtZ.x / viewportWidth, viewportSizeAtZ.y / viewportHeight };
}
