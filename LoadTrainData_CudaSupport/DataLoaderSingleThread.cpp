#include "DataLoader.hpp"

std::vector<py::list> LoadTrainDataST(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue) {
	// Time keeping variables.
	int64_t StartTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	int64_t EndTime;
	int64_t TimeTaken;
	// Samples & File Data Variables.
	std::vector<py::list> LoadedSamples;
	std::string FileName = dataPath + tokenizerName;
	std::ifstream File;
	std::string FileDataSectionBuffer;
	uint64_t NumberOfSamplesInFile;
	uint64_t FileHeaderSectionLength; // Length of the section not including the length of the uint64_t val that indicates length.
	uint64_t FileDataSectionLength;
	uint64_t FileLength;
	std::vector<BIN::SampleHeaderData> SamplesMetadata;
	// Sample Processing Variables.
	std::vector<int> SampleFromFileDataBuffer_int32;
	std::vector<uint16_t> SampleFromFileDataBuffer_int16;
	// Progress tracking variables.
	int64_t MaxSamples = samplesToRead;
	int64_t CurrentLine = 0;
	int64_t ProgressReportInterval = MaxSamples / 100;

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

	std::cout << "Loading File Header." << std::endl;
	// Loading the file header contents to a vector.
	SamplesMetadata.resize(NumberOfSamplesInFile);
	File.seekg(16);
	File.read((char*)SamplesMetadata.data(), sizeof(BIN::SampleHeaderData) * NumberOfSamplesInFile);
	std::cout << "File Header Loaded." << std::endl;
	std::cout << "Number Of Samples In File " << NumberOfSamplesInFile << std::endl;

	// Query the file data section length.
	FileDataSectionLength = FileLength - FileHeaderSectionLength;

	// Load the file data section into a buffer.
	FileDataSectionBuffer.resize(FileDataSectionLength);
	File.seekg(FileHeaderSectionLength);
	File.read(FileDataSectionBuffer.data(), FileDataSectionLength);

	// Iterate over the metadata to attain each sample from the file and process it.
	for (BIN::SampleHeaderData& metadata : SamplesMetadata) {
		if (samplesToRead > 0) {
			// Initialise a new py::list for the data
			py::list SampleData;
			if (metadata.dtypeint16 == 0) {
				// Resize the buffer to take in the file data.
				SampleFromFileDataBuffer_int32.resize(metadata.SampleLength / 4);

				// Seek to position of the sample in file and read it.
				File.seekg(FileHeaderSectionLength + metadata.OffsetFromDataSectionStart + 8);
				File.read((char*)SampleFromFileDataBuffer_int32.data(), sampleLength);
			}

			if (metadata.dtypeint16 == 1) {
				// Resize the buffer to take in the file data.
				SampleFromFileDataBuffer_int16.resize(metadata.SampleLength / 2);

				// Seek to position of the sample in file and read it.
				File.seekg(FileHeaderSectionLength + metadata.OffsetFromDataSectionStart + 8);
				File.read((char*)SampleFromFileDataBuffer_int16.data(), sampleLength);

				// Resize and populate 32 bit int array.
				SampleFromFileDataBuffer_int32.resize(SampleFromFileDataBuffer_int16.size());
				for (size_t i = 0; i < SampleFromFileDataBuffer_int16.size(); i++) {
					SampleFromFileDataBuffer_int32[i] = static_cast<int>(SampleFromFileDataBuffer_int16[i]);
				}
			}

			// Apply padding & start + finish tokens.
			SampleFromFileDataBuffer_int32.emplace(SampleFromFileDataBuffer_int32.begin(), startToken);
			SampleFromFileDataBuffer_int32.push_back(endToken);

			for (size_t i = SampleFromFileDataBuffer_int32.size(); i < sampleLength; i++) {
				SampleFromFileDataBuffer_int32.push_back(paddingValue);
			}
			
			//Convert sample to py::list
			for (size_t i = 0; i < SampleFromFileDataBuffer_int32.size(); i++) {
				SampleData.append(SampleFromFileDataBuffer_int32[i]);
			}

			// append the sample to the return array.
			LoadedSamples.push_back(SampleData);

			//Modify some loop variables.
			samplesToRead--;
		}
		else break;
		
		
	}

	File.close();
	std::cout << "Samples Have Been Read." << std::endl;
	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	TimeTaken = (EndTime - StartTime) / 1000000000;
	std::cout << "Time Taken: " << TimeTaken << " Seconds." << std::endl;
	return LoadedSamples;
};

void SaveTrainDataST(std::vector<std::vector<int>> Data, std::string FileName) {
	std::cout << "WARNING THI FUNCTION DOES NOT WORK AS INTENDED DO NOT USE PROPERLY." << std::endl;
	std::ofstream File(FileName, std::ios::binary | std::ios::out | std::ios::app);
	std::cout << "File Created." << std::endl;

	int64_t DataLength = Data.size();
	int64_t CurrentIndex = 0;
	int64_t ProgressReportInterval = DataLength / 100;
	std::string tmpchr;
	std::string tmpchr2;
	char bytes[4];
	for (std::vector<int> intvec : Data) {
		tmpchr.erase();
		for (int integer : intvec) {
			bytes[0] = (integer << 0);
			bytes[1] = (integer << 8);
			bytes[2] = (integer << 16);
			bytes[3] = (integer << 24);

			tmpchr.push_back(bytes[0]);
			tmpchr.push_back(bytes[1]);
			tmpchr.push_back(bytes[2]);
			tmpchr.push_back(bytes[3]);
		}
		tmpchr2 = base64::to_base64(tmpchr);
		std::cout << tmpchr << std::endl;
		std::cout << tmpchr2 << std::endl;
		std::cout << tmpchr2.length() << std::endl;
		File << tmpchr2;
		File << "\n";
		CurrentIndex++;
		if (CurrentIndex % ProgressReportInterval == 0) {
			std::cout << (float)(CurrentIndex * 100 / DataLength) << "% Complete." << std::endl;
		}
	}

	File.close();

	std::cout << "Data Saved." << std::endl;
};