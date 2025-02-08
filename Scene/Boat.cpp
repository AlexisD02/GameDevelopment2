/*-----------------------------------------------------------------------------------------
	Entity class for Boats
-----------------------------------------------------------------------------------------*/

#include "Boat.h"
#include "Input.h"
#include "Messenger.h"
#include "Missile.h"
#include "Shield.h"

#include "SceneGlobals.h" // For gEntityManager and gMessenger
#include "MathHelpers.h"

#include <random>
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>
#include <numbers> // C++20 finally provides the value of PI from the <numbers> header (pi)
using namespace std::numbers; // Don't usually recommend using statements like this, but this one to allow convenient use of standard math values is OK


/*-----------------------------------------------------------------------------------------
   Update / Render
-----------------------------------------------------------------------------------------*/
// Update the entity including message processing for each entity
// ***Entity Update functions should return false if the entity is to be destroyed***
bool Boat::Update(float frameTime)
{
    bool shouldDestroy = false;

    //********************************************************/
    // Message handling
    //********************************************************/
    // Fetch any messages. The Messenger class handles the collection and delivery - we just enter
    // a loop collecting any pending messages, acting accordingly
    Message message;
    while (gMessenger->ReceiveMessage(GetID(), &message))
    {
        switch (message.type)
        {
        case MessageType::Start:
            if (mState == State::Inactive)
            {
                mState = State::Patrol;
                mPatrolPoint = ChooseRandomPointInArea();
            }
            break;

        case MessageType::Evade:
            if (mState != State::Inactive && mState != State::Destroyed) 
            {
                mState = State::Evade;
            }
            break;

        case MessageType::Stop:
            mState = State::Inactive;
            mSpeed = 0.0f;
            break;

        case MessageType::Hit:
        {
            if (mShieldEntityID == NO_ID) {
                MissileHitData hitData = std::get<MissileHitData>(message.data);
                EntityID lastHitByBoatID = hitData.launchingBoatID;
                Boat* hitByBoat = gEntityManager->GetEntity<Boat>(lastHitByBoatID);
                float damage = (hitByBoat) ? hitByBoat->GetMissileDamage() : 20.0f;
                mHP -= damage;

                SetBoatText("-" + std::to_string(static_cast<int>(damage)) + " Health");

                if (mHP <= 0.0f)
                {
                    mState = State::Destroyed;
                }

                BroadcastHelpMessage(hitByBoat);
            }
            else {
                SetBoatText("0 Damage");
            }

            break;
        }

        case MessageType::MineHit:
            if (mShieldEntityID == NO_ID) {
                mHP -= 50.0f;
                SetBoatText("-50 Health");
            }
            else {
                mHP -= 25.0f;
                SetBoatText("-25 Health");
            }

            if (mHP <= 0.0f)
            {
                mState = State::Destroyed;
            }
            else
            {
                mTimer = 2.0f;
                mWigglePhase = 0.0f;
                mLastWiggleAngle = 0.0f;
                mState = State::Wiggle;
            }
            break;

        case MessageType::Help:
            if (Random(0.0f, 1.0f) < 0.5f && mState != State::Aim)
            {
                mState = State::Aim;
                mTimer = 2.0f;
            }
            break;

        case MessageType::Reload:
            mState = State::Reloading;
            break;

        case MessageType::CrateCollected:
        {
            CratePickupData crateData = std::get<CratePickupData>(message.data);

            if (crateData.type == CrateType::Missile)
            {
                AddMissiles(2);
                SetBoatText("+2 Missiles");
            }
            else if (crateData.type == CrateType::Health)
            {
                SetHP(GetHP() + 20.0f);
                SetBoatText("+20 Health");
            }
            else if (crateData.type == CrateType::Shield)
            {
                // If there is already a shield, remove it first.
                if (mShieldEntityID != NO_ID)
                {
                    gEntityManager->DestroyEntity(mShieldEntityID);
                    mShieldEntityID = NO_ID;
                }
                // Attach the new shield.
                AttachShieldMesh();
                mShieldTimer = Random(7.0f, 15.0f);
                SetBoatText("+Shield");
            }

            mTargetCrate = nullptr;
        }
        break;

        case MessageType::TargetPoint:
        {
            TargetPointData targetData = std::get<TargetPointData>(message.data);
            mTargetPoint = targetData.target;
            mTargetRange = targetData.range;
            mState = State::TargetPoint;
        }
        break;

        case MessageType::Die:
            mState = State::Destroyed;
            break;

        default:
            break;
        }
    }

    // Execute behavior based on state.
    switch (mState)
    {
    case State::Inactive:
        mSpeed = 0.0f;
        break;

    case State::Patrol:
        UpdatePatrol(frameTime);
        {
            EntityID enemyID = CheckForEnemy();
            if (enemyID != NO_ID)
            {
                mTimer = 2.0f;
                mSpeed = 0.0f;
                mTargetBoat = enemyID;
                mState = State::Aim;
            }
        }
        break;

    case State::Aim:
        UpdateAim(frameTime);
        mSpeed = 0.0f;
        break;

    case State::Evade:
        UpdateEvade(frameTime);
        break;

    case State::Reloading:
        UpdateReloading(frameTime);
        break;

    case State::TargetPoint:
    {
        Vector3 toTarget = mTargetPoint - Transform().Position();
        if (toTarget.Length() <= mTargetRange) 
        {
            mState = State::Patrol;
        }
        else
        {
            FaceDirection(toTarget, frameTime, mBoatTemplate.mTurnSpeed);
            mSpeed = mBoatTemplate.mMaxSpeed;
        }
    }
    break;

    case State::PickupCrate:
        UpdatePickupCrate(frameTime);
        break;

    case State::Wiggle:
        UpdateWiggle(frameTime);
        break;

    case State::Destroyed:
        DestructionBehaviour(frameTime, shouldDestroy);

        return !shouldDestroy;
    }

    if (mState != State::Aim)
    {
        HandleCollisionAvoidance(frameTime);
        Transform().MoveLocalZ(mSpeed * frameTime);
    }
    UpdateShieldAttachment(frameTime);
    UpdateBoatTextTimer(frameTime);

    return true;
}

