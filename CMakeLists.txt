cmake_minimum_required(VERSION 3.17)
project(GavinBackendDatasetUtils)

set(CMAKE_CXX_STANDARD 17)

find_package(Python 3.8 EXACT REQUIRED COMPONENTS Interpreter Development)

configure_file(CMakeLists.txt.in pybind11-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pybind11-download)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pybind11-download)

# Add pybind11 directly
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/pybind11-src
        ${CMAKE_CURRENT_BINARY_DIR}/pybind11-build
        EXCLUDE_FROM_ALL)

pybind11_add_module(${PROJECT_NAME} GavinBackendDatasetUtils/PythonFuncsDefinitions.cpp GavinBackendDatasetUtils/DataSetConversions.cpp
        GavinBackendDatasetUtils/DataLoaderSingleThread.cpp GavinBackendDatasetUtils/DataLoaderMultiThread.cpp
        GavinBackendDatasetUtils/DataLoaderLegacySupport.cpp GavinBackendDatasetUtils/DataLoader.hpp GavinBackendDatasetUtils/base64.hpp)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".pyd")

