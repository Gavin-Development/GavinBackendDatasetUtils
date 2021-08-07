#include "DataLoader.hpp"

std::mutex LockThreadsForWrite;

std::vector<py::list> DataLoadAndParseThread(int64_t samplesToRead, std::string FileName, int startToken, int endToken, int sampleLength, int paddingValue, int threadId) {
	std::cout << "Thread " << threadId << " Starting. Opening File: " << FileName << std::endl;
	std::fstream File(FileName);
	std::vector<py::list> FileData;
	//py::object pickle = py::module_::import("pickle").attr("loads");
	//py::object base64decode = py::module_::import("base64").attr("b64decode");
	std::string Line;
	py::list intvec;
	int64_t CurrentLine = 0;
	int64_t MaxSamples = samplesToRead;
	int64_t progresssReportInterval = samplesToRead / 100;

	try {
		while (std::getline(File, Line)) {
			if (samplesToRead > 0) {
				Line.erase(Line.begin(), Line.begin() + 2);
				Line.erase(Line.end() - 1, Line.end());
				//std::cout << Line << std::endl;
				try {
					//LockThreadsForWrite.lock();
					//intvec = pickle(base64decode(Line));
					//LockThreadsForWrite.unlock();
				}
				catch (py::error_already_set& e) {
					std::cout << "Error In Parsing Base64 Data." << std::endl;
					continue;
				}
				intvec.insert(0, startToken);
				intvec.insert(intvec.size(), endToken);

				if (intvec.size() < sampleLength) {
					for (int i = intvec.size(); i < sampleLength; i++) {
						intvec.append(paddingValue);
					}
				}

				if (intvec.size() > sampleLength) {
					for (int i = intvec.size() - 2; i > sampleLength - 2; i--) {
						intvec.attr("pop")(i);
					}
				}

				if (intvec.size() != sampleLength) {
					std::cout << intvec.size() << std::endl;
				}

				FileData.push_back(intvec);
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
	

	File.close();
	std::cout << "Thread " << threadId << "Done." << std::endl;

	return FileData;
};


std::tuple< std::vector<py::list>, std::vector<py::list>> LoadTrainDataMT(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue) {
	std::vector<std::thread> ProcessingThreads;
	std::vector<py::list> ToFileData;
	std::vector<py::list> FromFileData;

	std::cout << "Loading Data Set MT -- WIP FUNCTION USE AT OWN RISK" << std::endl;

	int64_t StartTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	int64_t EndTime;
	int64_t TimeTaken;

	std::string FileName = dataPath + tokenizerName;

	std::future<std::vector<py::list>> ToFileDataLoader =  std::async(std::launch::async, DataLoadAndParseThread, samplesToRead, FileName + ".to", startToken, endToken, sampleLength, paddingValue, 0);
	std::future<std::vector<py::list>> FromFileDataLoader = std::async(std::launch::async, DataLoadAndParseThread, samplesToRead, FileName + ".from", startToken, endToken, sampleLength, paddingValue, 1);

	ToFileData = ToFileDataLoader.get();
	FromFileData = FromFileDataLoader.get();

	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	TimeTaken = (EndTime - StartTime) / 1000000000;
	std::cout << "All File Contents Loaded In " << TimeTaken << " Second(s)." << std::endl;
	return { ToFileData, FromFileData };
};
