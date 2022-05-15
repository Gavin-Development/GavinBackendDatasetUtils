#include "DataLoader.hpp"

// default constructor / destructor stuff.

BINFile::BINFile(std::string dataPath, int startToken, int endToken, int sampleLength, int paddingVal) : startToken(startToken), endToken(endToken), sampleLength(sampleLength), paddingVal(paddingVal) {
#ifdef _DEBUG
	std::cout << "Initialising BIN file handler." << std::endl;
#endif // _DEBUG


	// check for existing BIN file.
	_File = std::fstream(dataPath, std::ios::in | std::ios::ate | std::ios::out | std::ios::binary);

	// try to open existing file.
	if (!_File.is_open()) {
		std::cout << "No BIN File Present, Creating A New One." << std::endl;

		_File.close();
		_File = std::fstream(dataPath, std::ios::out | std::ios::binary | std::ios::app);

#ifdef _DEBUG
		std::cout << "Opened New BIN File." << std::endl;
#endif // _DEBUG

		// Writing Placeholder Header Section Length And Number Of Samples Values To The File.
		for (size_t i = 0; i < 16; i++) {
			_File << (char)0;
		}

#ifdef _DEBUG
		std::cout << "Written Metadata To New BIN File." << std::endl;
#endif // _DEBUG

		// Close And Re Open The File For General Use.
		_File.close();
		_File = std::fstream(dataPath, std::ios::in | std::ios::ate | std::ios::out | std::ios::binary);

		// If The File Is Not Open Still Then We CBA To Handle It So We Crash :)
		if (!_File.is_open()) {
			std::cout << "Error, Unabel To Open File, Can Not Handle For This Issue." << std::endl;
			return;
		}
#ifdef _DEBUG
		std::cout << "New BIN File Created." << std::endl;
#endif // _DEBUG
	}
	// If there is an existing file.
	else {
		std::cout << "Opened Existing BIN File." << std::endl;

		// Get File Length.
		_File.seekg(std::ios::ate);
		_FileLength = _File.tellg();

		// Get Header Section Length.
		_File.seekg(0);
		_File.read((char*)&_HeaderSectionLength, sizeof(uint64_t));

		// Get Number Of Samples In The File.
		_File.seekg(8);
		_File.read((char*)&_NumberOfSamplesInFile, sizeof(uint64_t));

		// Determine The Position Of The Start Of The Data Section Length.
		_DataSectionPosition = 16 + _HeaderSectionLength;

		// Setup Python capsule and py array for user interaction.

		//DataCapsuel = py::capsule(int*, [](void* ))
	}

	// Pre Allocate The Temporary Read Buffers For Performance Reasons.
	// This Is Only Done For The Single Threaded Loader.
	_pBuffer_int32 = (int*)malloc(sizeof(int) * sampleLength);
	_pBuffer_int24 = (uint24_t*)malloc(sizeof(uint24_t) * sampleLength);
	_pBuffer_int16 = (uint16_t*)malloc(sizeof(uint16_t) * sampleLength);

	// Allocate and load up the header information for this file.

	_pSampleHeaderData = (BIN::SampleHeaderData*)malloc(_HeaderSectionLength);

	_File.seekg(16);
	_File.read((char*)_pSampleHeaderData, _HeaderSectionLength);

	// Some Test Code.
	for (int i = 0; i < 10; i++) {
		std::cout << _pSampleHeaderData[i].dtypeint16 << std::endl << _pSampleHeaderData[i].OffsetFromDataSectionStart << std::endl << _pSampleHeaderData[i].SampleLength << std::endl << std::endl;
	}

	// Calling File close to test.
	_File.close();
};

BINFile::~BINFile() {
#ifdef _DEBUG
	std::cout << "Called BINFile Destructor." << std::endl;
#endif // _DEBUG
	if (_File.is_open()) _File.close();
}


// Read and write sample functions (Inline for obvious reasons).

inline py::array_t<int> BINFile::_readsample(uint64_t Index) {
#ifdef _DEBUG
	std::cout << "Private Method To Read Sample Called." << std::endl;
#endif // _DEBUG

	int* memory = (int*)malloc(sizeof(int) * 10);
	py::capsule capsule(memory, [](void* memory) { delete reinterpret_cast<int*>(memory); });

	py::array_t<int> RtnArr(
		{ (uint64_t)sampleLength, (uint64_t)1 },
		memory,
		capsule
		);
	return RtnArr;
};

// Operator Overloads.

py::array_t<int> BINFile::operator[](uint64_t Index) {
#ifdef _DEBUG
	std::cout << "Single variable Array indexing syntax overload." << std::endl;
#endif // _DEBUG

	if (Index >= _NumberOfSamplesInFile) {
		std::cout << "Array index out of bounds for this file." << std::endl;
		throw std::runtime_error("Array Index Out Of Bounds.");
	}
	else return _readsample(Index);

}