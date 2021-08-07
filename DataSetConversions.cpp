#include "DataLoader.hpp"

void ConvertToBinFormat(int64_t samplesToRead, std::string fileToLoad, std::string fileToSave) {
	std::cout << "Converting Old Files To .BIN Format." << std::endl;
	py::object pickle = py::module_::import("pickle").attr("loads");
	py::object base64decode = py::module_::import("base64").attr("b64decode");
	std::ifstream Infile = std::ifstream(fileToLoad);
	std::ofstream Outfile = std::ofstream(fileToSave, std::ios::app | std::ios::binary);
	std::string Line;
	py::list LineData;
	std::vector<int> intvec;
	char bytes[4];
	std::string DataToWrite;


	if (Infile.is_open()) {
		std::cout << "Loading (Up To) " << samplesToRead << " Samples." << std::endl;
	}
	else {
		std::cout << "Failed To Open File." << std::endl;
		return;
	}

	while (std::getline(Infile, Line)) {
		if (samplesToRead > 0) {
			Line.erase(Line.begin(), Line.begin() + 2);
			Line.erase(Line.end() - 1, Line.end());

			LineData = pickle(base64decode(Line));

			for (size_t i = 0; i < LineData.size(); i++) {
				intvec.push_back(py::cast<int>(LineData[i]));
			}
			DataToWrite.resize(intvec.size() * 4);
			memcpy(DataToWrite.data(), intvec.data(), sizeof(intvec[0]) * intvec.size());

			Line = base64::to_base64(DataToWrite);
			Line = Line + "\n";
			Outfile << Line;
			DataToWrite.clear();
			intvec.clear();
			samplesToRead--;
		}
		else break;


	}

	Infile.close();
	Outfile.close();
};

//
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 	std::stringstream strstream;
// 
// 
// 			strstream.clear();
//std::copy(intvec.begin(), intvec.end(), std::ostream_iterator<int>(strstream, " "));
//std::cout << strstream.str() << std::endl;
//Line = strstream.str();
//