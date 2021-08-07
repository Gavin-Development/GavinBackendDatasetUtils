#include "DataLoader.hpp"

std::mutex LockThreadsForWrite;

void DataLoadAndParseThread(int64_t samplesToRead, std::string RawFileData, std::vector<py::list>* ProcessedSamples, int startToken, int endToken, int sampleLength, int paddingValue, int threadId) {
	std::cout << "Thread " << threadId << " Starting." << std::endl;
	std::string Buffer = RawFileData;

	py::object pickle = py::module_::import("pickle").attr("loads");
	py::object base64decode = py::module_::import("base64").attr("b64decode");
	std::stringstream strstream(RawFileData);
	std::string Line;
	py::list intvec;
	int64_t CurrentLine = 0;
	int64_t MaxSamples = samplesToRead;
	int64_t progresssReportInterval = 1;
	bool GettingSamples;
	if (threadId == 0) {
		GettingSamples = true;
	}
	else GettingSamples = false;


	while (std::getline(strstream, Line)) {
		if (GettingSamples == true) {
			if (samplesToRead >= 0) {
				Line.erase(Line.begin(), Line.begin() + 2);
				Line.erase(Line.end() - 2, Line.end());
				//std::cout << Line << std::endl;
				try {
					intvec = pickle(base64decode(Line));
				}
				catch (py::error_already_set& e) {
					std::cout << "Error In Parsing Base64 Data." << std::endl;
					continue;
				}

				intvec.insert(0, startToken);
				intvec.insert(intvec.size(), endToken);
				for (int i = intvec.size(); i < sampleLength; i++) {
					intvec.append(paddingValue);
				}
				LockThreadsForWrite.lock();
				ProcessedSamples->push_back(intvec);
				LockThreadsForWrite.unlock();
				samplesToRead--;
				CurrentLine++;
			}
			else {
				std::cout << "All Samples Loaded." << std::endl;
				break;
			}
		}
		else {
			GettingSamples = true;
		}


	}
};


std::vector<py::list> LoadTrainDataMT(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue) {
	std::vector<py::list> FileData;
	std::vector<std::thread> ProcessingThreads;

	if (samplesToRead < 100) {
		std::cout << "Please Specify A MINIMUM Of 100 Samples To Load." << std::endl;
		return FileData;
	}
	std::cout << "Loading Data Set MT -- WIP FUNCTION USE AT OWN RISK" << std::endl;

	int64_t StartTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	int64_t EndTime;
	int64_t TimeTaken;

	std::string FileName = dataPath + tokenizerName;
	std::ifstream File;
	int64_t FileAccessIndex = 0;
	int64_t FileLength;
	int64_t FileLengthRemaining;
	int64_t BufferSize;


	int NumberOfThreadsToUse = std::thread::hardware_concurrency() * 0.8;


	std::cout << "Using " << NumberOfThreadsToUse << " Threads." << std::endl;
	File = std::ifstream(FileName, std::ios::binary | std::ios::ate);
	FileLength = File.tellg();
	FileLengthRemaining = FileLength;
	BufferSize = FileLength / NumberOfThreadsToUse;
	if (BufferSize > 2147483647) {
		BufferSize = 2147483647;
		NumberOfThreadsToUse = FileLength / BufferSize;
		std::cout << "Processor Does Not Have Enough Threads To Satisfy Hardware Concurrency. To Limit Load Buffer Size More Threads Will Be Executed Than Hardware Threads Avalible." << std::endl;
	}
	int64_t SamplesToReadPerThread = samplesToRead / NumberOfThreadsToUse;
	std::cout << "This File is " << FileLength << " Bytes Long. Each Thread Will Read " << BufferSize << " Bytes, Returning " << SamplesToReadPerThread << " Samples." << std::endl;


	for (int64_t i = 0; i < NumberOfThreadsToUse; i++) {
		std::string Buffer;
		Buffer.resize(BufferSize);
		File.seekg(FileAccessIndex);
		File.read(Buffer.data(), sizeof(char) * BufferSize);
		FileAccessIndex = FileAccessIndex + BufferSize;
		ProcessingThreads.push_back(std::thread(DataLoadAndParseThread, SamplesToReadPerThread, Buffer, &FileData, startToken, endToken, sampleLength, paddingValue, i));
	}

	for (auto& th : ProcessingThreads) {
		th.join();
	}

	File.close();
	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	TimeTaken = (EndTime - StartTime) / 1000000000;
	std::cout << "All File Contents Loaded In " << TimeTaken << " Second(s)." << std::endl;
	return FileData;
};

















int tmpint;
while (std::getline(File, Line)) {
	if (samplesToRead > 0) {
		//Line.erase(Line.end() - 1, Line.end());
		tmpstr = base64::from_base64(Line);
		//std::cout << Line << std::endl;
		std::cout << tmpstr << std::endl;
		std::cout << tmpstr.length() << std::endl;
		for (int i = 0; i < (tmpstr.length() / 4); i++) {
			//std::cout << "Converting char to int." << std::endl;
			//tmpint = int(
			//	(uint8_t)(tmpstr[(i * 4) + 3]) << 24 |
			//	(uint8_t)(tmpstr[(i * 4) + 2]) << 16 |
			//	(uint8_t)(tmpstr[(i * 4) + 1]) << 8  |
			//	(uint8_t)(tmpstr[(i * 4) + 0])
			//);

			tmpint = static_cast<int>(static_cast<unsigned char>(tmpstr[(i * 4) + 0]) << 24 |
				static_cast<unsigned char>(tmpstr[(i * 4) + 1]) << 16 |
				static_cast<unsigned char>(tmpstr[(i * 4) + 2]) << 8 |
				static_cast<unsigned char>(tmpstr[(i * 4) + 3])) << 0;

			PythonList.append(tmpint);

			//tmpintvec.push_back( (tmpstr[(i * 4) + 0] << 24) + (tmpstr[(i * 4) + 1] << 16) + (tmpstr[(i * 4) + 2] << 8) + (tmpstr[(i * 4) + 3]) );                                 // (tmpstr[(i * 4)] << 24) + (tmpstr[(i * 4) + 1] << 16) + (tmpstr[(i * 4) + 2] << 8) + (tmpstr[(i * 4) + 3]
			//std::cout << tmpintvec[i] << std::endl;
		}
		FileData.emplace_back(PythonList);
		samplesToRead--;
		CurrentLine++;
		if (CurrentLine % ProgressReportInterval == 0) {
			std::cout << (float)(CurrentLine * 100 / MaxSamples) << "% Done." << std::endl;
		}
	}
	else {
		std::cout << "There Arent Enough Samples In File. But Avalible Samples Were Loaded." << std::endl;
		break;
	}

}