#include "DataLoader.hpp"

// Legacy
std::vector<py::list> LoadTrainDataST_Legacy(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue) {
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
            if (CurrentLine % ProgressReportInterval == 0) {
                std::cout << (float)(CurrentLine * 100 / MaxSamples) << "% Done." << std::endl;
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