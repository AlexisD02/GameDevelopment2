/*-----------------------------------------------------------------------------------------
  Entity class for Missiles
-----------------------------------------------------------------------------------------*/

#include "Missile.h"
#include "SceneGlobals.h"
#include "Messenger.h"
#include "Vector3.h"
#include "Matrix4x4.h"

#include <cmath>
#include <vector>

/*-----------------------------------------------------------------------------------------
   Update / Render
-----------------------------------------------------------------------------------------*/
// Entity Update functions should return false if the entity is to be destroyed
bool Missile::Update(float frameTime)
{
    // Apply gravity to the vertical velocity
    mVelocity.y += -9.81f * frameTime;

    // Update position based on current velocity
    Vector3 newPos = Transform().Position() + mVelocity * frameTime;
    Transform().Position() = newPos;

    if (newPos.y < -15.0f)
    {
        return false;
    }

    // Update missile's rotation to face its velocity direction
    if (mVelocity.Length() > 0.01f)
    {
        Transform().FaceDirection(Normalise(mVelocity));
    }

    // Retrieve all Boat entities
    std::vector<Boat*> allBoats = gEntityManager->GetAllBoatEntities();

    // Check collision with each boat, excluding the launching boat
    for (Boat* boatPtr : allBoats)
    {
        if (boatPtr->GetID() == mLaunchingBoatID) continue;

        Vector3 toBoat = boatPtr->Transform().Position() - newPos;
        float distSq = toBoat.x * toBoat.x + toBoat.y * toBoat.y + toBoat.z * toBoat.z;

        if (distSq <= (15.0f * 15.0f))
        {
            MissileHitData hitData;
            hitData.launchingBoatID = mLaunchingBoatID;

            gMessenger->DeliverMessage(GetID(), boatPtr->GetID(), MessageType::Hit, hitData);

            return false;
        }
    }

    return true; // Keep missile alive
}
