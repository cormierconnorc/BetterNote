/*
 *Connor Cormier
 *6/10/14
 *Utility functions to assist betternote
 */


#ifndef BETTERNOTEUTILS_H
#define BETTERNOTEUTILS_H

#include "evernote_api/src/NoteStore_types.h"

namespace Util
{
	//Generate 128 bit random number and convert it to a GUID
	evernote::edam::Guid genGuid();
}

#endif