//------------------------------------------------------------------------------
// Update patrol behavior.
void Boat::UpdatePatrol(float frameTime)
{
    // If missiles are exhausted, switch to reloading.
    if (mMissilesRemaining <= 0 && !mReloading)
    {
        mState = State::Reloading;
        return;
    }

    // Update gun parts for visual effect.
    Transform(3) = MatrixRotationY(mBoatTemplate.mGunTurnSpeed * frameTime) * Transform(3);
    Transform(4) = MatrixRotationX(std::sin(mTimer * 3.0f) * frameTime) * Transform(4);

    // Move toward the patrol point.
    Vector3 toPatrol = mPatrolPoint - Transform().Position();

    if (toPatrol.Length() < 5.0f)
    {
        mPatrolPoint = ChooseRandomPointInArea();
    }
    else
    {
        FaceDirection(toPatrol, frameTime, mBoatTemplate.mTurnSpeed);
        mSpeed = mBoatTemplate.mMaxSpeed;
    }
    mTimer += frameTime;
}

//------------------------------------------------------------------------------
// Update aim behavior.
void Boat::UpdateAim(float frameTime)
{
    mTimer -= frameTime;
    Boat* enemyPtr = gEntityManager->GetEntity<Boat>(mTargetBoat);
    if (enemyPtr != nullptr)
    {
        Vector3 enemyPos = enemyPtr->Transform().Position();
        Vector3 directionToEnemy = enemyPos - Transform(3).Position();
        Vector3 desiredDirection = Normalise(directionToEnemy);
        Transform(3).FaceDirection(desiredDirection);

        if (mTimer <= 0.0f)
        {
            Vector3 enemyForward = enemyPtr->Transform().ZAxis();
            enemyForward = Normalise(enemyForward);

            float enemySpeed = enemyPtr->Template<BoatTemplate>().mMaxSpeed;
            Vector3 enemyVelocity = enemyForward * enemySpeed;

            float enemyHeight = 10.0f;
            enemyPos.y += enemyHeight;

            Vector3 relativePos = enemyPos - Transform().Position();
            float missileSpeed = 45.0f;

            float interceptTime = relativePos.Length() / missileSpeed;
            Vector3 predictedPos = enemyPos + enemyVelocity * interceptTime;

            Vector3 displacement = predictedPos - Transform().Position();

            float timeToTarget = interceptTime;
            Vector3 initialVelocity;

            if (timeToTarget > 0.0f)
            {
                initialVelocity.x = displacement.x / timeToTarget;
                initialVelocity.z = displacement.z / timeToTarget;

                float gravity = -9.81f; // Gravity acceleration
                initialVelocity.y = (displacement.y - 0.5f * gravity * timeToTarget * timeToTarget) / timeToTarget;
            }
            else
            {
                initialVelocity = Normalise(displacement) * missileSpeed;
            }

            // Create the missile with the calculated velocity
            Vector3 normalizedVelocity = Normalise(initialVelocity);
            Matrix4x4 initialTransform = Matrix4x4(Transform().Position(), Transform().GetRotation(), 1.0f);
            initialTransform.FaceDirection(normalizedVelocity);

            EntityID missileID = gEntityManager->CreateEntity<Missile>("Missile", initialTransform);
            Missile* missile = gEntityManager->GetEntity<Missile>(missileID);
            if (missile)
            {
                missile->SetVelocity(initialVelocity);
                missile->SetLaunchingBoatID(GetID());
            }

            mEvadePoint = ChooseEvadePoint(enemyPos);
            UseMissile();
            mState = State::Evade;
        }
    }
    else {
        mPatrolPoint = ChooseRandomPointInArea();
        mState = State::Patrol;
    }
}

