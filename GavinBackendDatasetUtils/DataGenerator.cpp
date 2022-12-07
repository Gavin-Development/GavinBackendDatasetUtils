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

	// configure and setup the capsules and array_t objects for python to access.

	ToSampleBufferCapsule = py::capsule(ToSampleBuffer, [](void* ToSampleBuffer) { delete reinterpret_cast<int*>(ToSampleBuffer); });
	FromSampleBufferCapsule = py::capsule(FromSampleBuffer, [](void* FromSampleBuffer) { delete reinterpret_cast<int*>(FromSampleBuffer); });

	ToSampleBufferArray_t = py::array_t<int>({ (uint64_t)BufferSize, (uint64_t)sampleLength }, ToSampleBuffer, ToSampleBufferCapsule);
	FromSampleBufferArray_t = py::array_t<int>({ (uint64_t)BufferSize, (uint64_t)sampleLength }, FromSampleBuffer, FromSampleBufferCapsule);

	// Print out the settings for this generator to console to allow the programmer to check that they have configured the generator correctly.
	std::cout << "Data generator initialised:\n ToFile: " << (dataPath + tokenizertoName) << "\n FromFile: " << (dataPath + tokenizerfromName) << "\n NumberOfSamplesInFile: " << NumberOfSamplesInFile << "\n Sample Length: " << sampleLength << "\n StartToken: " << startToken << "\n EndToken: " << endToken << "\n PaddingValue: " << paddingValue << "\n BufferSize: " << BufferSize << std::endl;
};

void DataGenerator::UpdateDataBuffer() {
	std::cout << "Updating Data Buffers." << std::endl;

	BIN::SampleHeaderData ToFileHeaderData;
	BIN::SampleHeaderData FromFileHeaderData;

	for (uint32_t i = 0; i < BufferSize; i++) {
		// check that we are not about to go out of bounds for file data.
		if (CurrentSampleNumber > NumberOfSamplesInFile) CurrentSampleNumber = 0;

		// seek to the position of the header data in the file.
		ToFile.seekg(CurrentSampleNumber * sizeof(BIN::SampleHeaderData) + 16);
		FromFile.seekg(CurrentSampleNumber * sizeof(BIN::SampleHeaderData) + 16);

		// Read in the header Data.
		ToFile.read((char*)&ToFileHeaderData, sizeof(BIN::SampleHeaderData));
		FromFile.read((char*)&FromFileHeaderData, sizeof(BIN::SampleHeaderData));


		// Load the samples into their respective buffers.
		ReadSampleFromFile(&ToFile, ToFileHeaderData, &ToSampleBuffer[i * sampleLength]);
		ReadSampleFromFile(&FromFile, FromFileHeaderData, &FromSampleBuffer[i * sampleLength]);

		// Incriment the "CurrentSample" variable to make the loop read the next sample in line next pass.
		CurrentSampleNumber++;

	}

	std::cout << "Data Buffers Updated." << std::endl;
};

void DataGenerator::ReadSampleFromFile(std::ifstream* File, BIN::SampleHeaderData HeaderData, int* BufferToLoadTo) {
	int j;
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
			File->read((char*)&Buffer_int32[1], (sampleLength - 2) * sizeof(int));
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
			File->read((char*)Buffer_int24, HeaderData.SampleLength);

			// Transfer the 24 bit values to 32bit values.
			for (int i = 0; i < (HeaderData.SampleLength / 3); i++) {
				Buffer_int32[i + 1] = (int)Buffer_int24[i];
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
			File->read((char*)Buffer_int24, sizeof(uint24_t) * (sampleLength - 2));

			for (int i = 0; i < sampleLength - 2; i++) {
				Buffer_int32[i + 1] = (int)Buffer_int24[i];
			}

			// Set the end token.
			Buffer_int32[sampleLength - 1] = endToken;

			// memcpy to the destination array.
			memcpy(BufferToLoadTo, Buffer_int32, sizeof(int) * sampleLength);
			return;
		}
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
			File->read((char*)Buffer_int16, HeaderData.SampleLength);

			//transfer the 16 bit values to 32 bit values
			for (size_t i = 0; i < (HeaderData.SampleLength / 2); i++) {
				Buffer_int32[i+1] = Buffer_int16[i];
			}

			// Add end token to the 32 bit array.
			Buffer_int32[(HeaderData.SampleLength / 2) + 1] = endToken;

			//memcpy to the destination array.
			memcpy(BufferToLoadTo, Buffer_int32, sizeof(int) * sampleLength);
			return;
		}

		// If the sampple is longer than the buffer.
		else {
			File->read((char*)Buffer_int16, sizeof(uint16_t) * (sampleLength-2));

			// transfer to the 32 bit array.
			for (int i = 0; i < sampleLength - 2; i++) {
				Buffer_int32[i + 1] = (int)Buffer_int16[i];
			}

			// Set the end token
			Buffer_int32[sampleLength - 1] = endToken;

			//memcpy to the destination array.
			memcpy(BufferToLoadTo, Buffer_int32, sizeof(int) * sampleLength);
			return;
		}
	}
};


DataGenerator::~DataGenerator() {
	free(ToSampleBuffer);
	free(FromSampleBuffer);

	free(Buffer_int32);
	free(Buffer_int24);
	free(Buffer_int16);
};