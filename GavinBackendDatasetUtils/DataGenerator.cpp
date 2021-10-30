#include "DataLoader.hpp"

DataGenerator::DataGenerator(std::string dataPath, std::string tokenizertoName, std::string tokenizerfromName, uint64_t iBufferSize, int istartToken, int iendToken, int isampleLength, int ipaddingValue) {
	uint64_t NumberOfSamplesInToFile, NumberOfSamplesInFromFile;
	std::cout << "Initialising Data Generator." << std::endl;

	// Open the files then establish their length to ensure no reading goes outside of file bounds. Also gathering some data about the dataset.

	ToFile = std::ifstream(dataPath + tokenizertoName, std::ios::binary | std::ios::ate);
	FromFile = std::ifstream(dataPath + tokenizerfromName, std::ios::binary | std::ios::ate);
	ToFileLength = ToFile.tellg();
	FromFileLength = FromFile.tellg();

	ToFile.seekg(0);
	ToFile.read((char*)&ToFileHeaderLength, sizeof(uint64_t));

	FromFile.seekg(0);
	FromFile.read((char*)&FromFileHeaderLength, sizeof(uint64_t));

	ToFile.seekg(8);
	ToFile.read((char*)&NumberOfSamplesInToFile, sizeof(uint64_t));

	FromFile.seekg(8);
	FromFile.read((char*)&NumberOfSamplesInFromFile, sizeof(uint64_t));

	if (NumberOfSamplesInFromFile != NumberOfSamplesInToFile) {
		throw std::runtime_error("The files do not contain the same number of samples, symetrical loading will NOT WORK");
	}
	else NumberOfSamplesInFile = NumberOfSamplesInFromFile;

	// set some class variables.

	startToken = istartToken;
	endToken = iendToken;
	sampleLength = isampleLength;
	paddingValue = ipaddingValue;
	BufferSize = iBufferSize;
	CurrentSampleNumber = 0;

	// allocate the data buffer.

	ToSampleBuffer = (int*)malloc(sizeof(int) * sampleLength * iBufferSize);
	FromSampleBuffer = (int*)malloc(sizeof(int) * sampleLength * iBufferSize);
};

void DataGenerator::UpdateDataBuffer() {
	std::cout << "Updating Data Buffer." << std::endl;

	for (uint32_t i = 0; i < BufferSize; i++) {

	}

};

void DataGenerator::ReadSampleFromFile(std::ifstream* File) {
	int* SampleFromFileDataBuffer_int32 = (int*)malloc(sizeof(int) * sampleLength);
	uint24_t* SampleFromFileDataBuffer_int24 = (uint24_t*)malloc(sizeof(uint24_t) * (sampleLength - 2));
	uint16_t* SampleFromFileDataBuffer_int16 = (uint16_t*)malloc(sizeof(uint16_t) * (sampleLength - 2));
};