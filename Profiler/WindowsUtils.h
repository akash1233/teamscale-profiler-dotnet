#pragma once

#include <string>

class WindowsUtils {
public:

	/**
	* Returns the message for the last WinAPI error (retrieved via GetLastError).
	* Adapted from https://stackoverflow.com/a/17387176/1396068
	*/
	static std::string getLastErrorAsString();

	/** Return the value for the environment variable COR_PROFILER_<suffix> or the empty string if it is not set. */
	static std::string getConfigValueFromEnvironment(std::string suffix);

	/** Returns information about the profiled process. */
	static std::string getProcessInfo();
};