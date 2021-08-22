#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <future>
#include <stdio.h>
#include <cstdlib>
#include <stdexcept>
#include <chrono>
#include <mutex>
#include <tuple>
#include <vector>


#include "base64.hpp"

//PYBIND11_MAKE_OPAQUE(std::vector<int>);

namespace py = pybind11;

struct GPU_Accelerated_Line {
	int64_t Index;

};


// Legacy

std::vector<py::list> LoadTrainDataST_Legacy(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);

//future

std::vector<py::list> LoadTrainDataST(uint64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);
std::vector<py::list> LoadTrainDataMT(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);
std::vector<py::list> LoadTrainDataGPU_Accelerated_Future(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);

void SaveTrainDataST(std::vector<std::vector<int>> Data, std::string FileName);

void ConvertToBinFormat(int64_t samplesToRead, std::string fileToLoad, std::string fileToSave);


namespace BIN {
	struct SampleHeaderData {
		uint64_t alignas(uint64_t) OffsetFromDataSectionStart;
		uint16_t alignas(uint16_t) SampleLength;
		uint8_t alignas(uint8_t) dtypeint16;
	};
};