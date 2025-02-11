/*-----------------------------------------------------------------------------------------
	Entity Template class and Entity class for Boats
-----------------------------------------------------------------------------------------*/
// The class Boat (entity) is written below the BoatTemplate class


// Additional technical notes for the assignment:

// - There are global variables gEntityManager and gMessenger available. They work in the same
//   way as the Car Tag lab. There is some sample usage of both in the boat Update function below.
//   This sample code is not required for the assignment and once you have understood it you may
//   remove it (commenting it out might be better)
//
// - The Boat.h header file contains the definitions of the boat template and boat classes. Read it!
//   Both classes inherit from base classes so also read Entity.h to see what functions they inherit
//
// - The boat template class contains boat statistics that you should access for some assignment
//   options. The boat is a friend class of the boat template so it can access the template member
//   data directly and there is no need for getters. E.g. float maxSpeed = Template()->mMaxSpeed;
//
// - There is sample code to show how to find an enemy boat by looking for boats using a different
//   template. If you try the advanced tasks you may need to improve this code. You may even want to
//   change how the entity manager works if you go for very advanced solutions
//
// - Boats have five parts: Root, Boat and GunBase, GunMount and Barrels. They are parented to each
//   other. Each part has its own transformation matrix, accessed with the Transform function:
//   Root: Transform(), Boat: Transform(1), GunBase: Transform(2), GunMount: Transform(3), Barrels: Transform(4)
//   You can rotate the different parts by using matrix rotation functions on the appropriate transform
//   as shown in the example code
//
// - Vector work similar to the car tag lab will be needed for the boat->enemy facing requirements
//   in the Patrol state. Use the Matrix4x4 and Vector functions - look at their .h files
//
// - The Shell class is simply an outline. To support shell firing, you will need to add member data
//   to it and add code to its constructor & update functions
//
// - Destroy an entity by returning false from its Update function - the entity manager wil perform
//   the destruction. Don't try to call DestroyEntity from within the Update function.
// 
// - As entities can be destroyed you must check that entity IDs refer to existant entities before
//   using their entity pointers. The return value from EntityManager->GetEntity(ID) will be nullptr
//   if the entity no longer exists. Use this to avoid working with boats that no longer exist


#ifndef _BOAT_H_INCLUDED_
#define _BOAT_H_INCLUDED_

#include "Entity.h"
#include "EntityTypes.h"
#include "RandomCrate.h"
#include "Vector3.h"
#include "Matrix4x4.h"

struct AABB;

enum class Team : int 
{
    TeamA = 0,
    TeamB,
    TeamC
};

inline const char* const teamNames[] = { "Team A", "Team B", "Team C" };

/*-----------------------------------------------------------------------------------------
	Boat Entity Template Class
-----------------------------------------------------------------------------------------*/
class BoatTemplate : public EntityTemplate // entity templates must inherit from EntityTemplate class
{
	friend class Boat; // Allow Boat entity class access to private BoatTemplate data - use friend sparingly, but justified here for related classes
	                   // Do the same in any entity template classes you create

	/*-----------------------------------------------------------------------------------------
	   Constructor
	-----------------------------------------------------------------------------------------*/
public:
	// Entity template constructor parameters must begin with parameters: type amd meshFilename, used to construct the base class.
	// Further parameters are allowed if required - here several parameters to intialise base stats for this kind of boat
	// Must include importFlags as last parameter regardless
    BoatTemplate(const std::string& type, const std::string& meshFilename,
        float maxSpeed, float acceleration, float turnSpeed, float gunTurnSpeed,
        float maxHP, float missileDamage, Team team, ImportFlags importFlags = {})
        : EntityTemplate(type, meshFilename, importFlags),
        mMaxSpeed(maxSpeed), mAcceleration(acceleration), mTurnSpeed(turnSpeed),
        mGunTurnSpeed(gunTurnSpeed), mMaxHP(maxHP), mMissileDamage(missileDamage),
        mTeam(team)
    {}


	/*-----------------------------------------------------------------------------------------
	   Private data
	-----------------------------------------------------------------------------------------*/
private:
	// Statistics common to all boats of this type
	// There are no getters in this class, but the Boat class is a friend, so boats can access all of these variables directly, e.g. Template()->mMaxSpeed
    float mMaxSpeed;
    float mAcceleration;
    float mTurnSpeed;
    float mGunTurnSpeed;
    float mMaxHP;
    float mMissileDamage;
    Team mTeam;
};



/*-----------------------------------------------------------------------------------------
	Boat Entity Class 
-----------------------------------------------------------------------------------------*/
class Boat : public Entity // entity classes must inherit from Entity
{
	/*-----------------------------------------------------------------------------------------
	   Constructor
	-----------------------------------------------------------------------------------------*/
public:
	// Entity constructor parameters must begin with EntityTemplate and ID, used to construct the base class.
	// Further parameters are allowed if required. Here we set an initial speed for the new boat.
	// Since the base class also has optional transform and name parameters you should also add those two parameters at the end
	// at the end also even though not strictly required
    Boat(EntityTemplate& entityTemplate, EntityID ID, float initSpeed,
        const Matrix4x4& transform, const std::string& name = "")
        : Entity(entityTemplate, ID, transform, name),
        mBoatTemplate(static_cast<BoatTemplate&>(entityTemplate)),
        mTeam(mBoatTemplate.mTeam)
    {
        mHP = mBoatTemplate.mMaxHP;
        mSpeed = initSpeed;
        if (mSpeed > mBoatTemplate.mMaxSpeed)  mSpeed = mBoatTemplate.mMaxSpeed;

        mDoubleSpeed = mBoatTemplate.mMaxSpeed * 2;
        mState = State::Inactive;
        mTimer = 0.0f;
        mMissileDamage = mBoatTemplate.mMissileDamage;
        mMissilesRemaining = 10;
        mReloading = false;
    }


