#include "DataLoader.hpp"

int InitModule() {
    std::cout << "GavinBackendDataSetTools Loaded. Please see readme.md for usage." << std::endl;
    return 0;
}

PYBIND11_MODULE(GavinBackendDatasetUtils, handle) {
    InitModule();
    handle.doc() = "This module is a custom module written in c++ to accelerate dataset loading for Gavin Bot made by Josh (Scot_Survivor)";
    handle.def("LoadTrainDataST_Legacy", &LoadTrainDataST_Legacy);
    handle.def("LoadTrainDataMT", &LoadTrainDataMT);
    handle.def("LoadTrainDataST", &LoadTrainDataST);

    handle.def("ConvertDataSet_TEST", &ConvertToBinFormat);

    py::class_<DataGenerator>(handle, "DataGenerator")
        .def(py::init<std::string, std::string, std::string, uint64_t, int, int, int, int>())
        .def("UpdateBuffer", & DataGenerator::UpdateDataBuffer)
        .def_readonly("ToSampleBuffer", &DataGenerator::ToSampleBufferArray_t)
        .def_readonly("FromSampleBuffer", &DataGenerator::FromSampleBufferArray_t);

    py::class_<Tokenizer>(handle, "Tokenizer")
        .def(py::init<std::string, uint64_t>())
        .def(py::init<std::string>());


#ifdef VERSION_INFO
    handle.attr("__version__") = VERSION_INFO;
#else
    handle.attr("__version__") = "dev";
#endif
}