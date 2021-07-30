#include "DataLoader.hpp"

// Legacy
std::vector<py::list> LoadTrainDataST(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue) {
	std::vector<py::list> FileData;
	if (samplesToRead < 100) {
		std::cout << "Please Specify A MINIMUM Of 100 Samples To Load." << std::endl;
		return FileData;
	}
	int64_t StartTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	int64_t EndTime;
	int64_t TimeTaken;
	std::string FileName = dataPath + tokenizerName;
	std::ifstream File;
	int64_t MaxSamples = samplesToRead;
	int64_t CurrentLine = 0;
	int64_t ProgressReportInterval = MaxSamples / 100;
	py::object pickle = py::module_::import("pickle").attr("loads");
	py::object base64decode = py::module_::import("base64").attr("b64decode");

	std::cout << "Loading " << samplesToRead << " Samples From " << FileName << std::endl;

	File = std::ifstream(FileName);
	std::string Line;
	py::list intvec;

	if (File.is_open()) {
		std::cout << "Loading (Up To) " << MaxSamples << " Samples." << std::endl;
	}
	else {
		std::cout << "Failed To Open File." << std::endl;
		return FileData;
	}

	while (std::getline(File, Line)) {
		if (samplesToRead > 0) {
			Line.erase(Line.begin(), Line.begin() + 2);
			Line.erase(Line.end() - 1, Line.end());
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
			FileData.push_back(intvec);
			samplesToRead--;
			CurrentLine++;
			if (CurrentLine % ProgressReportInterval == 0) {
				std::cout << (float)(CurrentLine * 100 / MaxSamples)  << "% Done." << std::endl;
			}
		}
		else {
			std::cout << "All Samples Loaded." << std::endl;
			break;
		}
		
	}

	File.close();
	std::cout << "Samples Have Been Read." << std::endl;
	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	TimeTaken = (EndTime - StartTime) / 1000000000;
	std::cout << "Time Taken: " << TimeTaken << " Seconds." << std::endl;
	return FileData;
};























// current - THIS CODE DOES NOT FUNCTION CORRECTLY DO NO USE

std::vector<std::vector<int>> LoadTrainDataST_Future(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue) {
	std::cout << "WARNING THI FUNCTION MIGHT NOT WORK AS INTENDED DO NOT USE PROPERLY." << std::endl;
	std::vector<std::vector<int>> FileData;
	if (samplesToRead < 100) {
		std::cout << "Please Specify A MINIMUM Of 100 Samples To Load." << std::endl;
		return FileData;
	}
	int64_t StartTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	int64_t EndTime;
	int64_t TimeTaken;
	std::string FileName = dataPath + tokenizerName;
	std::ifstream File;
	int64_t MaxSamples = samplesToRead;
	int64_t CurrentLine = 0;
	int64_t ProgressReportInterval = MaxSamples / 100;

	std::cout << "Loading " << samplesToRead << " Samples From " << FileName << std::endl;

	File = std::ifstream(FileName);
	std::string Line;
	std::string tmpstr;
	std::vector<int> tmpintvec;

	if (File.is_open()) {
		std::cout << "Loading (Up To) " << MaxSamples << " Samples." << std::endl;
	} 
	else {
		std::cout << "Failed To Open File." << std::endl;
		return FileData;
	}

	
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

				tmpint = static_cast<int>(static_cast<unsigned char>(tmpstr[(i * 4) + 0]) << 1 |
					static_cast<unsigned char>(tmpstr[(i * 4) + 1]) << 0 |
					static_cast<unsigned char>(tmpstr[(i * 4) + 2]) << 0 |
					static_cast<unsigned char>(tmpstr[(i * 4) + 3])) << 0;

				tmpintvec.push_back(tmpint);

				//tmpintvec.push_back( (tmpstr[(i * 4) + 0] << 24) + (tmpstr[(i * 4) + 1] << 16) + (tmpstr[(i * 4) + 2] << 8) + (tmpstr[(i * 4) + 3]) );                                 // (tmpstr[(i * 4)] << 24) + (tmpstr[(i * 4) + 1] << 16) + (tmpstr[(i * 4) + 2] << 8) + (tmpstr[(i * 4) + 3]
				//std::cout << tmpintvec[i] << std::endl;
			}
			FileData.emplace_back(tmpintvec);
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

	File.close();
	std::cout << "Samples Have Been Read." << std::endl;
	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	TimeTaken = (EndTime - StartTime) / 1000000000;
	std::cout << "Time Taken: " << TimeTaken << " Seconds." << std::endl;
	return FileData;
};

void SaveDataST_Future(std::vector<std::vector<int>> Data, std::string FileName) {
	std::cout << "WARNING THI FUNCTION MIGHT NOT WORK AS INTENDED DO NOT USE PROPERLY." << std::endl;
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