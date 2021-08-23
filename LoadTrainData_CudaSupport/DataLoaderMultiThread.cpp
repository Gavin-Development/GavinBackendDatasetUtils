#include "DataLoader.hpp"

void LoadDataThread(uint64_t SamplesToRead, uint64_t FileHeaderSectionLength, size_t ThreadId, std::string FileName, int* SamplesArray, BIN::SampleHeaderData* SamplesMetadata, int startToken, int endToken, int sampleLength, int paddingValue) {
	std::ifstream File(FileName, std::ios::binary);
	uint64_t SampleWritePos;
	uint64_t NumberOfSamplesRead = 0;
	std::vector<int> SampleFromFileDataBuffer_int32;
	std::vector<uint16_t> SampleFromFileDataBuffer_int16;

		for (size_t i = ThreadId * SamplesToRead; i < (ThreadId * SamplesToRead) + SamplesToRead; i++) {
			SampleWritePos = (ThreadId * SamplesToRead * sampleLength) + (sampleLength * NumberOfSamplesRead); // Thread offset in the mallocd array then the sample offset.

			if (SamplesMetadata[i].dtypeint16 == 0) {
				// Resize the 32bit int vector.
				SampleFromFileDataBuffer_int32.resize(SamplesMetadata[i].SampleLength / 4);

				// Seek to position and read it.
				File.seekg(FileHeaderSectionLength + SamplesMetadata[i].OffsetFromDataSectionStart + 8);
				File.read((char*)SampleFromFileDataBuffer_int32.data(), SamplesMetadata[i].SampleLength);
			}

			if (SamplesMetadata[i].dtypeint16 == 1) {
				// Resize the int16 vector to match the sample size.
				SampleFromFileDataBuffer_int16.resize(SamplesMetadata[i].SampleLength / 2);

				// Read the sample into the vector.
				File.seekg(FileHeaderSectionLength + SamplesMetadata[i].OffsetFromDataSectionStart + 8);
				File.read((char*)SampleFromFileDataBuffer_int16.data(), SamplesMetadata[i].SampleLength);

				// Resize 32 bit vector and transfer and cast 16 bit contents into it.
				SampleFromFileDataBuffer_int32.resize(SampleFromFileDataBuffer_int16.size());
				for (size_t j = 0; j < SampleFromFileDataBuffer_int16.size(); j++) {
					SampleFromFileDataBuffer_int32[j] = static_cast<int>(SampleFromFileDataBuffer_int16[j]);
				}

			}

			// Apply padding & start + end token & trim / pad array.
			SampleFromFileDataBuffer_int32.emplace(SampleFromFileDataBuffer_int32.begin(), startToken);
			if (SampleFromFileDataBuffer_int32.size() >= sampleLength) {
				SampleFromFileDataBuffer_int32.resize(sampleLength);
			}
			SampleFromFileDataBuffer_int32.push_back(endToken);

			for (size_t j = SampleFromFileDataBuffer_int32.size(); j < sampleLength; j++) {
				SampleFromFileDataBuffer_int32.push_back(paddingValue);
			}

			//Copy the contents into the thread return memory buffer.
			memcpy(&SamplesArray[SampleWritePos], SampleFromFileDataBuffer_int32.data(), sizeof(SampleFromFileDataBuffer_int32[0]) * SampleFromFileDataBuffer_int32.size());
			NumberOfSamplesRead++;
	}
};

py::array_t<int> LoadTrainDataMT(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue) {
	// Time keeping variables.
	int64_t StartTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	int64_t EndTime;
	int64_t TimeTaken;
	// Samples & File Data Variables.
	std::string FileName = dataPath + tokenizerName;
	std::ifstream File;
	uint64_t NumberOfSamplesInFile;
	uint64_t FileHeaderSectionLength; // Length of the section not including the length of the uint64_t val that indicates length.
	uint64_t FileLength;
	BIN::SampleHeaderData* SamplesMetadata;
	// Progress tracking variables.
	uint64_t MaxSamples = samplesToRead;
	uint64_t ProgressReportInterval = MaxSamples / 100;
	// Multithreading setup variables.
	std::vector<std::thread> DataLoaderThreads;
	int* MultiThreadDataBuffer;
	uint32_t NumberOfThreadsAvalible;
	uint32_t NumberOfThreadsToUse;
	uint64_t SamplesToReadPerThread;
	uint64_t SamplesLaunched;

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
	}
	else std::cout << "Loading " << samplesToRead << " Samples From " << FileName << std::endl;

	std::cout << "Loading File Header." << std::endl;

	// Loading the file header contents to a vector.
	SamplesMetadata = (BIN::SampleHeaderData*)malloc(sizeof(BIN::SampleHeaderData) *samplesToRead);
	File.seekg(16);
	File.read((char*)SamplesMetadata, sizeof(BIN::SampleHeaderData) * samplesToRead);
	std::cout << "File Header Loaded." << std::endl;
	std::cout << "Number Of Samples In File " << NumberOfSamplesInFile << std::endl;

	// Determine optimal number of threads to use.
	NumberOfThreadsAvalible = std::thread::hardware_concurrency() * 0.8;
	NumberOfThreadsToUse = NumberOfThreadsAvalible;

	// Will try to distrobute across all threads evenly.
	SamplesToReadPerThread = samplesToRead / NumberOfThreadsAvalible;
	std::cout << "All Threads Will Read " << SamplesToReadPerThread << " Samples." << std::endl;

	// Prepare The MultiThread data load buffer.
	// This is done as i am not sure how thread safe STL containers are so fuck it good ol malloc ;)
	MultiThreadDataBuffer = (int*)malloc(sizeof(int) * SamplesToReadPerThread * NumberOfThreadsToUse * sampleLength);

	// Close the file pointer to prevent the OS from getting confused.
	File.close();

	// Launch Threads.
	for (size_t i = 0; i < NumberOfThreadsToUse; i++) {
		std::cout << "Thread Launched. " << i << std::endl;
		DataLoaderThreads.push_back(std::thread(LoadDataThread, SamplesToReadPerThread, FileHeaderSectionLength, i, FileName, MultiThreadDataBuffer, SamplesMetadata, startToken, endToken, sampleLength, paddingValue));
	}
	std::cout << "Thread Launches Done." << std::endl;
	for (std::thread& th : DataLoaderThreads) {
		th.join();
	}
	//{ SamplesToReadPerThread * NumberOfThreadsToUse, sampleLength, 1},{1000 * 8 * 1000, 1000 * 8, 8},
	std::cout << "Data Loaded." << std::endl; //SamplesToReadPerThread * NumberOfThreadsToUse * sampleLength
	py::capsule capsule = py::capsule(MultiThreadDataBuffer, [](void* MultiThreadedDataBuffer) { delete reinterpret_cast<std::vector<int>*>(MultiThreadedDataBuffer); });
	py::array_t<int> LoadedSamples(
		{ (int64_t)SamplesToReadPerThread * (int64_t)NumberOfThreadsToUse, (int64_t)sampleLength },
		MultiThreadDataBuffer,
		capsule);
	return LoadedSamples;

}