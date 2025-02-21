/*-----------------------------------------------------------------------------------------
	Entity class for Missiles
-----------------------------------------------------------------------------------------*/
// No need for a template class for a missile - there are no generic features for shells
// other than their mesh so they can use the base entity template class

// The missile entity class contains no behaviour and must be rewritten as one of the assignment
// requirements. As well as code you should add anything else you need, member variables,
// constructor parameters, getters etc., although the Missile class will be a lot simpler
// than the Boat class so don't overdo it

#ifndef _MISSILE_H_INCLUDED_
#define _MISSILE_H_INCLUDED_

#include "Entity.h"
#include "Vector3.h"
#include "Matrix4x4.h"

#include <string>

/*-----------------------------------------------------------------------------------------
    Missile Entity Class
-----------------------------------------------------------------------------------------*/
class Missile : public Entity // entity classes must inherit from Entity
{
    /*-----------------------------------------------------------------------------------------
       Constructor
    -----------------------------------------------------------------------------------------*/
public:
    // Entity constructor parameters must begin with EntityTemplate and ID.
    Missile(EntityTemplate& entityTemplate, EntityID ID, const Matrix4x4& transform = Matrix4x4::Identity, 
        float speed = 45.0f, Vector3 velocity = Vector3(0.0f, 0.0f, 0.0f), EntityID boatID = NO_ID)
        : Entity(entityTemplate, ID, transform),
        mSpeed(speed), mVelocity(velocity), mLaunchingBoatID(boatID)
    {}

    /*-----------------------------------------------------------------------------------------
       Update / Render
    -----------------------------------------------------------------------------------------*/
public:
    // Update the missile (simple projectile motion and collision checks)
    virtual bool Update(float frameTime) override;

    // Setter for velocity
    void SetVelocity(const Vector3& newVelocity) {
        mVelocity = newVelocity;
    }

    // Setter for launching boat ID
    void SetLaunchingBoatID(const EntityID& boatID) {
        mLaunchingBoatID = boatID;
    }

    /*-----------------------------------------------------------------------------------------
       Getters
    -----------------------------------------------------------------------------------------*/
    float GetSpeed() { return mSpeed; }

    /*-----------------------------------------------------------------------------------------
       Private data
    -----------------------------------------------------------------------------------------*/
private:
    float mSpeed = 45.0f;  // Speed of the missile
    Vector3 mVelocity = Vector3(0.0f, 0.0f, 0.0f); // Current velocity of the missile
    EntityID mLaunchingBoatID = NO_ID;    // ID of the launching boat
};


#endif //_MISSILE_H_INCLUDED_
