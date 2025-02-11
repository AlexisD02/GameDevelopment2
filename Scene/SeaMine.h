#ifndef _SEAMINE_H_INCLUDED_
#define _SEAMINE_H_INCLUDED_

#include "Entity.h" 
#include "EntityTypes.h"
#include "Vector3.h"
#include "Matrix4x4.h"
#include <string>

class SeaMine : public Entity
{
public:
    // Constructor: Pass in the entity template, unique ID, initial transform, and optional name.
    SeaMine(EntityTemplate& entityTemplate, EntityID id, const Matrix4x4& transform)
        : Entity(entityTemplate, id, transform)
    {}

public:
    // Update is called every frame. Returns false when the mine should be destroyed.
    virtual bool Update(float frameTime) override;

private:
    float mOscillationTime = 0.0f;
    float mBaseY = -11.5f; // Target surface level
    float mExplosionRadius = 15.0f;

    // Rising phase variables
    float mRiseTimer = 0.0f;
    float mRiseDuration = 2.0f; // Time to rise (seconds)
    float mStartY = -20.0f; // Initial depth
};

#endif // _SEAMINE_H_INCLUDED_
