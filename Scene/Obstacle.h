/*-----------------------------------------------------------------------------------------
    Entity class for Obstacles
-----------------------------------------------------------------------------------------*/

#ifndef _OBSTACLE_H_INCLUDED_
#define _OBSTACLE_H_INCLUDED_

#include "Entity.h"
#include "EntityTypes.h"
#include "Vector3.h"
#include "Matrix4x4.h"

#include <string>

struct AABB
{
    Vector3 min;
    Vector3 max;
};

class Obstacle : public Entity
{
public:
    // Constructor: The entity template, unique ID, initial transform, a vector repressenting half the width, height, 
    // and depth of the obstacleand and optional name are passed in.
    Obstacle(EntityTemplate& entityTemplate, EntityID id, const Matrix4x4& transform, const std::string& name = "", 
        const Vector3& halfExtents = Vector3(60.0f, 20.0f, 60.0f))
        : Entity(entityTemplate, id, transform, name),
        mHalfExtents(halfExtents)
    {
        // Compute the AABB based on the transform's position.
        Vector3 pos = transform.Position();
        mAABB.min = pos - mHalfExtents;
        mAABB.max = pos + mHalfExtents;
    }

    // Returns the obstacle's axis-aligned bounding box.
    const AABB& GetAABB() const { return mAABB; }

private:
    AABB mAABB; // The axis-aligned bounding box for collision or occlusion tests.
    Vector3 mHalfExtents;  // Half-dimensions of the obstacle.
};

#endif // _OBSTACLE_H_INCLUDED_
