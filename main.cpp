#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include <stdio.h>
#include <stdexcept>
#include <chrono>


namespace py = pybind11;

struct ThreadDataRTN {
    std::vector<int> ProcessedData;
    int64_t NumberOfItems;
};

static char CharToSixBit(char c) {
    char lookupTable[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
        'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };

    if (c == '=')
    {
        return 0;
    }
    else
    {
        for (int x = 0; x < 64; x++)
        {
            if (lookupTable[x] == c)
                return (char)x;
        }

        return 0;
    }
}

static std::string Decode(std::string data) {
    int length, length2, length3;
    int blockCount;
    int paddingCount = 0;
    int dataLength = data.length();

    length = dataLength;
    blockCount = length / 4;
    length2 = blockCount * 3;

    for (int x = 0; x < 2; x++)
    {
        if (data[length - x - 1] == '=')
            paddingCount++;
    }

    char* buffer = new char[length];
    char* buffer2 = new char[length2];

    for (int x = 0; x < length; x++)
    {
        buffer[x] = CharToSixBit(data[x]);
    }

    char b, b1, b2, b3;
    char temp1, temp2, temp3, temp4;

    for (int x = 0; x < blockCount; x++)
    {
        temp1 = buffer[x * 4];
        temp2 = buffer[x * 4 + 1];
        temp3 = buffer[x * 4 + 2];
        temp4 = buffer[x * 4 + 3];

        b = (char)(temp1 << 2);
        b1 = (char)((temp2 & 48) >> 4);
        b1 += b;

        b = (char)((temp2 & 15) << 4);
        b2 = (char)((temp3 & 60) >> 2);
        b2 += b;

        b = (char)((temp3 & 3) << 6);
        b3 = temp4;
        b3 += b;

        buffer2[x * 3] = b1;
        buffer2[x * 3 + 1] = b2;
        buffer2[x * 3 + 2] = b3;
    }

    length3 = length2 - paddingCount;
    std::string result;

    for (int x = 0; x < length3; x++)
    {
        result += buffer2[x];
    }

    delete[] buffer;
    delete[] buffer2;

    return result;
}

void SearchAndStrip(char* pchar, int64_t pcharlen, const char* example) { // Depreciated.
    int examplelen = strlen(example);
    std::cout << examplelen << std::endl;
    bool totalmatch = false;

    int64_t newlen = pcharlen;

    for (int i = 0; i < pcharlen; i++) {
        if (pchar[i] == example[0]) {
            for (int j = 0; j < examplelen; j++) {
                if (pchar[j + i] == example[j]) {
                    totalmatch = true;
                }
                else {
                    totalmatch = false;
                    break;
                }
            }

            if (totalmatch == true) {
                std::cout << example << " Found In Data At Index " << i << std::endl;
                newlen = newlen - examplelen;
                totalmatch = false;
            }

        }
    }
};

void DataSorterThread(int64_t BlockSize, int64_t BlockStartIndex, char* pDataArr, int ThreadNumber, int64_t FileLength, ThreadDataRTN* DataReturnStruct, int StartToken, int EndToken, int PaddingValue, int SampleLength, int64_t samplestoread) {

    std::cout << "Thread " << ThreadNumber << " Preparing Data." << std::endl;

    int startexamplelen = 2;
    const char* startexample = "b'";
    int endexamplelen = 1;
    const char* endexample = "'";
    bool totalmatch = false;

    int64_t StartIndex = BlockStartIndex;
    int64_t EndIndex = BlockStartIndex;
    int64_t IdentifiedSampleLength = 0;

    for (int64_t i = BlockStartIndex; i < (BlockStartIndex + BlockSize); i++) {
        if (pDataArr[i] == startexample[0]) {
            for (int j = 0; j < startexamplelen; j++) {
                if (pDataArr[j + i] == startexample[j]) {
                    totalmatch = true;
                }
                else {
                    totalmatch = false;
                    break;
                }
            }

            if (totalmatch == true) {
                StartIndex = i + startexamplelen;
                totalmatch = false;
            }

        }

        if (pDataArr[i] == endexample[0]) {
            for (int j = 0; j < endexamplelen; j++) {
                if (pDataArr[i + j] == endexample[j]) {
                    totalmatch = true;
                }
                else {
                    totalmatch = false;
                    break;
                }
            }

            if (totalmatch == true) {
                // Now we have identified the range of the chunk we need to process it into int64_t.
                EndIndex = i - 1;
                totalmatch = false;
                IdentifiedSampleLength = EndIndex - StartIndex;
                if (IdentifiedSampleLength <= 0) break;
                std::string tmp;
                std::cout << IdentifiedSampleLength << std::endl;
                tmp.resize(IdentifiedSampleLength);
                for (int l = 0; l < tmp.size(); l++) {
                    tmp[l] = pDataArr[StartIndex + l];
                }
                //memcpy(&tmp, &pDataArr[StartIndex], IdentifiedSampleLength);
                std::string DecodedValues = Decode(tmp);

                for (int m = 0; m < IdentifiedSampleLength / 4; m++) {
                    DataReturnStruct->ProcessedData.push_back((int)DecodedValues[(m * 4), (m * 4) + 1, (m * 4) + 2, (m * 4) + 3]);
                }
                std::cout << DecodedValues << std::endl;
                DataReturnStruct->NumberOfItems = DataReturnStruct->NumberOfItems + 1;
            }
        }

        if (DataReturnStruct->NumberOfItems >= samplestoread) {
            break;
        }

        if (i >= FileLength) {
            break;
        }
    }

    std::cout << "Thread " << ThreadNumber << "Done." << std::endl;
};


std::vector<int> LoadTrainData(int64_t maxSamples, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue) {

    std::vector<uint8_t> RawFileData;
    std::vector<int> EncodedData;
    int NumberOfThreadsAvalible;
    std::vector<std::thread> ProcessingThreads;
    std::vector<ThreadDataRTN> ThreadDataRTNVec;
    int64_t FileBlockSize;
    int64_t FileLength;
    int64_t SamplesToRead;


    // Open the file and gather its length to divide it up amongst all threads.
    std::string FileName;
    FileName = dataPath + tokenizerName + ".to";
    std::cout << "Opening File " << FileName << std::endl;
    std::ifstream from(FileName, std::ios::binary | std::ios::beg);
    from.seekg(0, std::ios::end);
    FileLength = from.tellg();
    from.seekg(0, std::ios::beg);

    std::cout << "Filed Length In Bytes: " << FileLength << std::endl;
    RawFileData = (char*)malloc(sizeof(char) * FileLength);

    std::cout << "Allocated DRAM for the data." << std::endl;

    std::cout << "File Opened." << std::endl;
    NumberOfThreadsAvalible = std::thread::hardware_concurrency();
    std::cout << NumberOfThreadsAvalible << " Threads Avalible." << std::endl;
    NumberOfThreadsAvalible = NumberOfThreadsAvalible - (NumberOfThreadsAvalible * 0.2);
    std::cout << "Using " << NumberOfThreadsAvalible << " Threads." << std::endl;
    NumberOfThreadsAvalible = NumberOfThreadsAvalible / 2;
    FileBlockSize = FileLength / NumberOfThreadsAvalible;
    SamplesToRead = maxSamples / NumberOfThreadsAvalible;
    std::cout << "Each Thread Will Read " << FileBlockSize << " Bytes Of The File." << std::endl;
    std::cout << "Each Thread Will Return " << SamplesToRead << " Samples." << std::endl;


    ThreadDataRTNVec.resize(NumberOfThreadsAvalible);

    from.read(RawFileData, FileLength);
    from.close();

    std::cout << "Searching for b'" << std::endl;

    //SearchAndStrip(RawFileData, FileLength, "b'");

    int64_t StartIndex = 0;
    for (size_t i = 0; i < NumberOfThreadsAvalible; i++) {
        StartIndex = i * FileBlockSize;
        std::cout << "Start Index: " << StartIndex << std::endl;
        ProcessingThreads.emplace_back(std::thread(DataSorterThread, FileBlockSize, StartIndex, RawFileData, i, FileLength, &ThreadDataRTNVec[i], startToken, endToken, paddingValue, sampleLength, SamplesToRead));
        std::cout << "Launched Thread " << i << "." << std::endl;
    }

    std::cout << "Threads Launched." << std::endl;

    for (auto& th : ProcessingThreads) {
        th.join();
    }

    std::cout << "All Threads Terminated." << std::endl;

    for (int i = 0; i < 1000; i++) {
        std::cout << RawFileData[i];
    }
    std::cout << std::endl;

    std::cout << "Dataset Loading Done." << std::endl;
    for (ThreadDataRTN RTNstruct : ThreadDataRTNVec) {
        for (int number : RTNstruct.ProcessedData) {
            EncodedData.push_back(number);
        }
    }
    return EncodedData;

};

PYBIND11_MODULE(LoadTrainData, handle) {
    handle.doc() = "This is a test.";
    handle.def("LoadTrainData", &LoadTrainData);

    #ifdef VERSION_INFO
        handle.attr("__version__") = VERSION_INFO;
    #else
        handle.attr("__version__") = "dev";
    #endif
}
























































for (int64_t i = 0; i < FileLength; i++) {
    if (RawFileData[i] == StartExample[0]) {
        for (int j = 0; j < StartExampleLen; j++) {
            if (RawFileData[i + j] == StartExample[j]) {
                Match = true;
            }
            else {
                Match = false;
                break;
            }

        }

        if (Match == true) {
            StartIndex = i + StartExampleLen;
            i = i + StartExampleLen;
            Match = false;
        }
    }

    if (RawFileData[i] == EndExample[0]) {
        for (int j = 0; j < EndExampleLen; j++) {
            if (RawFileData[i + j] == EndExample[j]) {
                Match = true;
            }
            else {
                Match = false;
                break;
            }
        }

        if (Match == true) {
            EndIndex = i - 1;
            IdentifiedSampleLength = EndIndex - StartIndex;
            if (IdentifiedSampleLength <= 2) {
                Match = false;
                continue;
            }

            //std::cout << "Identified a sample." << std::endl;

            Sample.resize(IdentifiedSampleLength);
            for (int j = 0; j < IdentifiedSampleLength; j++) {
                Sample[j] = RawFileData[i + j];
            };
            DecodedValues = Decode(Sample);
            std::vector<int> DecodedInts;
            for (int j = 0; j < IdentifiedSampleLength / 4; j++) {
                DecodedInts.push_back((int)DecodedValues[(j * 4), (j * 4) + 1, (j * 4) + 2, (j * 4) + 3]);
            }
            DecodedData.emplace_back(DecodedInts);
            samplesToRead--;
            if (samplesToRead <= 0) break;
        }
    }
}





































void DataLoadAndParseThread(int64_t samplesToRead, std::string FileName, int64_t blockStartIndex, int64_t blockSize, std::vector<py::list>*returnData, int64_t progresssReportInterval, int startToken, int endToken, int sampleLength, int paddingValue, int threadId) {
    py::object pickle = py::module_::import("pickle").attr("loads");
    py::object base64decode = py::module_::import("base64").attr("b64decode");
    std::ifstream File(FileName, std::ios::ate);
    File.seekg(blockStartIndex);
    std::string Line;
    bool GettingSamples;
    if (blockStartIndex == 0) {
        GettingSamples = true;
    }
    else GettingSamples = false;
    py::list intvec;
    int64_t CurrentLine = 0;
    int64_t MaxSamples = samplesToRead;
    int64_t FileAccessLimitIndex = blockStartIndex + blockSize;
    //std::cout << "Thread " << threadId << " Located At Byte " << File.tellg() << " In File." << std::endl;

    while (std::getline(File, Line)) {
        if (GettingSamples == true) {
            if (samplesToRead > 0 && File.tellg() < FileAccessLimitIndex) {
                Line.erase(Line.begin(), Line.begin() + 2);
                Line.erase(Line.end() - 1, Line.end());
                intvec = pickle(base64decode(Line));
                intvec.insert(0, startToken);
                intvec.insert(intvec.size(), endToken);
                for (int i = intvec.size(); i < sampleLength; i++) {
                    intvec.append(paddingValue);
                }
                //std::cout << "Thread " << threadId << " About To Push Back To vector." << std::endl;
                //LockThreadsForWrite.lock();
                //returnData->push_back(intvec);
                //LockThreadsForWrite.unlock();
                //std::cout << "Thread " << threadId << " Pushed Back To vector." << std::endl;
                samplesToRead--;
                CurrentLine++;
                if (CurrentLine % progresssReportInterval == 0) {
                    //std::cout << (CurrentLine * 100 / MaxSamples) << "% Done." << std::endl;
                }
            }
            else {
                break;
            }
        }
        else {
            GettingSamples = true;
        }
    };
    File.close();
    //std::cout << "Thread " << threadId << " Done." << std::endl;


};





















int startexamplelen = 2;
const char* startexample = "b'";
int endexamplelen = 1;
const char* endexample = "'";
bool totalmatch = false;

int64_t StartIndex = 0;
int64_t EndIndex = 0;
int64_t IdentifiedSampleLength = 0;

std::cout << "Thread " << threadId << " Starting." << std::endl;
std::cout << RawFileData.size() << std::endl;
for (int i = 0; i < 1000; i++) {
    std::cout << RawFileData[i];
}
std::cout << std::endl;
for (int64_t i = 0; i < RawFileData.size(); i++) {
    if (RawFileData[i] == startexample[0]) {
        for (int j = 0; j < startexamplelen; j++) {
            if (RawFileData[j + i] == startexample[j]) {
                totalmatch = true;
            }
            else {
                totalmatch = false;
                break;
            }
        }

        if (totalmatch == true) {
            StartIndex = i + startexamplelen;
            totalmatch = false;
        }

    }

    if (RawFileData[i] == endexample[0]) {
        for (int j = 0; j < endexamplelen; j++) {
            if (RawFileData[i + j] == endexample[j]) {
                totalmatch = true;
            }
            else {
                totalmatch = false;
                break;
            }
        }

        if (totalmatch == true) {
            // Now we have identified the range of the chunk we need to process it into int64_t.
            EndIndex = i - 1;
            totalmatch = false;
            IdentifiedSampleLength = EndIndex - StartIndex;
            if (IdentifiedSampleLength <= 0) break;
            std::cout << "Identified A Sample Of Length " << IdentifiedSampleLength << std::endl;
            Line.resize(IdentifiedSampleLength);
            std::cout << "String Resized." << std::endl;
            for (int j = 0; j < IdentifiedSampleLength; j++) {
                Line[j] = RawFileData[i + j];
            }
            std::cout << "Sample Copied To String" << std::endl;
            std::cout << Line << std::endl;

            if (IdentifiedSampleLength <= 0) break;
        }
    }
}