    /*-----------------------------------------------------------------------------------------
       Getters
    -----------------------------------------------------------------------------------------*/
public:
    int GetMissilesFired() { return mMissilesFired; }
    float GetHP() { return mHP; }
    float GetSpeed() { return mSpeed; }
    float GetDoubleSpeed() { return mDoubleSpeed; }
    float GetMissileDamage() { return mMissileDamage; }
    Team GetTeam() { return mTeam; }
    std::string GetState()
    {
        switch (mState)
        {
        case State::Inactive: return "Inactive";
        case State::Patrol:   return "Patrol";
        case State::Aim:      return "Aim";
        case State::Evade:    return "Evade";
        case State::Reloading:    return "Reloading";
        case State::Destroyed:    return "Destroyed";
        case State::TargetPoint:    return "TargetPoint";
        case State::PickupCrate:    return "PickupCrate";
        case State::Wiggle:    return "Wiggle";
        }
        return "?";
    }

    inline const char* GetTeamName() {
        return teamNames[static_cast<int>(mTeam)];
    }

    // Setters
    void SetTeam(Team team) { mTeam = team; }
    void SetHP(float HP) { mHP = HP; }
    void SetSpeed(float speed) { mSpeed = speed; }

    // Missile operations
    void UseMissile()
    {
        if (mMissilesRemaining > 0)
        {
            --mMissilesRemaining;
            ++mMissilesFired;
        }
    }
    int GetMissilesRemaining() const { return mMissilesRemaining; }
    void ReloadMissiles() { mMissilesRemaining = 10; }
    void AddMissiles(unsigned int missiles) { mMissilesRemaining += missiles; }

    void SetBoatText(const std::string& text)
    {
        mBoatText = text;
        mBoatTextTimer = 3.0f;
    }

    const std::string& GetBoatText() const { return mBoatText; }
    float GetBoatTextTimer() const { return mBoatTextTimer; }

    /*-----------------------------------------------------------------------------------------
       Update / Render
    -----------------------------------------------------------------------------------------*/
public:
    // Update the entity including message processing for each entity. Virtual function - using polymorphism
    // ***Entity Update functions should return false if the entity is to be destroyed***
    virtual bool Update(float frameTime);


	/*-----------------------------------------------------------------------------------------
	   Private types
	-----------------------------------------------------------------------------------------*/
private:
	// States available for boats
    enum class State
    {
        Inactive,
        Patrol,
        Aim,
        Evade,
        Reloading,
        Destroyed,
        TargetPoint,
        PickupCrate,
        Wiggle
    };

    // Helper functions for state behavior.
    void UpdatePatrol(float frameTime);
    void UpdateAim(float frameTime);
    void UpdateEvade(float frameTime);
    void UpdateReloading(float frameTime);
    void UpdateTargetPoint(float frameTime);
    void UpdatePickupCrate(float frameTime);
    void UpdateBoatTextTimer(float frameTime);
    void UpdateWiggle(float frameTime);

    // Internal helpers
    Vector3 ChooseRandomPointInArea();
    Vector3 ChooseEvadePoint(const Vector3& enemyPos);
    void FaceDirection(const Vector3& dir, float dt, float turnSpeed);
    EntityID CheckForEnemy(); // Return first boat ID seen
    void BroadcastHelpMessage(Boat* enemyEntity);
    void DestructionBehaviour(float frameTime, bool& shouldDestroy);
    void HandleCollisionAvoidance(float frameTime);
    RandomCrate* FindNearestCrate(float maxDistance);
    bool IntersectLineAABB(const Vector3& start, const Vector3& end, const AABB& box);
    bool IsLineOfSightBlocked(const Vector3& start, const Vector3& end);
    void AttachShieldMesh();


    /*-----------------------------------------------------------------------------------------
   Private data
    -----------------------------------------------------------------------------------------*/
private:
    // Boat state
    BoatTemplate& mBoatTemplate; // Reference to the boat template
    float mSpeed; // Current speed (in facing direction)
    float mDoubleSpeed; // Speed multiplier
    float mHP; // Current hit points for the boat
    float mTimer; // General-purpose timer for updates
    float mMissileDamage; // Damage dealt per missile
    int mMissilesFired = 0; // Count of fired missiles
    State mState; // Current state affecting behavior in Update function
    Team mTeam; // Team affiliation of the boat
    int mMissilesRemaining; // Number of missiles left
    bool mReloading; // Flag indicating if the boat is reloading
    float mWigglePhase = 0.0f; // Controls movement oscillation
    float mLastWiggleAngle = 0.0f; // Stores last wiggle angle

    Vector3 mPatrolPoint; // Destination point for patrol behavior
    Vector3 mEvadePoint; // Destination point for evasion
    Vector3 mTargetPoint; // Current target position
    float mTargetRange = 5.0f; // Distance within which the target is considered reached
    std::string mBoatText; // Text displayed for the boat
    float mBoatTextTimer = 0.0f; // Timer controlling boat text display

    RandomCrate* mTargetCrate = nullptr; // Pointer to the crate being targeted
    EntityID mShieldEntityID = NO_ID; // ID of the shield entity (if applicable)
    float mShieldTimer = 0.0f; // Duration for which the shield remains active

    EntityID mTargetBoat = NO_ID; // ID of the enemy boat being targeted
};


#endif //_BOAT_H_INCLUDED_
