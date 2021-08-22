#include "DataLoader.hpp"

void ConvertToBinFormat(int64_t samplesToRead, std::string fileToLoad, std::string fileToSave) {
	std::cout << "Converting Old Files To .BIN Format." << std::endl;
	// variables for progress tracking.
	int64_t CurrentLine = 0;
	int64_t MaxSamples = samplesToRead;
	int64_t ProgressReportInterval = MaxSamples / 100;
	int64_t StartTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	int64_t EndTime;
	// variables for loading & de serialising the data.
	py::object pickle = py::module_::import("pickle").attr("loads");
	py::object base64decode = py::module_::import("base64").attr("b64decode");
	std::ifstream Infile = std::ifstream(fileToLoad);
	std::string Line;
	py::list LineData;
	// variables for converting & saving the data.
	std::ofstream Outfile = std::ofstream(fileToSave, std::ios::app | std::ios::binary);
	std::vector<int> EncodeVec_Int32;
	std::vector<uint16_t> EncodeVec_Int16;
	std::vector<BIN::SampleHeaderData> FileHeaderContents;
	std::vector<std::string> FileDataContents;
	int64_t CurrentDataSectionOffset = 0;
	bool int16compatible;
	std::string EncodeVectorString;


	if (Infile.is_open()) {
		std::cout << "Loading (Up To) " << samplesToRead << " Samples." << std::endl;
	}
	else {
		std::cout << "Failed To Open File." << std::endl;
		return;
	}

	// Read and convert samples into RAM.
	while (std::getline(Infile, Line)) {
		if (samplesToRead > 0) {
			// Reset loop variables and prepare the line for base 64 decoding.
			int16compatible = true;
			Line.erase(Line.begin(), Line.begin() + 2);
			Line.erase(Line.end() - 1, Line.end());

			// decode and unpickle the line.
			LineData = pickle(base64decode(Line));

			// Convert the py::list to std::vector<int>
			for (size_t i = 0; i < LineData.size(); i++) {
				EncodeVec_Int32.push_back(py::cast<int>(LineData[i]));
			}

			// checking if int16 compatibility is possible.
			for (size_t i = 0; i < EncodeVec_Int32.size(); i++) {
				if (EncodeVec_Int32[i] > UINT16_MAX) {
					int16compatible = false;
				}
			}

			if (int16compatible) {
				// Cast the 32 bit array to a 16 bit representation.
				EncodeVec_Int16.resize(EncodeVec_Int32.size());
				for (size_t i = 0; i < EncodeVec_Int16.size(); i++) {
					EncodeVec_Int16[i] = static_cast<uint16_t>(EncodeVec_Int32[i]);
				}
				// Convert this to a string.
				EncodeVectorString.resize(EncodeVec_Int16.size() * sizeof(uint16_t) / sizeof(char));
				memcpy(EncodeVectorString.data(), EncodeVec_Int16.data(), sizeof(EncodeVec_Int16[0]) * EncodeVec_Int16.size());
				FileDataContents.push_back(EncodeVectorString);
				// Create a struct informing the program the offset, length & dtype of this sample in bytes.
				BIN::SampleHeaderData Metadata;
				Metadata.dtypeint16 = 1;
				Metadata.SampleLength = FileDataContents[FileDataContents.size() - 1].size();
				Metadata.OffsetFromDataSectionStart = CurrentDataSectionOffset;
				FileHeaderContents.push_back(Metadata);
				// Finally incriment the data section offset by the length of this sample in bytes.
				CurrentDataSectionOffset = CurrentDataSectionOffset + FileDataContents[FileDataContents.size() - 1].size();
			}
			else {
				// Convert this to a string.
				EncodeVectorString.resize(EncodeVec_Int32.size() * sizeof(uint32_t) / sizeof(char));
				memcpy(EncodeVectorString.data(), EncodeVec_Int32.data(), sizeof(EncodeVec_Int32[0]) * EncodeVec_Int32.size());
				FileDataContents.push_back(EncodeVectorString);
				// Create a struct informing the program the offset, length & dtype of this sample in bytes.
				BIN::SampleHeaderData Metadata;
				Metadata.dtypeint16 = 0;
				Metadata.SampleLength = FileDataContents[FileDataContents.size() - 1].size();
				Metadata.OffsetFromDataSectionStart = CurrentDataSectionOffset;
				FileHeaderContents.push_back(Metadata);
				// Finally incriment the data section offset by the length of this sample in bytes.
				CurrentDataSectionOffset = CurrentDataSectionOffset + FileDataContents[FileDataContents.size() - 1].size();
			}

			// Reset some loop variables.
			EncodeVec_Int32.clear();
			samplesToRead--;
			CurrentLine++;
			if (CurrentLine % ProgressReportInterval == 0) {
				std::cout << (float)(CurrentLine * 100 / MaxSamples) << "% Done." << std::endl;
			}
		}
		else break;
		


	}
	Infile.close();

	// Save File Header.

	std::cout << "Samples loaded, saving to BIN file." << std::endl;

	std::string FileWriteBuffer;
	uint64_t HeaderSectionLength = FileHeaderContents.size() * sizeof(FileHeaderContents[0]) + sizeof(uint64_t);
	uint64_t NumberOfSamplesInFile = FileDataContents.size();
	FileWriteBuffer.resize(sizeof(uint64_t));
	memcpy(FileWriteBuffer.data(), &HeaderSectionLength, sizeof(uint64_t));
	Outfile << FileWriteBuffer;
	memcpy(FileWriteBuffer.data(), &NumberOfSamplesInFile, sizeof(uint64_t));
	Outfile << FileWriteBuffer;
	for (BIN::SampleHeaderData SampleData : FileHeaderContents) {
		FileWriteBuffer.resize(sizeof(SampleData));
		memcpy(FileWriteBuffer.data(), &SampleData, 11); // we go with 11 as compiler default aligns the struct to 16 bytes but we only want the first 11.
		Outfile << FileWriteBuffer;
	}
	// Save the File Data.
	for (std::string& Sample : FileDataContents) {
		Outfile << Sample;
	}
	// Close the file to signal to the OS that operations on it are done.
	Outfile.close();

	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	std::cout << "Time Taken: " << (EndTime - StartTime) / 1000000000 << " Seconds." << std::endl;
};



//Outfile.write((const char*)&HeaderSectionLength, sizeof(uint64_t)); // I like this as its a nifty little trick to write its raw bytes!