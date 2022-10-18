#include "DataLoader.hpp"

py::array_t<int> LoadTrainDataST(uint64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue) {
	// Time keeping variables.
	int64_t StartTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	int64_t EndTime;
	int64_t TimeTaken;
	// Samples & File Data Variables.
	int* LoadedSamples;
	std::string FileName = dataPath + tokenizerName;
	std::ifstream File;
	std::string FileDataSectionBuffer;
	uint64_t NumberOfSamplesInFile;
	uint64_t FileHeaderSectionLength; // Length of the section not including the length of the uint64_t val that indicates length.
	uint64_t FileDataSectionLength;
	uint64_t FileLength;
	uint64_t FileDataLengthToLoad;
	std::vector<BIN::SampleHeaderData> SamplesMetadata;
	// Sample Processing Variables.
	std::vector<int> SampleFromFileDataBuffer_int32;
	std::vector<uint24_t> SampleFromFileDataBuffer_int24;
	std::vector<uint16_t> SampleFromFileDataBuffer_int16;
	// Progress tracking variables.
	uint64_t MaxSamples = samplesToRead;
	uint64_t CurrentLine = 0;
	uint64_t ProgressReportInterval = MaxSamples / 100;

	// Open the file and query number of samples.
	File = std::ifstream(FileName, std::ios::binary | std::ios::ate);
	FileLength = File.tellg();
	File.seekg(0);
	File.read((char*)&FileHeaderSectionLength, sizeof(uint64_t));
	File.seekg(8);
	File.read((char*)&NumberOfSamplesInFile, sizeof(uint64_t));

	// Logic for handling a file that has less samples than user requested.
	if (samplesToRead > NumberOfSamplesInFile) {
		std::cout << "This File Does Not Contain Enough Samples, Loading " << NumberOfSamplesInFile << " Samples Instead." << std::endl;
		samplesToRead = NumberOfSamplesInFile;
		MaxSamples = samplesToRead;
		ProgressReportInterval = MaxSamples / 100;
	} else std::cout << "Loading " << samplesToRead << " Samples From " << FileName << std::endl;

	// Malloc the loaded samples array to contain the samples that are loaded.
	LoadedSamples = (int*)malloc(sizeof(int) * samplesToRead * sampleLength);

	std::cout << "Loading File Header." << std::endl;

	// Loading the file header contents to a vector.
	SamplesMetadata.resize(samplesToRead);
	File.seekg(16);
	File.read((char*)SamplesMetadata.data(), sizeof(BIN::SampleHeaderData) * samplesToRead);
	std::cout << "File Header Loaded." << std::endl;
	std::cout << "Number Of Samples In File " << NumberOfSamplesInFile << std::endl;

	// Query the file data section length.
	FileDataSectionLength = FileLength - FileHeaderSectionLength;

	// This is the IO & Memory efficient implimentation.

	// Set the length to the last sample + its length (almost totaled this up as a loop smh...)
	FileDataLengthToLoad = SamplesMetadata[SamplesMetadata.size() - 1].OffsetFromDataSectionStart + SamplesMetadata[SamplesMetadata.size() - 1].SampleLength;
		
	// Load the file data section into a buffer.
	FileDataSectionBuffer.resize(FileDataLengthToLoad);
	File.seekg(FileHeaderSectionLength + 8);
	File.read(FileDataSectionBuffer.data(), FileDataLengthToLoad);

	// Iterate over the metadata to attain each sample from the file and process it.
	for (BIN::SampleHeaderData& metadata : SamplesMetadata) {

		if (metadata.dtypeint16 == BIN_FILE_DTYPE_INT32) {
			// Resize the buffer to take in the file data.
			SampleFromFileDataBuffer_int32.resize(metadata.SampleLength / 4);

			// memcpy from the samples position in the buffer.
			memcpy(SampleFromFileDataBuffer_int32.data(), &FileDataSectionBuffer[metadata.OffsetFromDataSectionStart], metadata.SampleLength);
		}

		if (metadata.dtypeint16 == BIN_FILE_DTYPE_INT24) {
			// Resize the buffer to take in the file data.
			SampleFromFileDataBuffer_int24.resize(metadata.SampleLength / 3);

			// memcpy from the samples position in the buffer.
			memcpy(SampleFromFileDataBuffer_int24.data(), &FileDataSectionBuffer[metadata.OffsetFromDataSectionStart], metadata.SampleLength);

			// Resize and populate 32 bit int array.
			SampleFromFileDataBuffer_int32.resize(SampleFromFileDataBuffer_int24.size());
			for (size_t i = 0; i < SampleFromFileDataBuffer_int24.size(); i++) {
				SampleFromFileDataBuffer_int32[i] = static_cast<int>(SampleFromFileDataBuffer_int24[i]);
			}

		}

		if (metadata.dtypeint16 == BIN_FILE_DTYPE_INT16) {
			// std::cout << "Dtype uint24 Detected." << std::endl;
			// std::cout << CurrentLine << std::endl;
			// Resize the buffer to take in the file data.
			SampleFromFileDataBuffer_int16.resize(metadata.SampleLength / 2);

			// memcpy from the samples position in the buffer.
			memcpy(SampleFromFileDataBuffer_int16.data(), &FileDataSectionBuffer[metadata.OffsetFromDataSectionStart], metadata.SampleLength);

			// Resize and populate 32 bit int array.
			SampleFromFileDataBuffer_int32.resize(SampleFromFileDataBuffer_int16.size());
			for (size_t i = 0; i < SampleFromFileDataBuffer_int16.size(); i++) {
				SampleFromFileDataBuffer_int32[i] = static_cast<int>(SampleFromFileDataBuffer_int16[i]);
			}
		}

		// Apply padding & start + finish tokens & trim / pad array.
		SampleFromFileDataBuffer_int32.emplace(SampleFromFileDataBuffer_int32.begin(), startToken);
		if (SampleFromFileDataBuffer_int32.size() >= sampleLength) {
			SampleFromFileDataBuffer_int32.resize(sampleLength);
		}
		SampleFromFileDataBuffer_int32.push_back(endToken);

		for (size_t i = SampleFromFileDataBuffer_int32.size(); i < sampleLength; i++) {
			SampleFromFileDataBuffer_int32.push_back(paddingValue);
		}

		// append the sample to the return array.
		memcpy(&LoadedSamples[CurrentLine * sampleLength], SampleFromFileDataBuffer_int32.data(), sizeof(int) * SampleFromFileDataBuffer_int32.size());

		//Modify some loop variables.
		CurrentLine++;

		// Report progress (if neccesary)
		// if (CurrentLine % ProgressReportInterval == 0) {
		//	std::cout << (float)(CurrentLine * 100 / MaxSamples) << "% Done." << std::endl;
		// }
	}
	

	// Close the file and print out how long it took to load the data.
	File.close();
	std::cout << "Samples Have Been Read." << std::endl;
	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	TimeTaken = (EndTime - StartTime) / 1000000000;
	std::cout << "Time Taken: " << TimeTaken << " Seconds." << std::endl;

	// Encapsulate data into a numpy array for transfer back to python.
	py::capsule capsule = py::capsule(LoadedSamples, [](void* LoadedSamples) { delete reinterpret_cast<int*>(LoadedSamples); });
	py::array_t<int> SamplesReturnArray(
		{ (int64_t)samplesToRead, (int64_t)sampleLength },
		LoadedSamples,
		capsule);
	return SamplesReturnArray;
};