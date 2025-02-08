/*-----------------------------------------------------------------------------------------
	Messenger class
-----------------------------------------------------------------------------------------*/
// Allows messages with arbitrary payloads to be sent to other entities (addressed by EntityID)

#ifndef _MESSENGER_H_INCLUDED_
#define _MESSENGER_H_INCLUDED_

#include "Vector3.h"
#include "Entity.h"

#include <map>
#include <variant>


/*-----------------------------------------------------------------------------------------
	Message Types and Data
-----------------------------------------------------------------------------------------*/

// Enum of supported message types
//*** These are just examples, add / remove messages as required
enum class MessageType
{
	Pause,        // Game is pausing                 - no data payload
	Unpause,      // Game is unpausing               - no data payload
	TargetEntity, // Set target to a given enity UID - carries TargetUIDData payload
	TargetPoint,  // Set target to a given point     - carries TargetPointData payload
	TargetNone,   // Set no target                   - no data payload
	Die,          // Destroy entity or component     - no data payload
	Start,
	Stop,
	Hit,
	Evade,
	Help,
	Reload,
	MineHit,
	CrateCollected
};


// Some messages carry a "payload" of supporting data - define a struct for each. Other messages require no payload
//*** These are examples, add structures if you need them for new message types, also add them to MessageData below

// Data payload for Msg_TargetEntity
struct TargetEntityData
{
	EntityID target;
	float    range;
};

// Data payload for Msg_TargetPoint
struct TargetPointData
{
	Vector3 target;
	float   range;
};

enum class CrateType
{
	Missile,
	Health,
	Shield
};

struct CratePickupData
{
	CrateType type;
};

struct MissileHitData
{
	EntityID launchingBoatID;
};

// Then modify the MessageData type:
// Different types of messages carry different payloads of data. We could do this with polymorphism, but messaging must be
// efficient and polymorphism carries some overheads (this overhead is often not a problem but could be in a messaging heavy game). 
// Instead we use std::variant (C++17), which is a type that can be one from a collection of alternatives.
// The type MessageData below can be one of TargetEntityData, TargetPointData or empty (indicated by std::monostate)
//*** These are examples, if you add new message data structures, add them to this collection
using MessageData = std::variant<std::monostate, TargetEntityData, TargetPointData, CratePickupData, MissileHitData>;

// Message structure as returned from ReceiveMessage function: contains the sender, message type and optionally some kind of data payload
//*** Do not change this structure
struct Message
{
	EntityID    from;
	MessageType type;
	MessageData data; // Different message types carry different data. MessageData is one of the payload types above or empty - see the comment on MessageData above
};


/*-----------------------------------------------------------------------------------------
	Messenger Class
----------------------------------------------------------------------------------------*/

// Messenger class allows the sending and receiving of messages between entities - addressed by UID
class Messenger
{
	/*-----------------------------------------------------------------------------------------
	    Message sending/receiving
	-----------------------------------------------------------------------------------------*/
public:
	// Send a message and message data to a particular UID
	//   Example: DeliverMessage(myUID, allyUID, MessageType::TargetEntity, TargetEntityData{enemyUID, targetRange});
	//
	// Each message type has an expected data payload (see the enum in Messenger.h). The final parameter must use the appropriate
	// message data type. Don't pass the final parameter (let it use the default) for messages that need no data payload.
	// No validation is performed to ensure the target UID exists or that the message is well-formed
	void DeliverMessage(EntityID from, EntityID to, MessageType type, MessageData data = {});

	// Fetch the next available message for the given UID, returns the message through the given 
	// pointer. Returns false if there are no messages for this UID
	bool ReceiveMessage(EntityID to, Message* msg);


	/*-----------------------------------------------------------------------------------------
		Private data
	-----------------------------------------------------------------------------------------*/
private:
	// A multimap is similar to a map - it maps keys to values. In a map each key can only occur once, but in a multimap there can
	// be multiple entries with the same key. This is perfect for holding messages - the key is the recipient, the value the message.
	// We can have more than one message per recipient (as this is a multimap) and the recipient can look up their messages efficiently
	// since this is a map
	std::multimap<EntityID, Message> mMessages;
};


#endif //_MESSENGER_H_INCLUDED_