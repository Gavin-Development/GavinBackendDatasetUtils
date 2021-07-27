#include "DataLoader.hpp"


PYBIND11_MODULE(LoadTrainData, handle) {
    handle.doc() = "This module is a custom module written in c++ to accelerate dataset loading for Gavin Bot made by Josh (Scott Survivor)";
    handle.def("LoadTrainDataST", &LoadTrainDataST);
    handle.def("LoadTrainDataST_Future", &LoadTrainDataST_Future);
    handle.def("SaveDataST_Future", &SaveDataST_Future);

#ifdef VERSION_INFO
    handle.attr("__version__") = VERSION_INFO;
#else
    handle.attr("__version__") = "dev";
#endif
}