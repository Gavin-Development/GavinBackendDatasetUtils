#include "DataLoader.hpp"

// default constructor / destructor stuff.

BINFile::BINFile(std::string dataPath, int startToken, int endToken, int sampleLength, int paddingVal) : startToken(startToken), endToken(endToken), sampleLength(sampleLength), paddingVal(paddingVal) {
#ifdef _DEBUG
	std::cout << "Initialising BIN file handler, Default Constructor." << std::endl;
#endif // _DEBUG


	// check for existing BIN file.
	_File = std::fstream(dataPath, std::ios::in | std::ios::ate | std::ios::out | std::ios::binary);

	// try to open existing file.
	if (!_File.is_open()) {
		std::cout << "No BIN File Present." << std::endl;

		throw std::runtime_error("Incorrect constructor used or unable to open BIN file.");
	}
	// If there is an existing file.
	else {
		std::cout << "Opened Existing BIN File." << std::endl;

		// Get File Length.
		_FileLength = _File.tellg();

		// Get Header Section Length.
		_File.seekg(0);
		_File.read((char*)&_HeaderSectionLength, sizeof(uint64_t));

		// Get Number Of Samples In The File.
		_File.seekg(8);
		_File.read((char*)&NumberOfSamplesInFile, sizeof(uint64_t));

		// Determine the maximum number of possible samples in the file.
		_MaxNumSamples  = (_HeaderSectionLength - 8) / sizeof(BIN::SampleHeaderData);
		MaxNumberOfSamples = _MaxNumSamples;

		// Determine The Position Of The Start Of The Data Section Length.
		_DataSectionPosition = 8 + _HeaderSectionLength;

		// Setup and load up the file header data.
		_pSampleHeaderData = (BIN::SampleHeaderData*)malloc(_HeaderSectionLength - 8);
		_File.seekg(16);
		_File.read((char*)_pSampleHeaderData, _HeaderSectionLength);

		// Pre Allocate The Temporary Read Buffers For Performance Reasons.
		// This Is Only Done For The Single Threaded Loader.
		_pBuffer_int32 = (int*)malloc(sizeof(int) * sampleLength);
		_pBuffer_int24 = (uint24_t*)malloc(sizeof(uint24_t) * (sampleLength - 2));
		_pBuffer_int16 = (uint16_t*)malloc(sizeof(uint16_t) * (sampleLength - 2));
	}
};

BINFile::BINFile(std::string dataPath, uint64_t numberOfSamples, int startToken, int endToken, int sampleLength, int paddingVal) : _MaxNumSamples(numberOfSamples), startToken(startToken), endToken(endToken), sampleLength(sampleLength), paddingVal(paddingVal) {
#ifdef _DEBUG
	std::cout << "New File Constructor called." << std::endl;
#endif // _DEBUG

	// Setup the file in memory
	_HeaderSectionLength = _MaxNumSamples * sizeof(BIN::SampleHeaderData);
	_DataSectionPosition = _HeaderSectionLength + 8;
	NumberOfSamplesInFile = 0;

	_pSampleHeaderData = (BIN::SampleHeaderData*)malloc(sizeof(BIN::SampleHeaderData) * _MaxNumSamples);

	// Writing in placeholder header information.

	// Set the values for each of the sample header data structs stored in RAM.
	for (size_t i = 0; i < _MaxNumSamples; i++) {
		_pSampleHeaderData[i] = { (uint64_t)0,(uint16_t)0,(uint8_t)0 };
	}

	if (_createnewfile(dataPath) != true) { std::cout << "Failed To Create New BIN File." << std::endl; throw std::runtime_error("Failed To Create New BIN File."); }

	// Open the file with read and write and move to the end of it.
	_File = std::fstream(dataPath, std::ios::ate | std::ios::binary | std::ios::out | std::ios::in);
	
	// Get the file length.
	_FileLength = _File.tellg();
};

