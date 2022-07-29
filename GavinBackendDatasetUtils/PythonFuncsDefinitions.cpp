#include "DataLoader.hpp"

int InitModule() {
    std::cout << "GavinBackendDataSetTools Loaded. Please see readme.md for usage. THIS VERSION IS ONEAPI ACCELERATED." << std::endl;

    std::cout << "Module init completed." << std::endl;
    return 0;
}

PYBIND11_MODULE(GavinBackendDatasetUtils, handle) {
    InitModule();
    handle.doc() = "This module is a custom module written in c++ to accelerate dataset loading for Gavin Bot made by Josh (Scot_Survivor)";
    handle.def("LoadTrainDataMT", &LoadTrainDataMT);
    handle.def("LoadTrainDataST", &LoadTrainDataST);

    py::class_<Tokenizer>(handle, "Tokenizer")
        .def(py::init<std::string>())
        .def(py::init<>())

        .def_readonly("Words", &Tokenizer::Encodings)
        .def_readonly("Occurrences", &Tokenizer::Commonalities)
        .def("Save", static_cast<bool (Tokenizer::*)(void)>(&Tokenizer::SaveTokenizer))
        .def("Save", static_cast<bool (Tokenizer::*)(std::string)>(&Tokenizer::SaveTokenizer))
        .def("Load", static_cast<bool (Tokenizer::*)(void)>(&Tokenizer::LoadTokenizer))
        .def("Load", static_cast<bool (Tokenizer::*)(std::string)>(&Tokenizer::LoadTokenizer))
        .def("Encode", &Tokenizer::Encode)
        .def("GetVocabSize", &Tokenizer::GetVocabSize)
        .def("Decode", &Tokenizer::Decode)
        .def("BuildEncodes_GPU", &Tokenizer::BuildEncodes_GPU)
        .def("BuildEncodes", &Tokenizer::BuildEncodes);

    py::class_<BINFile>(handle, "BINFile")
        .def(py::init<std::string, int, int, int, int>())
        .def(py::init<std::string, int, int, int, int, int>())
        .def_readonly("NumberOfSamples", &BINFile::NumberOfSamplesInFile)
        .def_readonly("MaxNumberOfSamples", &BINFile::MaxNumberOfSamples)
        .def("get_slice", &BINFile::get_slice)
        .def("append", &BINFile::append)
        .def("__getitem__", static_cast<py::array_t<int>(BINFile::*)(uint64_t)>(&BINFile::operator[]));


#ifdef VERSION_INFO
    handle.attr("__version__") = VERSION_INFO;
#else
    handle.attr("__version__") = "dev";
#endif
}



