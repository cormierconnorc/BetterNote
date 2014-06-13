/*
 *Connor Cormier
 *6/10/14
 *Utility functions to assist betternote
 */


#ifndef BETTERNOTEUTILS_H
#define BETTERNOTEUTILS_H

#include "evernote_api/src/NoteStore_types.h"
#include <string>

namespace Util
{
	//Generate 128 bit random number and convert it to a GUID
	evernote::edam::Guid genGuid();

	//Basic util function to help with others
	int getIntEquiv(char hex);
	char getHexEquiv(int integer);

	//Convert strings between hexadecimal and binary representations
	std::string hexToBinaryString(std::string hex);
	std::string binaryToHexString(std::string bin);
}

#endif
