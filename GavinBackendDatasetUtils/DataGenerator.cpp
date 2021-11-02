#include "DataLoader.hpp"

DataGenerator::DataGenerator(std::string dataPath, std::string tokenizertoName, std::string tokenizerfromName, uint64_t iBufferSize, int istartToken, int iendToken, int isampleLength, int ipaddingValue) {
	uint64_t NumberOfSamplesInToFile, NumberOfSamplesInFromFile;
	uint64_t ToFileHeaderLength, FromFileHeaderLength;
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

	if (ToFileHeaderLength != FromFileHeaderLength) {
		throw std::runtime_error("The files headers are not the same length, this indicates an incompatibility of the dataset.");
	}
	else FileHeaderLength = ToFileHeaderLength;

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

	// Allocate the load buffers for sample loading

	Buffer_int32 = (int*)malloc(sizeof(int) * sampleLength);
	Buffer_int24 = (uint24_t*)malloc(sizeof(uint24_t) * (sampleLength - 2));
	Buffer_int16 = (uint16_t*)malloc(sizeof(uint16_t) * (sampleLength - 2));

	Buffer_int32[0] = startToken;
};

void DataGenerator::UpdateDataBuffer() {
	std::cout << "Updating Data Buffer." << std::endl;

	BIN::SampleHeaderData headerData;
	BIN::TemporaryLoadBuffers LoadBuffers(sampleLength);
	LoadBuffers.Buffer_int32[0] = startToken;

	for (uint32_t i = 0; i < BufferSize; i++) {

	}

};

void DataGenerator::ReadSampleFromFile(std::ifstream* File, BIN::SampleHeaderData HeaderData, int* BufferToLoadTo) {
	// seek the file to the location of the sample in the file
	File->seekg(8 + FileHeaderLength + HeaderData.OffsetFromDataSectionStart);

	// padd out the 32 bit array.
	for (int i = 1; i < sampleLength; i++) {
		Buffer_int32[i] = paddingValue;
	}

	// Read and manipulate the sample based on its Dtype.
	if (HeaderData.dtypeint16 == BIN_FILE_DTYPE_INT32) {

		// read in the values from the file into the buffer.
		if (HeaderData.SampleLength / 4 < sampleLength - 2) {
			File->read((char*)&Buffer_int32[1], HeaderData.SampleLength);
			Buffer_int32[(HeaderData.SampleLength / 4) + 1] = endToken;
		}
		else {
			File->read((char*)&Buffer_int32[1], (sampleLength - 2) * 4);
			Buffer_int32[sampleLength - 1] = endToken;
		}

		// memcpy to the destination array.
		memcpy(BufferToLoadTo, Buffer_int32, sizeof(int) * sampleLength);
		return;
	}

	if (HeaderData.dtypeint16 == BIN_FILE_DTYPE_INT24) {
		// Read in the values from the file and covert to int32.

		// If the sample is shorter than the buffer.
		if (HeaderData.SampleLength / 3 < sampleLength - 2) {

			// padd out the 24 bit array
			for (int i = 0; i < sampleLength - 2; i++) {
				Buffer_int24[i] = paddingValue;
			}

			// Read the values into 24 bit array
			File->read((char*)&Buffer_int24, HeaderData.SampleLength);

			// Transfer the 24 bit values to 32bit values.
			for (int i = 0; i < (HeaderData.SampleLength / 3); i++) {
				Buffer_int32[i + 1] = Buffer_int24[i];
			}

			// Add end token to the 32bit array.
			Buffer_int32[(HeaderData.SampleLength / 3) + 1] = endToken;

			// memcpy to the destination array.
			memcpy(BufferToLoadTo, Buffer_int32, sizeof(int) * sampleLength);
			return;
		}

		// If the sample is longer than the buffer.
		else {
			// Read the values into the 24 bit array.
			File->read((char*)&Buffer_int24, sizeof(int) * (sampleLength - 2));

			for (int i = 0; i < sampleLength - 2; i++) {
				Buffer_int32[i + 1] = Buffer_int24[i];
			}

			// Set the end token.
			Buffer_int32[sampleLength - 1] = endToken;

			// memcpy to the destination array.
			memcpy(BufferToLoadTo, Buffer_int32, sizeof(int) * sampleLength);
			return;
		}

		if (HeaderData.dtypeint16 == BIN_FILE_DTYPE_INT16) {
			// Read in the values from the file and convert to int32.

			// If the sample is shorter than the buffer.
			if (HeaderData.SampleLength / 2 < sampleLength - 2) {

				// padd out the 16 bit array.
				for (int i = 0; i < sampleLength - 2; i++) {
					Buffer_int16[i] = paddingValue;
				}

				//read in the values to the 16 bit array.
				File->read((char*)&Buffer_int16, HeaderData.SampleLength);
			}
		}
	}
};