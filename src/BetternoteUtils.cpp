/*
 *Connor Cormier
 *6/10/14
 *Utility functions to assist betternote
 */

#include "BetternoteUtils.h"
#include <ctime>
#include <cstdlib>
#include <boost/multiprecision/cpp_int.hpp>
#include <sstream>


using namespace evernote::edam;
using namespace boost::multiprecision;
using namespace std;

Guid Util::genGuid()
{
	//Amount to shift in each time
	size_t eachShift = sizeof(int) * 8;

	//Big integer type to read into
	uint128_t bigRandomNumber;

	//Place random numbers in bigRand:
	for (size_t i = 0; i < sizeof(uint128_t) / sizeof(int); i++)
		bigRandomNumber = (bigRandomNumber << eachShift) | rand();

	ostringstream chars;
	
	//Now convert the random number to a hexadecimal GUID:
	//Little endian representation
	for (size_t i = 0; i < 128 / 4; i++)
	{
		//Wor
		int digit = (bigRandomNumber % 16).convert_to<int>();

		//Hex representation
		if (digit < 10)
			chars << digit;
		else
			chars << (char)(digit - 10 + 'a');

		//Hyphens to make valid GUID
		if (i == 7 || i == 11 || i == 15 || i == 19)
			chars << '-';

		bigRandomNumber /= 16;
	}

	return chars.str();
}

int Util::getIntEquiv(char hex)
{
	if (hex >= '0' && hex <= '9')
		return hex - '0';
	else if (hex >= 'A' && hex <= 'F')
		return hex - 'A' + 10;
	else
		return hex - 'a' + 10;
}

char Util::getHexEquiv(int integer)
{
	if (integer < 10)
		return (char)(integer + '0');
	else
		return (char)(integer - 10 + 'a');
}

string Util::hexToBinaryString(string hex)
{
	//If length is not divisible by 2, add a 0 byte to the start
	if (hex.length() % 2 != 0)
		hex = "0" + hex;
	
	//Create a buffer to hold the binary string
	char buffer[hex.length() / 2];

	//Look at 2 characters in the hex string each time, matching them with 1 buffer char
	for (size_t i = 0; i < hex.length(); i += 2)
	{
		int high = getIntEquiv(hex[i]), low = getIntEquiv(hex[i + 1]);

		buffer[i / 2] = ((high & 0xF) << 4) | (low & 0xF);
	}

	//Now convert it to a string
	return string(buffer, hex.length() / 2);
	
}

string Util::binaryToHexString(string bin)
{
	//Hex buffer
	char buffer[bin.length() * 2];

	for (size_t i = 0; i < bin.length(); i++)
	{
		char fullByte = bin[i];
		
		//High side of buffer:
		buffer[i * 2] = getHexEquiv((fullByte >> 4) & 0xF);

		//Low side of buffer:
		buffer[i * 2 + 1] = getHexEquiv(fullByte & 0xF);
	}

	return string(buffer, bin.length() * 2);
}
