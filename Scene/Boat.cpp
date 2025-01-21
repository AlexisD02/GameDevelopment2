/*-----------------------------------------------------------------------------------------
	Entity class for Boats
-----------------------------------------------------------------------------------------*/

#include "Boat.h"
#include "Input.h"

#include "SceneGlobals.h" // For gEntityManager and gMessenger

#include <cmath>
#include <numbers>            // C++20 finally provides the value of PI from the <numbers> header (pi)
using namespace std::numbers; // Don't usually recommend using statements like this, but this one to allow convenient use of standard math values is OK


/*-----------------------------------------------------------------------------------------
   Update / Render
-----------------------------------------------------------------------------------------*/
// Update the entity including message processing for each entity
// ***Entity Update functions should return false if the entity is to be destroyed***
bool Boat::Update(float frameTime)
{
	//********************************************************/
	// Message handling
	//********************************************************/
	// Fetch any messages. The Messenger class handles the collection and delivery - we just enter
	// a loop collecting any pending messages, acting accordingly
	Message message;
	while (gMessenger->ReceiveMessage(GetID(), &message))
	{
		// Update monster state based on messages received
		switch (message.type)
		{

		case MessageType::Unpause:
			mState = State::Bounce; // For the example code, boats go into Bounce state when the game is started - need to change this to suit assignment requirements
			break;

		case MessageType::Pause:
			mState = State::Stop;
			break;

		// Example of receiving a message with a payload of data - example code to show how things work only
		case MessageType::TargetPoint: 
		{ // You need curly brackets if you declare variables in a case statement
			EntityID fromID = message.from;
			auto& targetData = std::get<TargetPointData>(message.data); // Example of how to access a message payload. Use the correct type in the angle brackets
			Vector3f target = targetData.target;
			break;
		}

		// Die message makes entity destroy itself immediately
		case MessageType::Die:
			return false;

		}
	}

	//********************************************************/
	// Boat behaviour
	//********************************************************/
	// Behaviour for boat depends on its "state"

	// Get the boat's template data - we cast the base class template to a boat template to access boat-specific base stats
	//**** currently unused variable, but will be convenient for the assignemnt
	auto& boatTemplate = static_cast<BoatTemplate&>(Template());



	//---------------------------------------------------------
	// Stop state: no movement
	//---------------------------------------------------------
	// Only run this code if boat is in Stop state
	if (mState == State::Stop)
	{
		mSpeed = 0;
	}



	//---------------------------------------------------------
	// Bounce state: move backwards and forwards - Bounce is an example state - it should should be deleted for the assignment
	//---------------------------------------------------------
	// Only run this code if boat is in Bounce state
	if (mState == State::Bounce)
	{
		// Cycle speed up and down using a sine wave - just demonstration behaviour
		//**** Variations on this sine wave code does not count as patrolling - for the
		//**** assignment the boat must move naturally between two specific points
		mSpeed = 10 * std::sin(mTimer * 4.0f);
		mTimer += frameTime;

		// Example of how to rotate gun - here rotating Barrels (node 4) up and down on sine wave (see comments at top)
		Transform(4) = MatrixRotationX(std::sin(mTimer * 4.0f) * frameTime) * Transform(4); // Local rotation, for world rotation swap the multiply the other way round
	}


	//---------------------------------------------------------
	// Example code - only to show how to use features - delete this for the assignment
	//---------------------------------------------------------

	// Example code of sending a message - delete this when you understand it!
	if (KeyHeld(Key_X))
	{
		// Parameters are: own ID, recipient ID, message type, [if required]message payload
		gMessenger->DeliverMessage(GetID(), 2, MessageType::TargetPoint, TargetPointData{Vector3{100, 0, 0}, 5.0f}); // See how the payload data of TargetPoint is sent (see Messenger.h)
		gMessenger->DeliverMessage(GetID(), GetID(), MessageType::Die); // Send death message to self!
	}


	// Example of working with entity template types
	if (KeyHeld(Key_Y))
	{
		std::string enemy;
		if (Template().GetType() == "Blue Tanker")  enemy = "Green Fleeter"; // This gets an enemy, but will need to be improved to work with three or more boats
		else                                        enemy = "Blue Tanker";
		auto enemyTemplate = gEntityManager->GetTemplate(enemy);
		if (enemyTemplate->Entities().size() > 0) // Gets a vector of entities using the enemy's template
		{
			EntityID firstEnemyEntity = enemyTemplate->Entities().front();
			gMessenger->DeliverMessage(GetID(), firstEnemyEntity, MessageType::Pause); // Pauses the first enemy, example code only - delete this!
		}
	}


	//---------------------------------------------------------
	// Car movement - code runs in all states. You may be able to retain this line for the assignment (but you don't have to)
	//---------------------------------------------------------me time
	Transform().MoveLocalZ(mSpeed * frameTime);


	return true; // Return true to keep the entity alive, false to destory it
}
