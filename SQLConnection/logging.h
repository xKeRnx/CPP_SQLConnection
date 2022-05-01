#pragma once
#include <Windows.h>
#include <fstream>

namespace SStub {

	inline std::string ExePath() {

		CHAR buffer[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, buffer, MAX_PATH);

		std::string::size_type pos = std::string(buffer).find_last_of("\\/");

		return std::string(buffer).substr(0, pos);
	}

	static void logError(std::string error) {

		std::string path = ExePath() + "//..//error.txt";

		std::ofstream ofs(path);

		ofs << "Error: " << error;
		ofs.close();

	}

}