//------------------------------------------------------------------------------
// Update evade behavior.
void Boat::UpdateEvade(float frameTime)
{
    mTimer += frameTime;
    static float mEvadeTimer = 5.0f;
    mEvadeTimer -= frameTime;

    // Rotate gun parts for visual effect.
    Transform(3) = MatrixRotationY(mBoatTemplate.mGunTurnSpeed * frameTime) * Transform(3);
    Transform(4) = MatrixRotationX(std::sin(mTimer * 2.0f) * frameTime) * Transform(4);

    // Move toward the evade point.
    Vector3 toEvade = mEvadePoint - Transform().Position();
    if (toEvade.Length() < 5.0f || mEvadeTimer <= 0.0f)
    {
        mTargetCrate = FindNearestCrate(75.0f);
        mEvadeTimer = 5.0f;
        mState = State::PickupCrate;
    }
    else
    {
        FaceDirection(toEvade, frameTime, mBoatTemplate.mTurnSpeed);
        mSpeed = mBoatTemplate.mMaxSpeed * 2.0f;  // Increase speed when evading.
    }
}

//------------------------------------------------------------------------------
// Update reloading behavior.
void Boat::UpdateReloading(float frameTime)
{
    std::vector<ReloadStation*> reloadStations = gEntityManager->GetAllReloadStationEntities();
    ReloadStation* nearestStation = nullptr;
    float nearestDistance = std::numeric_limits<float>::max();

    // Find the nearest reload station.
    for (ReloadStation* station : reloadStations)
    {
        float distance = (station->Transform().Position() - Transform().Position()).Length();
        if (distance < nearestDistance)
        {
            nearestDistance = distance;
            nearestStation = station;
        }
    }

    if (nearestStation)
    {
        if (nearestDistance > 40.0f)
        {
            // Not close enough to the reload station: move toward it.
            Vector3 toStation = nearestStation->Transform().Position() - Transform().Position();
            FaceDirection(toStation, frameTime, mBoatTemplate.mTurnSpeed);
            mSpeed = mBoatTemplate.mMaxSpeed;
            mTimer = 0.0f;
        }
        else
        {
            // We are close to the reload station: stop moving.
            mSpeed = 0.0f;
            mTimer += frameTime;
            if (mTimer >= 5.0f)
            {
                // After waiting 5 seconds, reload missiles.
                ReloadMissiles();
                mState = State::Patrol;
                mTimer = 0.0f;
                mReloading = false;
            }
        }
    }
    else
    {
        // If no reload station is found, simply stop and resume patrol.
        mSpeed = 0.0f;
        mState = State::Patrol;
        mReloading = false;
        mTimer = 0.0f;
    }
}

