#include "DataLoader.hpp"

BINFile::BINFile(std::string dataPath, int startToken, int endToken, int sampleLength, int paddingVal) : startToken(startToken), endToken(endToken), sampleLength(sampleLength), paddingVal(paddingVal) {
	std::cout << "Initialising BIN file handler." << std::endl;
	
	// check for existing BIN file.
	_File = std::fstream(dataPath, std::ios::in | std::ios::ate | std::ios::out | std::ios::binary);

	// try to open existing file.
	if (!_File.is_open()) {
		std::cout << "No BIN File Present, Creating A New One." << std::endl;

		_File.close();
		_File = std::fstream(dataPath, std::ios::out | std::ios::binary);

		std::cout << "Created New BIN File." << std::endl;
		_File.write((char*)(uint64_t)8, sizeof(uint64_t));
	}
	// If there is an existing file.
	else {
		std::cout << "Opened Existing BIN File." << std::endl;
		_File.seekg(std::ios::ate);
		_FileLength = _File.tellg();
	}
	// Calling File close to test.
	_File.close();
};

BINFile::~BINFile() {
	std::cout << "Called BINFile Destructor." << std::endl;
	if (_File.is_open()) _File.close();
}