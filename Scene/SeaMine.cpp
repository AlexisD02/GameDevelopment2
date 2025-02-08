#include "SeaMine.h"
#include "SceneGlobals.h"
#include "Boat.h"
#include "Messenger.h"
#include "MathHelpers.h"

#include <vector>
#include <cmath>

bool SeaMine::Update(float frameTime)
{
    mOscillationTime += frameTime;

    Vector3 minePos = Transform().Position();

    minePos.y = mBaseY + 0.7f * std::sin(mOscillationTime);

    Transform().Position() = minePos;
    Transform().RotateLocalY(0.35f * frameTime);

    std::vector<Boat*> boats = gEntityManager->GetAllBoatEntities();

    // Loop through all boats to see if any come within the explosion radius.
    for (Boat* boatPtr : boats)
    {
        // Calculate the squared distance between the mine and the boat.
        Vector3 diff = boatPtr->Transform().Position() - minePos;
        float distanceSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;

        if (distanceSq <= (mExplosionRadius * mExplosionRadius))
        {
            gMessenger->DeliverMessage(GetID(), boatPtr->GetID(), MessageType::MineHit);

            return false;
        }
    }

    return true;
}