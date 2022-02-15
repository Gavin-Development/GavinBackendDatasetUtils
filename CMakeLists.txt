cmake_minimum_required(VERSION 3.21.0)
project(GavinBackendDatasetUtils)

set(CMAKE_CXX_STANDARD 17)
set(PYBIND11_CPP_STANDARD -std=c++17)
if (UNIX)
    set(CMAKE_C_COMPILER /opt/intel/oneapi/compiler/latest/linux/bin/icx)
    set(CMAKE_CXX_COMPILER /opt/intel/oneapi/compiler/latest/linux/bin/icpx)
    set(CMAKE_PREFIX_PATH /opt/intel/oneapi/compiler/latest/linux/IntelDPCPP/)
    set(SYCL_INCLUDE_DIR /opt/intel/oneapi/compiler/latest/linux/include/)
    set(SYCL_LIBRARY_DIR /opt/intel/oneapi/compiler/latest/linux/lib/)
elseif(WIN32)
    set(CMAKE_C_COMPILER icx)
    set(CMAKE_CXX_COMPILER icx)
endif()

find_package(Python 3.8 EXACT REQUIRED COMPONENTS Interpreter Development)
find_package(IntelDPCPP REQUIRED)

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

pybind11_add_module(${PROJECT_NAME} MODULE NO_EXTRAS GavinBackendDatasetUtils/PythonFuncsDefinitions.cpp GavinBackendDatasetUtils/DataSetConversions.cpp
        GavinBackendDatasetUtils/DataLoaderSingleThread.cpp GavinBackendDatasetUtils/DataLoaderMultiThread.cpp
        GavinBackendDatasetUtils/DataLoaderLegacySupport.cpp GavinBackendDatasetUtils/DataLoader.hpp GavinBackendDatasetUtils/base64.hpp)
target_link_libraries(${PROJECT_NAME} PRIVATE sycl)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE /opt/intel/oneapi/compiler/2022.0.2/linux/include)
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".pyd")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test.py ${CMAKE_CURRENT_BINARY_DIR}/test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Test.BIN ${CMAKE_CURRENT_BINARY_DIR}/Test.BIN COPYONLY)