void Boat::UpdatePickupCrate(float frameTime)
{
    if (mTargetCrate && gEntityManager->GetEntity<RandomCrate>(mTargetCrate->GetID()))
    {
        // Calculate the distance to the crate.
        Vector3 cratePos = mTargetCrate->Transform().Position();
        Vector3 toCrate = cratePos - Transform().Position();
        float distance = toCrate.Length();

        // If close enough, pick up the crate.
        if (distance < mTargetCrate->collisionRadius)
        {
            mState = State::Patrol;
        }
        else
        {
            // Otherwise, steer toward the crate.
            FaceDirection(toCrate, frameTime, mBoatTemplate.mTurnSpeed);
            mSpeed = mBoatTemplate.mMaxSpeed;
        }
    }
    else {
        // If none is found, return to patrol.
        mState = State::Patrol;
        return;
    }
}

void Boat::UpdateBoatTextTimer(float frameTime)
{
    if (mBoatTextTimer > 0.0f)
    {
        mBoatTextTimer -= frameTime;
        if (mBoatTextTimer <= 0.0f)
        {
            mBoatText.clear();
        }
    }
}

void Boat::UpdateWiggle(float frameTime)
{
    mTimer -= frameTime;

    if (mTimer <= 0.0f)
    {
        mState = State::Patrol;
        Transform().RotateLocalZ(-mLastWiggleAngle);
        mLastWiggleAngle = 0.0f;

        return;
    }

    // Wiggle speed controls how fast we oscillate
    float wiggleSpeed = 8.0f;
    mWigglePhase += wiggleSpeed * frameTime;

    float amplitude = 0.1f;
    float newAngle = std::sin(mWigglePhase) * amplitude;

    // Delta is how much we need to rotate this frame
    float deltaAngle = newAngle - mLastWiggleAngle;
    Transform().RotateLocalZ(deltaAngle);

    mLastWiggleAngle = newAngle;
    mSpeed = mBoatTemplate.mMaxSpeed * 0.2f;
}

void Boat::UpdateShieldAttachment(float frameTime)
{
    if (mShieldEntityID != NO_ID)
    {
        // Decrement the shield timer.
        mShieldTimer -= frameTime;
        if (mShieldTimer <= 0.0f)
        {
            // Destroy the shield entity and clear the ID.
            gEntityManager->DestroyEntity(mShieldEntityID);
            mShieldEntityID = NO_ID;
        }
        else
        {
            // Update the shield's position so that it remains attached
            Shield* shield = gEntityManager->GetEntity<Shield>(mShieldEntityID);
            if (shield)
            {
                // Compute the new shield position (boat position plus an offset)
                Vector3 newShieldPos = Transform().Position();
                shield->Transform().Position() = newShieldPos;
            }
        }
    }
}

//------------------------------------------------------------------------------
// Destruction behavior: animate sinking before destruction.
void Boat::DestructionBehaviour(float frameTime, bool& shouldDestroy)
{
    static float sinkingAnimationTime = 5.0f;
    if (sinkingAnimationTime > 0.0f)
    {
        sinkingAnimationTime -= frameTime;
        Transform().RotateLocalZ(frameTime * 0.3f);
        Transform().RotateLocalX(frameTime * 0.3f);
        Transform().MoveLocalY(-3.0f * frameTime);
        shouldDestroy = false;
    }
    else
    {
        shouldDestroy = true;
        sinkingAnimationTime = 5.0f;
    }
}

