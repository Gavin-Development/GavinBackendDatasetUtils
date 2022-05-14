cmake_minimum_required(VERSION 3.21.0)
project(GavinBackendDatasetUtils)

set(CMAKE_CXX_STANDARD 17)
set(PYBIND11_CPP_STANDARD -std=c++17)
message("ONEAPI_ROOT Set at: ${ONEAPI_ROOT}")
if(DEFINED ENV{ONEAPI_ROOT})
    if (UNIX)
        set(CMAKE_C_COMPILER "${ONEAPI_ROOT}/compiler/latest/linux/bin/icx")
        set(CMAKE_CXX_COMPILER "${ONEAPI_ROOT}/compiler/latest/linux/bin/icpx")
        set(CMAKE_PREFIX_PATH "${ONEAPI_ROOT}/compiler/latest/linux/IntelDPCPP/")
        set(SYCL_INCLUDE_DIR "${ONEAPI_ROOT}/compiler/latest/linux/include/")
        set(SYCL_LIBRARY_DIR "${ONEAPI_ROOT}/compiler/latest/linux/lib/")
    elseif(WIN32)
        set(CMAKE_C_COMPILER "${ONEAPI_ROOT}\\compiler\\latest\\windows\\bin\\icx")
        set(CMAKE_CXX_COMPILER "${ONEAPI_ROOT}\\compiler\\latest\\windows\\bin\\icx")
        set(CMAKE_PREFIX_PATH "${ONEAPI_ROOT}\\compiler\\latest\\windows\\IntelDPCPP\\")
        set(SYCL_INCLUDE_DIR "${ONEAPI_ROOT}\\compiler\\latest\\windows\\include\\")
        set(SYCL_LIBRARY_DIR "${ONEAPI_ROOT}\\compiler\\latest\\windows\\lib\\")
    endif()
find_package(IntelDPCPP REQUIRED)
endif()

find_package(Python 3.8 EXACT REQUIRED COMPONENTS Interpreter Development NumPy)

configure_file(CMakeLists.txt.in pybind11-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pybind11-download)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/pybind11-download)

configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)

configure_file(CMakeLists.txt.in SubwordTextEncoder-download/CMakeLists.txt)


# Add pybind11 directly
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/pybind11-src
        ${CMAKE_CURRENT_BINARY_DIR}/pybind11-build
        EXCLUDE_FROM_ALL)

# Add googletest directly
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
        ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
        EXCLUDE_FROM_ALL)

file(GLOB SRC_FILES GavinBackendDatasetUtils/*.cpp GavinBackendDatasetUtils/*.hpp)
message(SRC_FILES: ${SRC_FILES})
pybind11_add_module(${PROJECT_NAME} MODULE ${SRC_FILES})
if(DEFINED ENV{ONEAPI_ROOT})
    target_link_libraries(${PROJECT_NAME} PRIVATE sycl)
    if (UNIX)
        target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${ONEAPI_ROOT}/compiler/latest/linux/include")
    else(WIN32)
        target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE "${ONEAPI_ROOT}\\compiler\\latest\\windows\\include\\")
    endif()
endif()
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".pyd")
elseif(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".so")
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test.py ${CMAKE_CURRENT_BINARY_DIR}/test.py COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Test.BIN ${CMAKE_CURRENT_BINARY_DIR}/Test.BIN COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test-lines.txt ${CMAKE_CURRENT_BINARY_DIR}/test-lines.txt COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/small.to ${CMAKE_CURRENT_BINARY_DIR}/small.to COPYONLY)

enable_testing()
include_directories("${CMAKE_CURRENT_BINARY_DIR}/googletest-src/googletest/include/")
include_directories("${CMAKE_CURRENT_BINARY_DIR}/googletest-src/googlemock/include/")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/GavinBackendDatasetUtils/")
include_directories("${Python_INCLUDE_DIRS}")

add_executable(test_${PROJECT_NAME} test/unittests.cpp ${SRC_FILES})
target_link_libraries(test_${PROJECT_NAME}
        ${EXTRA_LIBS}
        ${PYTHON_LIBRARIES}
        pybind11::embed
        ${CMAKE_DL_LIBS}
        gtest
        gtest_main)
