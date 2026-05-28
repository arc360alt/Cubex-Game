#include "SharedConstants.h"

namespace Common {

std::string getGameVersionString(const std::string& versionSuffix /* = "" */)
{
	return std::string("v0.1.0") + versionSuffix + " beta";
}

};
