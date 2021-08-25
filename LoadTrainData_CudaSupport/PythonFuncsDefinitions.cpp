#include "DataLoader.hpp"

int InitModule() {
    std::cout << "GavinBackendDataSetTools Loaded. Please see readme.md for usage." << std::endl;
    return 0;
}

PYBIND11_MODULE(LoadTrainData_CudaSupport, handle) {
    InitModule();
    handle.doc() = "This module is a custom module written in c++ to accelerate dataset loading for Gavin Bot made by Josh (Scott Survivor)";
    handle.def("LoadTrainDataST_Legacy", &LoadTrainDataST_Legacy);
    handle.def("LoadTrainDataMT", &LoadTrainDataMT);
    handle.def("LoadTrainDataST", &LoadTrainDataST);
    handle.def("SaveTrainDataST", &SaveTrainDataST);

    handle.def("ConvertDataSet_TEST", &ConvertToBinFormat);

#ifdef VERSION_INFO
    handle.attr("__version__") = VERSION_INFO;
#else
    handle.attr("__version__") = "dev";
#endif
}