BINFile::~BINFile() {
#ifdef _DEBUG
	std::cout << "Called BINFile Destructor." << std::endl;
#endif // _DEBUG

	// Write data back to the file.


	// TO DO.

	// Close the file once done.
	if (_File.is_open()) _File.close();
}


// Create New File Function Here

bool BINFile::_createnewfile(std::string& pDataPath) {
#ifdef _DEBUG
	std::cout << "Creating New BIN File." << std::endl;
#endif

	_File = std::fstream(pDataPath, std::ios::out | std::ios::binary);

	if (!_File.is_open()) {
		std::cout << "Failed To Open New File." << std::endl;
		return false;
	}

#ifdef _DEBUG
	std::cout << "File has been created." << std::endl;
#endif // _DEBUG

	// seek to beginning of the file.
	_File.seekg(0);

	// Writing Placeholder Header Section Length And Number Of Samples Values To The File.
	_File.write((const char*)&_HeaderSectionLength, sizeof(uint64_t));

	_File.seekg(8);

	_File.write((const char*)&NumberOfSamplesInFile, sizeof(uint64_t));

	// Write the whole header sample data array to the file.
	_File.seekg(16);
	_File.write((const char*)_pSampleHeaderData, _MaxNumSamples * sizeof(BIN::SampleHeaderData));

	// close the file as we opened in append mode.
	_File.close();

#ifdef _DEBUG
	std::cout << "Placeholder Header Information Written To The File." << std::endl;
#endif // _DEBUG
	return true;
}


// Read and write sample functions (Inline for obvious reasons).