//------------------------------------------------------------------------------
// Collision avoidance: adjust heading if another boat is too close.
void Boat::HandleCollisionAvoidance(float frameTime)
{
    static float kSafeDistance = 40.0f;    // If another boat is within 40 units, we steer away
    static float kThreatDistance = 120.0f;   // If another boat is within 120 units and within certain angle => immediate threat
    static float kThreatAngleDeg = 50.0f;    // If boat is in front within ±60°, treat as immediate threat

    // Steering strength
    static float kAvoidStrength = 2.5f;     // Scales how strongly we steer away
    static float kForwardWeight = 0.50f;    // Weight of our current forward direction in final steering
    static float kAvoidWeight = 0.50f;    // Weight of avoidance direction
    static float kTurnMultiplier = 1.5f;     // Boost normal turn speed for regular avoidance
    static float kThreatSpeedCap = 12.0f;    // If threatened, clamp top speed
    static constexpr float kDirectionLerpFactor = 0.2f; // 20% each frame

    std::vector<Boat*> allBoats = gEntityManager->GetAllBoatEntities(GetID());
    Vector3 myPos = Transform().Position();
    Vector3 myForward = Transform().ZAxis();
    myForward.y = 0.0f;
    myForward = Normalise(myForward);

    bool immediateThreatDetected = false;
    Vector3 boatAvoidance(0.0f, 0.0f, 0.0f);

    for (Boat* otherBoat : allBoats)
    {
        Vector3 offset = otherBoat->Transform().Position() - myPos;
        float dist = offset.Length();
        if (dist < 0.0001f) continue;

        if (dist < kThreatDistance)
        {
            Vector3 offsetNorm = offset / dist;
            float dotVal = Dot(myForward, offsetNorm);
            if (dotVal > std::cos(ToRadians(kThreatAngleDeg)))
            {
                immediateThreatDetected = true;
            }
        }

        // Regular Avoidance if boat is within kSafeDistance
        if (dist < kSafeDistance)
        {
            // Steer away from the other boat
            Vector3 awayFromOther = -OffsetNorm(offset, dist); // see helper below
            // Weight it by how close we are (closer => stronger)
            float proximityFactor = 1.0f - (dist / kSafeDistance);
            boatAvoidance += awayFromOther * proximityFactor;
        }
    }

    std::vector<Obstacle*> obstacles = gEntityManager->GetAllObstacleEntities();
    Vector3 obstacleAvoidance(0.0f, 0.0f, 0.0f);
    float obstacleSafeDistance = 50.0f; // Set the safe distance from obstacles
    for (Obstacle* obs : obstacles)
    {
        if (!obs) continue;
        const AABB& box = obs->GetAABB();
        // Compute the center of the AABB
        Vector3 obsCenter = (box.min + box.max) * 0.5f;
        float distToObs = (myPos - obsCenter).Length();
        if (distToObs < obstacleSafeDistance)
        {
            // The closer the boat is to the obstacle, the stronger the repulsion.
            float factor = 1.0f - (distToObs / obstacleSafeDistance);
            // Calculate a vector away from the obstacle center.
            obstacleAvoidance += Normalise(myPos - obsCenter) * factor;
        }
    }

    Vector3 totalAvoidance = boatAvoidance + obstacleAvoidance;

    if (totalAvoidance.Length() > 0.0001f)
    {
        // Normalize the avoidance
        totalAvoidance = (totalAvoidance / totalAvoidance.Length()) * kAvoidStrength;

        // Blend with our forward direction
        Vector3 desiredDir = (myForward * kForwardWeight) + (totalAvoidance * kAvoidWeight);
        desiredDir.y = 0.0f;
        desiredDir = Normalise(desiredDir);

        Vector3 currentDir = myForward;
        Vector3 smoothedDir = Lerp(currentDir, desiredDir, kDirectionLerpFactor);
        smoothedDir = Normalise(smoothedDir);

        // If immediate threat, turn faster & reduce speed
        float finalTurnSpeed = mBoatTemplate.mTurnSpeed * kTurnMultiplier;
        if (immediateThreatDetected)
        {
            // Optionally clamp or reduce speed
            mSpeed = std::min(mSpeed, kThreatSpeedCap);
        }

        // Finally, rotate the boat
        FaceDirection(smoothedDir, frameTime, finalTurnSpeed);
    }
}

