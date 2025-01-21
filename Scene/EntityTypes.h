//--------------------------------------------------------------------------------------
// Entity-related types
//--------------------------------------------------------------------------------------

#ifndef _ENTITY_TYPES_H_INCLUDED_
#define _ENTITY_TYPES_H_INCLUDED_

#include <stdint.h>


/*-----------------------------------------------------------------------------------------
   Definitions
-----------------------------------------------------------------------------------------*/

// Entity IDs are simply unsigned integers, could use uint64_t if you had a long running app
using EntityID = uint32_t;

// Allow for some special IDs
const EntityID NO_ID = 0; // Special ID to indicate a null ID (e.g. return value in the case of an error)
const EntityID SYSTEM_ID = 1; // Special ID of the system itself, can be used as the "from" parameter of messages
const EntityID FIRST_ENTITY_ID = 2;


#endif //_ENTITY_TYPES_H_INCLUDED_
