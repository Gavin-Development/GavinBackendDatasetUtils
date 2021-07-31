#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <stdio.h>
#include <stdexcept>
#include <chrono>
#include <mutex>



#include "base64.hpp"

PYBIND11_MAKE_OPAQUE(std::vector<int>);

namespace py = pybind11;

struct ThreadDataRTN {
    std::vector<py::list> ProcessedData;
    int64_t NumberOfItems;
};





std::vector<py::list> LoadTrainDataST(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);

std::vector<py::list> LoadTrainDataMT(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);









std::vector<std::vector<int>> LoadTrainDataST_Future(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);

void SaveDataST_Future(std::vector<std::vector<int>> Data, std::string FileName);

