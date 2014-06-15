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

	//Binary file write and read functions
	bool writeFile(const std::string& filePath, const std::string& fileContent);
	bool readFile(const std::string& filePath, std::string& fileContent);
	bool isValidFile(const std::string& filePath);

	//Glib checksum function wrappers
	std::string getHexChecksum(const std::string& data);
	std::string getBinaryChecksum(const std::string& data);

	//Use gio to get the appropriate file mime type
	std::string getMimeType(const std::string& fileName, const std::string& fileData);
}

#endif
