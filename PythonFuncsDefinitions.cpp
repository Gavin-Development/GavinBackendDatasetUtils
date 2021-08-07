#include "DataLoader.hpp"

int InitModule() {
    std::cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << std::endl;
    std::cout << "This module is currently undergoing active development. Please only use the function 'LoadTrainDataST'" << std::endl;
    std::cout << "Example use for this function is as follows, testlist = LTD.LoadTrainDataST(10000000, 'C:/Users/User/Desktop/Gavin/GavinTraining/', 'Tokenizer-3.to', 69108,66109, 52, 0)" << std::endl;
    std::cout << "This will open the file and load 10000000 samples with start token 69108 and end token 69109 with width 52 and padding value 0." << std::endl;
    std::cout << "-------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << std::endl;
    return 0;
}

PYBIND11_MODULE(LoadTrainData, handle) {
    InitModule();
    handle.doc() = "This module is a custom module written in c++ to accelerate dataset loading for Gavin Bot made by Josh (Scott Survivor)";
    handle.def("LoadTrainDataST", &LoadTrainDataST);
    handle.def("LoadTrainDataMT", &LoadTrainDataMT);
    handle.def("LoadTrainDataST_Future", &LoadTrainDataST_Future);
    handle.def("SaveDataST_Future", &SaveDataST_Future);

    handle.def("ConvertDataSet_TEST", &ConvertToBinFormat);

#ifdef VERSION_INFO
    handle.attr("__version__") = VERSION_INFO;
#else
    handle.attr("__version__") = "dev";
#endif
}