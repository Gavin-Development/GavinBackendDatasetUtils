#include "DataLoader.hpp"

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cuda.h>
#include <device_functions.h>


__global__ void CudaKernel(char* FileData, int64_t* NumberOfLinesArr, int64_t* NumberOfLinesIO, size_t n) {
	int Tid = threadIdx.x;
	if (Tid < n) {
		if (FileData[Tid] == (char)0x0A) {
			NumberOfLinesArr[Tid] = 1;
		}
		else NumberOfLinesArr[Tid] = 0;

		for (int stride = 1; stride < n; stride * 2) {
			if (Tid + stride < n / stride) {
				NumberOfLinesArr[Tid] += NumberOfLinesArr[Tid + stride];
			}
		}

		if (Tid == 0) {
			*NumberOfLinesIO = NumberOfLinesArr[0];
		}
	}
};

void LaunchCudaKernels(char* FileData, size_t FileLength) {
	int64_t* NumberOfLines;
	int64_t* Device_NumberOfLines;
	char* Device_FileContentsBuffer;
	int64_t* Device_NumberOfLinesArr;
	cudaMallocManaged(&Device_FileContentsBuffer, sizeof(char) * FileLength);
	cudaMallocManaged(&Device_NumberOfLinesArr, sizeof(int64_t) * FileLength);
	cudaMallocManaged(&Device_NumberOfLines, sizeof(int64_t));
	NumberOfLines = (int64_t*)malloc(sizeof(int64_t) * FileLength);

	cudaMemcpy(Device_FileContentsBuffer, FileData, sizeof(char) * FileLength, cudaMemcpyHostToDevice);

	CudaKernel <<<1, FileLength>>> (Device_FileContentsBuffer, Device_NumberOfLinesArr, Device_NumberOfLines, FileLength);
	cudaDeviceSynchronize();

	cudaMemcpy(NumberOfLines, Device_NumberOfLinesArr, sizeof(int64_t) * FileLength, cudaMemcpyDeviceToHost);
	
	for (int64_t i = 0; i < FileLength; i++) {
		if (NumberOfLines[i] == 1) {
			std::cout << "NewLine." << std::endl;
		}
	}
	cudaFree(Device_FileContentsBuffer);
	cudaFree(Device_NumberOfLinesArr);
	cudaFree(Device_NumberOfLines);
	std::cout << *NumberOfLines << std::endl;
	std::cout << FileLength << std::endl;
	char tmp[3] = { FileData[1], FileData[1 + 1], 0 };
	std::cout << tmp << std::endl;
	std::cout << "Cuda Kernels Complete." << std::endl;
};

std::vector<py::list> LoadTrainDataGPU_Accelerated_Future(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue) {
	std::cout << "WARNING THI FUNCTION MIGHT NOT WORK AS INTENDED DO NOT USE PROPERLY." << std::endl;
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
	int64_t ProgressReportInterval = MaxSamples / 100;

	std::cout << "Loading " << samplesToRead << " Samples From " << FileName << std::endl;

	File = std::ifstream(FileName, std::ios::binary | std::ios::ate);
	if (File.is_open()) {
		std::cout << "Loading (Up To) " << MaxSamples << " Samples." << std::endl;
	}
	else {
		std::cout << "Failed To Open File." << std::endl;
		return FileData;
	}
	char* FileContentsBuffer;
	size_t FileLength = File.tellg();
	File.seekg(0);
	FileContentsBuffer = (char*)malloc(sizeof(char) * FileLength);
	File.read(FileContentsBuffer, FileLength);

	LaunchCudaKernels(FileContentsBuffer, FileLength);

	for (int64_t i = 0; i < 1000; i++) {
		const char tmp[2] = { FileContentsBuffer[i], 0 };
		std::cout << tmp;
		if (FileContentsBuffer[i] == (char)0x0A) {
			std::cout << "Found A New Line Char." << std::endl;
		}
	}
	std::cout << std::endl;

	for (int i = 0; i < 1000; i++) {
		std::cout << FileContentsBuffer[i];
	}
	std::cout << std::endl;

	File.close();
	std::cout << "Samples Have Been Read." << std::endl;
	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	TimeTaken = (EndTime - StartTime) / 1000000000;
	std::cout << "Time Taken: " << TimeTaken << " Seconds." << std::endl;
	return FileData;
}