/*-----------------------------------------------------------------------------------------
	Messenger class
-----------------------------------------------------------------------------------------*/
// Allows messages with arbitrary payloads to be sent to other entities (addressed by EntityID)

#include "Messenger.h"


/*-----------------------------------------------------------------------------------------
	Message sending/receiving
-----------------------------------------------------------------------------------------*/

// Send a message and message data from a given UID to a particular UID
//   Example: DeliverMessage(myUID, allyUID, MessageType::TargetEntity, TargetEntityData{enemyUID, targetRange});
//
// Each message type has an expected data payload (see the enum in Messenger.h). The final parameter must use the appropriate
// message data type. Don't pass the final parameter (let it use the default) for messages that need no data payload.
// No validation is performed to ensure the target UID exists or that the message is well-formed
void Messenger::DeliverMessage(EntityID from, EntityID to, MessageType type, MessageData data /*= {}*/)
{
	// Create a Message object and insert the recipient UID + message into the multi-map of all current messages
	// It will be inserted next to any other messages with the same recipient UID
	Message msg = { from, type, data };
	mMessages.insert({ to, msg });

	// This is a more efficient way to replace the above two lines - it reduces copying and the need to construct a temporary Message
	// It is particularly efficient when the data parameter is bracketed like this:  DeliverMessage(myUID, otherUID, MessageType::TargetPoint, TargetPointData{myTargetPoint, 5});
	//mMessages.emplace(std::piecewise_construct, std::forward_as_tuple(to), std::forward_as_tuple(from, type, data));
}


// Fetch the next available message for the given UID, returns the message through the given 
// pointer. Returns false if there are no messages for this UID
// Call multiple times until it returns false to fetch all messages for an entity
bool Messenger::ReceiveMessage(EntityID to, Message* msg)
{
	// Find the first message for this UID in the message map
	auto iter = mMessages.find(to);

	// Quit if no messages for this UID
	if (iter == mMessages.end())
	{
		return false;
	}

	// Return message, then delete it
	*msg = iter->second;
	mMessages.erase(iter);

	return true;
}
