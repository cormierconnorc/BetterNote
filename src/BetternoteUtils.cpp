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
