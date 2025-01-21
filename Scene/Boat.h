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
#include "Vector3.h"


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
		         float maxHP, float missileDamage, ImportFlags importFlags = {})
		: EntityTemplate(type, meshFilename, importFlags),
		  mMaxSpeed(maxSpeed), mAcceleration(acceleration), mTurnSpeed(turnSpeed), mGunTurnSpeed(gunTurnSpeed),
		  mMaxHP(maxHP), mMissileDamage(missileDamage) {} // No construction code, just call base class constructor and copy over all the stats to member variables


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
		 const Matrix4x4& transform = Matrix4x4::Identity, const std::string& name = "")
		: Entity(entityTemplate, ID, transform, name)
	{
		auto& boatTemplate = static_cast<BoatTemplate&>(entityTemplate);
		mHP = boatTemplate.mMaxHP;

		mSpeed = initSpeed;
		if (mSpeed > boatTemplate.mMaxSpeed)  mSpeed = boatTemplate.mMaxSpeed;

		mState = State::Bounce;
		mTimer = 0;
	}


	/*-----------------------------------------------------------------------------------------
	   Getters
	-----------------------------------------------------------------------------------------*/
public:
	float GetSpeed()
	{
		return mSpeed;
	}

	std::string GetState()
	{
		switch (mState)
		{
		case State::Stop:    return "Stop";   break;
		case State::Bounce:  return "Bounce"; break;
		}
		return "?";
	}


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
		Stop,
		Bounce,
	};


	/*-----------------------------------------------------------------------------------------
	   Private data
	-----------------------------------------------------------------------------------------*/
private:
	// Boat state
	float mSpeed; // Current speed (in facing direction)
	float mHP;    // Current hit points for the boat

	State mState; // Current state - different states trigger different behaviour code in the Update function
	float mTimer; // A timer used in the example Update function   
};


#endif //_BOAT_H_INCLUDED_
