#include "DataLoader.hpp"

BINFile::BINFile(std::string dataPath, int startToken, int endToken, int sampleLength, int paddingVal) : startToken(startToken), endToken(endToken), sampleLength(sampleLength), paddingVal(paddingVal) {
	std::cout << "Initialising BIN file." << std::endl;
};