RandomCrate* Boat::FindNearestCrate(float maxDistance)
{
    std::vector<RandomCrate*> crates = gEntityManager->GetAllCratesEntities();
    Vector3 boatPos = Transform().Position();
    RandomCrate* nearestCrate = nullptr;

    for (RandomCrate* crate : crates)
    {
        Vector3 cratePos = crate->Transform().Position();
        float distance = (cratePos - boatPos).Length();
        if (distance <= maxDistance)
        {
            maxDistance = distance;
            nearestCrate = crate;
        }
    }
    return nearestCrate;
}

bool Boat::IntersectLineAABB(const Vector3& start, const Vector3& end, const AABB& box)
{
    Vector3 dir = end - start;
    Vector3 invDir(1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z);

    float t1 = (box.min.x - start.x) * invDir.x;
    float t2 = (box.max.x - start.x) * invDir.x;
    float tmin = std::min(t1, t2);
    float tmax = std::max(t1, t2);

    t1 = (box.min.y - start.y) * invDir.y;
    t2 = (box.max.y - start.y) * invDir.y;
    tmin = std::max(tmin, std::min(t1, t2));
    tmax = std::min(tmax, std::max(t1, t2));

    t1 = (box.min.z - start.z) * invDir.z;
    t2 = (box.max.z - start.z) * invDir.z;
    tmin = std::max(tmin, std::min(t1, t2));
    tmax = std::min(tmax, std::max(t1, t2));

    if (tmax < 0 || tmin > tmax)
        return false;

    // Optionally ensure the intersection falls within the segment [0,1]:
    if (tmin > 1.0f || tmax < 0.0f)
        return false;

    return true;
}

bool Boat::IsLineOfSightBlocked(const Vector3& start, const Vector3& end)
{
    std::vector<Obstacle*> obstacles = gEntityManager->GetAllObstacleEntities();
    for (Obstacle* obs : obstacles)
    {
        if (!obs) continue;
        if (IntersectLineAABB(start, end, obs->GetAABB()))
        {
            return true;
        }
    }
    return false;
}

//------------------------------------------------------------------------------
// Helper: Rotate the boat gradually to face the given direction in the XZ plane.
void Boat::FaceDirection(const Vector3& dir, float dt, float turnSpeed)
{
    Vector3 flatDir = dir;
    flatDir.y = 0.0f;
    flatDir = Normalise(flatDir);

    Vector3 forward = Transform().ZAxis();
    forward.y = 0.0f;
    forward = Normalise(forward);

    float dotVal = Dot(forward, flatDir);
    dotVal = std::clamp(dotVal, -1.0f, 1.0f);
    float angle = std::acos(dotVal);

    float crossY = Cross(forward, flatDir).y;
    float turnDir = (crossY > 0.0f) ? 1.0f : -1.0f;

    float maxTurn = turnSpeed * dt;
    if (angle > maxTurn) {
        angle = maxTurn;
    }

    Matrix4x4 partialRot = MatrixRotationY(turnDir * angle);
    Vector3 newForward = partialRot.TransformVector(forward);
    Transform().FaceDirection(newForward);
}

//------------------------------------------------------------------------------
// Choose a random patrol point (for example, within a 500x500 area at water level).
Vector3 Boat::ChooseRandomPointInArea()
{
    float x = Random(-500.0f, 500.0f);
    float z = Random(-500.0f, 500.0f);
    return Vector3(x, -1.5f, z);
}