inline int* BINFile::_readsample(uint64_t Index) {
#ifdef _DEBUG
	std::cout << "Private Method To Read Sample Called." << std::endl;
#endif // _DEBUG

	BIN::SampleHeaderData* pSampleData = &_pSampleHeaderData[Index];

	// clense the int32 buffer.
	_pBuffer_int32[0] = startToken;
	for (uint64_t i = 1; i < sampleLength; i++) _pBuffer_int32[i] = paddingVal;

	// Seek to sample position.
	_File.seekg(_DataSectionPosition + pSampleData->OffsetFromDataSectionStart);

	// If the sample is of type Int32.
	if (pSampleData->dtypeint16 == BIN_FILE_DTYPE_INT32) {
#ifdef _DEBUG
		std::cout << "Sample is of type int32." << std::endl;
#endif // _DEBUG

		// If the sample is longer than the user requested sample size.
		if (pSampleData->SampleLength >= (sampleLength - 2) * 4) {
#ifdef _DEBUG
			std::cout << "Sample is longer than sample buffer." << std::endl;
#endif // _DEBUG
			// Read in the sample.
			_File.read((char*)&_pBuffer_int32[1], sizeof(int) * (sampleLength - 2));

			// Set the end token.
			_pBuffer_int32[sampleLength - 1] = endToken;

		}

		// If the length of the sample is less than the user requested sample size.
		if (pSampleData->SampleLength < (sampleLength - 2) * 4) {
#ifdef _DEBUG
			std::cout << "Sample is shorter than sample buffer." << std::endl;
#endif // _DEBUG
			// Read in the sample.
			_File.read((char*)&_pBuffer_int32[1], pSampleData->SampleLength);

			// set the end token
			_pBuffer_int32[(pSampleData->SampleLength / 4) + 1] = endToken;


		}
	}

	// If the sample is of type Int24
	if (pSampleData->dtypeint16 == BIN_FILE_DTYPE_INT24) {
#ifdef _DEBUG
		std::cout << "Sample is of type int24." << std::endl;
#endif // _DEBUG
		// cleanse the Int24 buffer.
		for (uint64_t i = 0; i < sampleLength - 2; i++) _pBuffer_int24[i] = paddingVal;

		// If the sample is longer than the user requested sample size.
		if (pSampleData->SampleLength >= (sampleLength - 2) * 3) {
#ifdef _DEBUG
			std::cout << "Sample is longer than sample buffer." << std::endl;
#endif // _DEBUG
			// Read in the sample.
			_File.read((char*)_pBuffer_int24, sizeof(uint24_t)* (sampleLength - 2));

			// Cast it and move it to Int32 Buffer.
			for (uint64_t i = 0; i < sampleLength - 2; i++) {
				_pBuffer_int32[i + 1] = static_cast<int>(_pBuffer_int24[i]);
			}

			// Set end token.
			_pBuffer_int32[sampleLength - 1] = endToken;
		}

		// If the sample is not longer than the user requested samnple size.
		if (pSampleData->SampleLength < (sampleLength - 2) * 3) {
#ifdef _DEBUG
			std::cout << "Sample is shorter than sample buffer." << std::endl;
#endif // _DEBUG
			// Read in the sample.
			_File.read((char*)_pBuffer_int24, pSampleData->SampleLength);

			// Cast it and move it to Int32 Buffer.
			for (uint64_t i = 0; i < sampleLength - 2; i++) {
				_pBuffer_int32[i + 1] = static_cast<int>(_pBuffer_int24[i]);
			}

			// set end token.
			_pBuffer_int32[(pSampleData->SampleLength / 3) + 1] = endToken;
		}
	}

	// If the sample is of type Int16
	if (pSampleData->dtypeint16 == BIN_FILE_DTYPE_INT16) {
#ifdef _DEBUG
		std::cout << "Sample is of type int16." << std::endl;
#endif // _DEBUG
		// cleanse the Int16 Buffer.
		for (uint64_t i = 0; i < sampleLength - 2; i++) _pBuffer_int16[i] = paddingVal;

		// If the sample is longer than the user requested sample size.
		if (pSampleData->SampleLength >= (sampleLength - 2) * 2) {
#ifdef _DEBUG
			std::cout << "Sample is longer than sample buffer." << std::endl;
#endif // _DEBUG
			// Read in the sample.
			_File.read((char*)_pBuffer_int16, sizeof(uint16_t) * (sampleLength - 2));

			// Cast it and move it to Int32 Buffer.
			for (uint64_t i = 0; i < sampleLength - 2; i++) {
				_pBuffer_int32[i + 1] = static_cast<int>(_pBuffer_int16[i]);
			}

			// Set end token.
			_pBuffer_int32[sampleLength - 1] = endToken;
		}

		// If the sample is not longer than the user requested sample size.
		if (pSampleData->SampleLength < (sampleLength - 2) * 2) {
#ifdef _DEBUG
			std::cout << "Sample is shorter than sample buffer." << std::endl;
#endif // _DEBUG
			// Read in the sample.
			_File.read((char*)_pBuffer_int16, pSampleData->SampleLength);

			// Cast it and move it to Int32 Buffer.
			for (uint64_t i = 0; i < sampleLength - 2; i++) {
				_pBuffer_int32[i + 1] = static_cast<int>(_pBuffer_int16[i]);
			}

			// set end token.
			_pBuffer_int32[(pSampleData->SampleLength / 2) + 1] = endToken;
		}
	}

#ifdef _DEBUG
	std::cout << "Sample load complete." << std::endl;
#endif // _DEBUG

	// Allocate and copy read data to a more permanent buffer.
	int* memory = (int*)malloc(sizeof(int) * sampleLength);
	memcpy(memory, _pBuffer_int32, sizeof(int) * sampleLength);

	return memory;
};

