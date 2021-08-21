#include "DataLoader.hpp"

std::mutex LockThreadsForWrite;

std::vector<py::list> DataLoadAndParseThread(int64_t samplesToRead, std::string Buffer, int startToken, int endToken, int sampleLength, int paddingValue, int threadId) {
	std::cout << "Thread " << threadId << " Starting."<< std::endl;
	std::vector<py::list> FileData;
	std::stringstream StringStream(Buffer);
	std::string Line;
	std::string tmpstr;
	std::vector<int> intvec;
	int64_t CurrentLine = 0;
	int64_t MaxSamples = samplesToRead;
	int64_t progresssReportInterval = samplesToRead / 100;

	try {
		while (std::getline(StringStream, Line)) {
			if (samplesToRead > 0) {
				py::list PythonList;

				tmpstr = base64::from_base64(Line);
				intvec.resize(tmpstr.size() / 4);
				memcpy(intvec.data(), tmpstr.data(), sizeof(tmpstr[0]) * tmpstr.length());
				PythonList.insert(0, startToken);
				for (int& integer : intvec) {
					PythonList.append(integer);
				}
				//std::cout << Line << std::endl;
				
				PythonList.insert(PythonList.size(), endToken);

				if (PythonList.size() < sampleLength) {
					for (int i = PythonList.size(); i < sampleLength; i++) {
						PythonList.append(paddingValue);
					}
				}

				if (PythonList.size() > sampleLength) {
					for (int i = PythonList.size() - 2; i > sampleLength - 2; i--) {
						PythonList.attr("pop")(i);
					}
				}

				if (PythonList.size() != sampleLength) {
					std::cout << PythonList.size() << std::endl;
				}
				FileData.push_back(PythonList);
				samplesToRead--;
				CurrentLine++;
				if (CurrentLine % progresssReportInterval == 0) {
					std::cout << (float)(CurrentLine * 100 / MaxSamples) << "% Done." << std::endl;
				}
			}
			else {
				std::cout << "All Samples Loaded." << std::endl;
				break;
			}

		}
	}
	catch (const std::exception& ex) {
		std::wcout << "Thread " << threadId << "Encountered An Error " << ex.what() << std::endl;
	}

	std::cout << "Thread " << threadId << "Done." << std::endl;
	return FileData;
};


std::vector<py::list> LoadTrainDataMT_Future(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue) {
	std::vector<std::thread> ProcessingThreads;
	std::vector<py::list> Data;
	std::string FileName = dataPath + tokenizerName;
	if (samplesToRead < 100) {
		std::cout << "Please Specify A Minimum Of 100 Samples To Load." << std::endl;
		return Data;
	}

	std::cout << "Loading Data Set MT -- WIP FUNCTION USE AT OWN RISK" << std::endl;


	int NumberOfThreadsToUse = 2;// std::thread::hardware_concurrency() * 0.8;

	std::ifstream File = std::ifstream(FileName, std::ios::binary | std::ios::ate);
	int64_t FileLength = File.tellg();
	int64_t FileLengthRemaining = FileLength;
	int64_t BufferSize = FileLength / NumberOfThreadsToUse;
	int64_t FileAccessIndex = 0;

	if (BufferSize > 2147483647) {
		BufferSize = 2147483647;
		NumberOfThreadsToUse = FileLength / BufferSize;
	}

	std::cout << "Using " << NumberOfThreadsToUse << " Threads." << std::endl;
	int64_t SamplesToReadPerThread = samplesToRead / NumberOfThreadsToUse;
	std::cout << "This File is " << FileLength << " Bytes Long. Each Thread Will Read " << BufferSize << " Bytes, Returning " << SamplesToReadPerThread << " Samples." << std::endl;

	int64_t StartTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	int64_t EndTime;
	int64_t TimeTaken;

	for (int64_t i = 0; i < NumberOfThreadsToUse; i++) {
		std::string Buffer;
		Buffer.resize(BufferSize);
		File.seekg(FileAccessIndex);
		File.read(Buffer.data(), sizeof(char) * BufferSize);
		ProcessingThreads.push_back(std::thread(DataLoadAndParseThread, SamplesToReadPerThread, Buffer, startToken, endToken, sampleLength, paddingValue, i));
		FileAccessIndex = FileAccessIndex + BufferSize;
	}

	for (std::thread& th : ProcessingThreads) {
		th.join();
	}

	File.close();
	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	TimeTaken = (EndTime - StartTime) / 1000000000;
	std::cout << "All File Contents Loaded In " << TimeTaken << " Second(s)." << std::endl;
	return Data;
};