//------------------------------------------------------------------------------
// Choose an evade point relative to an enemy's position.
Vector3 Boat::ChooseEvadePoint(const Vector3& enemyPos)
{
    const float minDist = 50.0f;
    const float maxDist = 80.0f;
    const float avoidCone = 45.0f;

    Vector3 myPos = Transform().Position();
    Vector3 toEnemy = enemyPos - myPos;
    toEnemy.y = 0.0f;
    toEnemy = Normalise(toEnemy);

    Vector3 chosenOffset;
    bool acceptable = false;
    for (int i = 0; i < 10; ++i)
    {
        float angle = Random(0.0f, 2.0f * static_cast<float>(pi));
        float dist = Random(minDist, maxDist);

        Vector3 offset{ dist * std::cos(angle), 0.0f, dist * std::sin(angle) };
        Vector3 offsetDir = Normalise(offset);

        float angleDeg = std::acos(std::clamp(Dot(offsetDir, toEnemy), -1.0f, 1.0f)) * (180.0f / pi);
        
        if (angleDeg < avoidCone) continue;

        chosenOffset = offset;
        acceptable = true;
        break;
    }
    if (!acceptable)
    {
        float angle = Random(0.0f, 2.0f * static_cast<float>(pi));
        float dist = Random(minDist, maxDist);
        chosenOffset = { dist * std::cos(angle), 0.0f, dist * std::sin(angle) };
    }

    return myPos + chosenOffset;
}

//------------------------------------------------------------------------------
// Check for an enemy boat (from a different team) in front within a given angle and distance.
EntityID Boat::CheckForEnemy()
{
    // Retrieve all Boat entities except self
    std::vector<Boat*> otherBoats = gEntityManager->GetAllBoatEntities(GetID());

    // Get this boat's forward direction in XZ plane
    Vector3 forward = Transform().ZAxis();
    forward.y = 0.0f;
    forward = Normalise(forward);

    for (Boat* enemyBoat : otherBoats)
    {
        if (enemyBoat)
        {
            if (!enemyBoat || enemyBoat->GetTeam() == GetTeam())
                continue;

            Vector3 boatPos = Transform().Position();
            Vector3 enemyBoatPos = enemyBoat->Transform().Position();
            if (!IsLineOfSightBlocked(boatPos, enemyBoatPos))
            {
                Vector3 toEnemy = enemyBoatPos - boatPos;
                float distance = toEnemy.Length();

                // Check if within 140 units
                if (distance > 140.0f) continue;

                // Normalize toEnemy vector
                Vector3 toEnemyNorm = Normalise(toEnemy);

                // Calculate the angle between forward and toEnemy vectors using dot product
                float dotProduct = Dot(forward, toEnemyNorm);
                dotProduct = std::clamp(dotProduct, -1.0f, 1.0f); // Clamp to avoid numerical issues
                float angle = std::acos(dotProduct) * (180.0f / pi); // Convert radians to degrees

                // Check if within 70 degrees
                if (angle <= 70.0f)
                {
                    // Enemy detected
                    return enemyBoat->GetID();
                }
            }
        }
    }

    // No enemy detected
    return NO_ID;
}

//------------------------------------------------------------------------------
// Broadcast a help message to team-mates.
void Boat::BroadcastHelpMessage(Boat* enemyEntity)
{
    if (!enemyEntity) return; 

    Vector3 enemyPos = enemyEntity->Transform().Position();

    // Get all teammate boats (all boats except self).
    std::vector<Boat*> teammates = gEntityManager->GetAllBoatEntities(GetID());
    for (Boat* mate : teammates)
    {
        if (!mate) continue;

        // Check that the boat is on the same team.
        if (mate && mate->GetTeam() == mTeam)
        {
            // Calculate the distance between this teammate and the enemy.
            Vector3 matePos = mate->Transform().Position();
            float distanceToEnemy = (matePos - enemyPos).Length();

            // Only deliver the help message if within 400 units.
            if (distanceToEnemy <= 400.0f)
            {
                gMessenger->DeliverMessage(GetID(), mate->GetID(), MessageType::Help);
            }
        }
    }
}

void Boat::AttachShieldMesh()
{
    // Example: Create a new shield entity as a child of this boat.
    Matrix4x4 shieldTransform = Transform(); // You might adjust the transform so that the shield appears above the boat.
    // Optionally, offset the shield slightly upward.
    shieldTransform.MoveLocalY(2.0f);

    // Create the shield entity (ensure "Shield" is a valid template).
    EntityID shieldID = gEntityManager->CreateEntity<Shield>("Shield", shieldTransform);

    // Store the shield entity ID if needed, or mark a flag that the boat is shielded.
    mShieldEntityID = shieldID;
}