inline void BINFile::_writesample(uint64_t Index, py::array_t<int> Arr) {
#ifdef _DEBUG
	std::cout << "Writing Sample To File." << std::endl;
#endif // _DEBUG

	bool int16compatible = true, int24compatible = true;

	std::vector<uint16_t> Int16Vec;
	std::vector<uint24_t> Int24Vec;

	// check if the sample can be expressed as a mixed or lower precision format in the file.
	for (size_t i = 0; i < Arr.size(); i++) {
		if (Arr.at(i) > UINT16_MAX) {
			int16compatible = false;
		}

		if (Arr.at(i) > UINT24_MAX) {
			int24compatible = false;
		}
	}

	// If Int16 compatible.
	if (int16compatible) {

		// Cast the values to Int16 vector.
		for (size_t i = 0; i < Arr.size(); i++) {
			Int16Vec.push_back(static_cast<uint16_t>(Arr.at(i)));
		}

		// Generate Header Info.

		// If this is the first sample.
		if (Index == 0) {
			_pSampleHeaderData[Index] = {
				(uint64_t)0,
				(uint16_t)(Int16Vec.size() * sizeof(uint16_t)),
				BIN_FILE_DTYPE_INT16
			};
		}
		// If not first sample
		else {
			_pSampleHeaderData[Index] = {
				(uint64_t)(_pSampleHeaderData[Index - 1].OffsetFromDataSectionStart + _pSampleHeaderData[Index - 1].SampleLength),
				(uint16_t)(Int16Vec.size() * sizeof(uint16_t)),
				BIN_FILE_DTYPE_INT16
			};
		}

		// Seek to the samples header frame position and write the header to the file.
		_File.seekg((sizeof(BIN::SampleHeaderData) * Index) + 16);
		_File.write((const char*)&_pSampleHeaderData[Index], sizeof(BIN::SampleHeaderData));

		// Seek to the data position and write the data to disk.
		_File.seekg(_pSampleHeaderData[Index].OffsetFromDataSectionStart + _DataSectionPosition);
		_File.write((const char*)Int16Vec.data(), sizeof(uint16_t) * Int16Vec.size());
	}
	// If not Int 16 but is Int24 compatible.
	if (!int16compatible && int24compatible) {

		// Cast the calues to Int24 vector.
		for (size_t i = 0; i < Arr.size(); i++) {
			Int24Vec.push_back(static_cast<uint24_t>(Arr.at(i)));
		}

		// Generate Header Info.

		// If this is the first sample.
		if (Index == 0) {
			_pSampleHeaderData[Index] = {
				(uint64_t)0,
				(uint16_t)(Int24Vec.size() * sizeof(uint24_t)),
				BIN_FILE_DTYPE_INT24
			};
		}
		// If not first sample
		else {
			_pSampleHeaderData[Index] = {
				(uint64_t)(_pSampleHeaderData[Index - 1].OffsetFromDataSectionStart + _pSampleHeaderData[Index - 1].SampleLength),
				(uint16_t)(sizeof(uint24_t) * Int24Vec.size()),
				BIN_FILE_DTYPE_INT24
			};
		}

		// Seek to the samples header frame position and write the header to the file.
		_File.seekg((sizeof(BIN::SampleHeaderData) * Index) + 16);
		_File.write((const char*)&_pSampleHeaderData[Index], sizeof(BIN::SampleHeaderData));

		// Seek to the data position and write the data to disk.
		_File.seekg(_pSampleHeaderData[Index].OffsetFromDataSectionStart + _DataSectionPosition);
		_File.write((const char*)Int24Vec.data(), sizeof(uint24_t) * Int24Vec.size());
	}

	// If only Int32 compatible.
	if (!int16compatible && !int24compatible) {

		// If this is the first sample.
		if (Index == 0) {
			_pSampleHeaderData[Index] = {
				(uint64_t)0,
				(uint16_t)(Arr.size() * sizeof(int)),
				BIN_FILE_DTYPE_INT32
			};
		}
		// If not first sample
		else {
			_pSampleHeaderData[Index] = {
				(uint64_t)(_pSampleHeaderData[Index - 1].OffsetFromDataSectionStart + _pSampleHeaderData[Index - 1].SampleLength),
				(uint16_t)(sizeof(int) * Arr.size()),
				BIN_FILE_DTYPE_INT32
			};
		}

		// Seek to the samples header frame position and write the header to the file.
		_File.seekg((sizeof(BIN::SampleHeaderData) * Index) + 16);
		_File.write((const char*)&_pSampleHeaderData[Index], sizeof(BIN::SampleHeaderData));

		// Seek to the sample data position and write the data to the disk.
		_File.seekg(_pSampleHeaderData[Index].OffsetFromDataSectionStart + _DataSectionPosition);
		_File.write((const char*)Arr.data(), sizeof(int) * Arr.size());
	}

#ifdef _DEBUG
	std::cout << "Sample Written To File." << std::endl;
#endif // _DEBUG

}


