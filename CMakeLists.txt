cmake_minimum_required(VERSION 3.21.0)
project(GavinBackendDatasetUtils)
find_package(IntelDPCPP REQUIRED)


include_directories($ENV{DPCPP_HOME}/llvm/build/include)
include_directories($ENV{DPCPP_HOME}/llvm/build/include/sycl/)
include_directories($ENV{DPCPP_HOME}/llvm/build/lib)

add_compile_options(-fsycl -fsycl-unnamed-lambda)
add_link_options(-fsycl -fsycl-unnamed-lambda)

find_package(Python 3.9 EXACT REQUIRED COMPONENTS Interpreter Development)

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

file(GLOB SRC_FILES GavinBackendDatasetUtils/*.cpp GavinBackendDatasetUtils/*.hpp)
message(SRC_FILES: ${SRC_FILES})
pybind11_add_module(${PROJECT_NAME} MODULE ${SRC_FILES})
if(DEFINED ENV{ONEAPI_ROOT})
    target_link_libraries(${PROJECT_NAME} PRIVATE sycl)
    if (UNIX)
        target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${ONEAPI_ROOT}/compiler/latest/linux/include)
    else(WIN32)
        target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${ONEAPI_ROOT}\\compiler\\latest\\windows\\include\\)
    endif()
endif()
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".pyd")
elseif(UNIX)
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".so")
endif()

# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test.py ${CMAKE_CURRENT_BINARY_DIR}/test.py COPYONLY)
# configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Test.BIN ${CMAKE_CURRENT_BINARY_DIR}/Test.BIN COPYONLY)
