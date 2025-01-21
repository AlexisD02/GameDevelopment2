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


/*-----------------------------------------------------------------------------------------
	Missile Entity Class
-----------------------------------------------------------------------------------------*/
class Missile : public Entity // entity classes must inherit from Entity
{
	/*-----------------------------------------------------------------------------------------
	   Constructor
	-----------------------------------------------------------------------------------------*/
public:
	// Entity constructor parameters must begin with EntityTemplate and ID, used to construct the base class.
	// Further parameters are allowed if required. Since the base class also has optional name and transform
	// parameters we add those two parameters at the end also even though not strictly required
	Missile(EntityTemplate& entityTemplate, EntityID ID, const Matrix4x4& transform = Matrix4x4::Identity, const std::string& name = "")
		: Entity(entityTemplate, ID, transform, name)
	{

	}


	/*-----------------------------------------------------------------------------------------
	   Update / Render
	-----------------------------------------------------------------------------------------*/
public:
	// Update the shell - perform simple shell behaviour as described in the assignment brief
	// ***Entity Update functions should return false if the entity is to be destroyed***
	virtual bool Update(float frameTime);


	/*-----------------------------------------------------------------------------------------
	   Private data
	-----------------------------------------------------------------------------------------*/
private:
	// Add your shell data here
};


#endif //_MISSILE_H_INCLUDED_