// Operator Overloads & User accessable data manipulation methods.

py::array_t<int> BINFile::operator[](uint64_t Index) {
#ifdef _DEBUG
	std::cout << "Single variable Array indexing syntax overload." << std::endl;
#endif // _DEBUG

	// check if the requested sample is in range of the file size.
	if (Index >= NumberOfSamplesInFile) {
		std::cout << "Array index out of bounds for this file." << std::endl;
		throw std::runtime_error("Array Index Out Of Bounds.");
	}
	else {

		int* memory = _readsample(Index);
		// Handing off resource handling / deallocation to python interpreter (WE DO NOT NEED TO TRACK py::capsule)
		py::capsule capsule(memory, [](void* memory) { delete reinterpret_cast<int*>(memory); });

		// Creating rtn array which will hold the data + have a capsule to handle memory dealloc.
		py::array_t<int> RtnArr(
			{ sampleLength },
			{ 4 },
			memory,
			capsule
		);
		return RtnArr;
	}

}

py::array_t<int> BINFile::get_slice(uint64_t StartIndex, uint64_t EndIndex) {
#ifdef _DEBUG
	std::cout << "User has called get_slice method to get a slice of the data." << std::endl;
#endif // _DEBUG

	// check if the requested sample is in range of the file size.
	for (uint64_t i = StartIndex; i < EndIndex; i++) {
		if (i >= NumberOfSamplesInFile) {
			std::cout << "Array index out of bounds for this file." << std::endl;
			throw std::runtime_error("Array Index Out Of Bounds.");
		}
	}

	// allocate memory for the samples.
	int* memory = (int*)malloc(sizeof(int) * sampleLength * (EndIndex - StartIndex));

	// read the samplesand store them in an array.
	for (uint64_t i = StartIndex; i < EndIndex; i++) {
		memcpy(&memory[(i - StartIndex) * sampleLength], _readsample(i), sizeof(int) * sampleLength);
	}

	py::capsule capsule(memory, [](void* memory) { delete reinterpret_cast<int*>(memory); });

	py::array_t<int> RtnArr(
		{ (uint64_t)(EndIndex - StartIndex), (uint64_t)sampleLength },
		{ sampleLength * 4, 4 },
		memory,
		capsule
	);

	return RtnArr;
};

bool BINFile::append(py::array_t<int> Data) {
#ifdef _DEBUG
	std::cout << "Append Data To File Function Called." << std::endl;
#endif // _DEBUG

	// Check if the append op will take us out of range of the file size.
	if (NumberOfSamplesInFile >= _MaxNumSamples) {
		std::cout << "Unable to Append more data as file limit has been reached." << std::endl;
		return false;
	}
	// If the append is deemed to be valid and within file size than it can happen.
	else {
		_writesample(NumberOfSamplesInFile, Data);

		// Seek to the Numberofsamplesinfile data position and incriment by 1.
		NumberOfSamplesInFile++;
		_File.seekg(8);
		_File.write((const char*)&NumberOfSamplesInFile, sizeof(uint64_t));

#ifdef _DEBUG
		std::cout << "Sample Written To File And NumberOfSamplesInFile Value Iterated." << std::endl;
#endif // _DEBUG
		return true;
	}

}