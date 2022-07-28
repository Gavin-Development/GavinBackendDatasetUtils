#include "DataLoader.hpp"

// Multithreaded CPU bit
void _GPU_Encode_Builder_CPU_Thread(std::mutex Mutex,
	std::vector<char> *vSamplesChar,
	sycl::buffer<char> *bSamplesChar,
	sycl::buffer<int> *bEncodePresent,
	sycl::buffer<uint64_t> *bEncodeCommonality,
	std::vector<std::string> *vFoundEncodes,
	std::vector<uint64_t> *vFoundEncodesIndices,
	sycl::queue *q,
	uint64_t N_Range,
	uint64_t WGS,
	uint64_t N_Workgroups,
	uint64_t ThreadId,
	uint64_t TotalThreads) {

	// Determine the sections of vSamplesChar we are to iterate over on this CPU thread.
	uint64_t StartIndex = (vSamplesChar->size() / TotalThreads) * ThreadId;
	uint64_t EndIndex;
	if (ThreadId == TotalThreads) EndIndex = vSamplesChar->size() - 1;
	else EndIndex = StartIndex + (vSamplesChar->size() / TotalThreads);
	uint64_t vSamplesCharSize = vSamplesChar->size();

	for (uint64_t i = StartIndex; i < EndIndex; i++) {

		// Create the byte pair string.
		std::string BytePair{ vSamplesChar[0][i], vSamplesChar[0][i + 1] };

		// check if the encode has been found already.
		bool FoundAlready = false;
		for (auto& Encode : *vFoundEncodes) {
			if (Encode == BytePair) { FoundAlready = true; break; }
		}

		// If the proposed Encode is unique.
		if (!FoundAlready) {
			// Acquire the mutex. Loop untill acquired.
			bool Locked = false;
			while (!Locked) {
				Locked = Mutex.try_lock();
			}

			// Set the encode as found in the vectors.
			vFoundEncodes->push_back(BytePair);
			vFoundEncodesIndices->push_back(i);

			// Launch the GPU kernels.
			auto CommonalityKernel = q[0].submit([&](sycl::handler& h) {

				sycl::accessor aSamplesChar(*bSamplesChar, h, sycl::read_only);
				sycl::accessor aEncodePresent(*bEncodePresent, h, sycl::write_only);

				// Local memory array in each workgroup.
				sycl::accessor<int, 1, sycl::access::mode::read_write, sycl::access::target::local> aLocal_Mem(sycl::range<1>(WGS), h);

				h.parallel_for(sycl::nd_range<1>{N_Range, WGS}, [=](sycl::nd_item<1> TID) {
					auto wg = TID.get_group();
					auto x = TID.get_global_id(0);
					auto localmemaccessindex = TID.get_local_id(0);

					uint64_t SamplesCharEncodeIndex = i;

					// If in range of the string buffer.
					if (x < vSamplesCharSize - 1) {
						if (aSamplesChar[SamplesCharEncodeIndex] == aSamplesChar[x] && aSamplesChar[SamplesCharEncodeIndex + 1] == aSamplesChar[x + 1]) {
							aLocal_Mem[localmemaccessindex] = 1;
						}
						else aLocal_Mem[localmemaccessindex] = 0;
					}
					else aLocal_Mem[localmemaccessindex] = 0;

					// synchronise all the kernels.
					TID.barrier(sycl::access::fence_space::local_space);

					// do a partial sum reduction from local memory into the device global memory (VRAM on a GPU).
					int sum_wg = sycl::reduce_over_group(wg, aLocal_Mem[localmemaccessindex], sycl::plus<>());

					if (localmemaccessindex == 0) { aEncodePresent[TID.get_group_linear_id()] = sum_wg; }

					});
				});

			// Final stage of sum reduction.
			q[0].submit([&](sycl::handler& h) {
				// Depends on the previous kernel.
				h.depends_on(CommonalityKernel);

				sycl::accessor aEncodePresent(*bEncodePresent, h, sycl::read_write);
				sycl::accessor aEncodeCommonality(*bEncodeCommonality, h, sycl::write_only);

				h.single_task([=]() {
					uint64_t sum = 0;
					uint64_t CommonalityAccessIndex = i;
					for (int j = 0; j < N_Workgroups; j++) {
						sum += aEncodePresent[j];
					}
					aEncodeCommonality[CommonalityAccessIndex] = sum;
					});
				});

			// Now we release the Mutex.
			Mutex.unlock();
		}

	}

}


// Constructors.
Tokenizer::Tokenizer(std::string iTokenizerName) {
#ifdef _DEBUG
	std::cout << "Initialising a new tokenizer." << std::endl;
#endif // _DEBUG

	// Set up some Tokenizer variables.
	TokenizerName = iTokenizerName;

	Encodings.resize(0);
	Commonalities.resize(0);

#ifdef _DEBUG
	std::cout << "Initialisation Done." << std::endl;
#endif // _DEBUG

};

// Build Encodes Functions.
void Tokenizer::BuildEncodes(std::vector<std::string> Samples) {
#ifdef _DEBUG
	std::cout << "Building Tokenizer Encodes On CPU." << std::endl;
#endif // _DEBUG

	// Time keeping variables.
	int64_t StartTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	int64_t EndTime;
	int64_t TimeTaken;
	
	// Setup samples char array and add in EOL / NULL tokens at the end of each word.
	std::vector<char> vSamplesChar;
	uint64_t vSamplesCharSize = 0;
	for (uint64_t i = 0; i < Samples.size(); i++) {
		vSamplesCharSize = vSamplesChar.size();
		vSamplesChar.resize(vSamplesCharSize + Samples[i].size());
		memcpy(&vSamplesChar[vSamplesCharSize], Samples[i].c_str(), sizeof(char) * Samples[i].size());
		vSamplesChar.push_back((char)32);
	}

	// Set a value for the interval to report progress.
	vSamplesCharSize = vSamplesChar.size();
	uint64_t ProgressReportInterval = vSamplesCharSize / 100;

	// Setup a vector of encodes found during THIS tokenizer functions lifespan.
	// This is used if you are modifying an existing tokenizers encodes by adding data to them.
	std::vector<std::string> vFoundEncodes;

	// We iterate over EVERY unique byte pair in the sequence and determine the commonality of it.
	for (uint64_t i = 0; i < (vSamplesChar.size() - 1); i++) {
		// Setup the byte pair string to make syntax easier for comparisons.
		std::string BytePair;
		BytePair.resize(2);
		BytePair = { vSamplesChar[i], vSamplesChar[i + 1] };

		// Report progress.
		if (i % ProgressReportInterval == 0) {
			double PercentDone = (((double)i / vSamplesCharSize) * 100);
			std::cout << round(PercentDone) << " percent done." << std::endl;
		}

		// Check if the encode has already been found in this function.
		bool FoundAlready = false;
		for (auto& Encode : vFoundEncodes) {
			if (Encode == BytePair) { FoundAlready = true; }
		}
		 
		// If the encode has not already been found in this function.
		if (!FoundAlready) {

			// check if the encode exists or not. If it exists lets get its Index in the encode array
			bool ExistingEncode = false;
			uint64_t EncodeIndex;
			for (uint64_t j = 0; j < Encodings.size(); j++) {
				if (Encodings[j] == BytePair) { ExistingEncode = true; EncodeIndex = j; }
			}

			// Determine its commonality.
			uint64_t EncodeCommonality = 0;
			for (uint64_t j = 0; j < (vSamplesChar.size() - 1); j++) {
				if (BytePair[0] == vSamplesChar[j] && BytePair[1] == vSamplesChar[j + 1]) {
					EncodeCommonality++;
				}
			}

			// If the encode exists already.
			if (ExistingEncode) {
				Commonalities[EncodeIndex] += EncodeCommonality;
			}
			// If the encodes does not exist already.
			else {
				// Append the new encode to the encodings along with its commonality.
				Encodings.push_back(BytePair);
				Commonalities.push_back(EncodeCommonality);
			}

			// Finally mark the encode as being done in this function.
			vFoundEncodes.push_back(BytePair);
		}
	}

	// Lets see how long it took.
	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	TimeTaken = (EndTime - StartTime) / 1000000000;
	std::cout << "Time Taken: " << TimeTaken << " Seconds." << std::endl;

	// Sort The Encodings & Commonality Vectors.

	std::cout << "Sorting The Encodes." << std::endl;

	_SortEncodings();

	std::cout << "Sorting Of Encodes Done." << std::endl;

#ifdef _DEBUG
	std::cout << "Doone Building Tokenizer On CPU." << std::endl;
#endif // _DEBUG
};


// Could do with re design to make use of multiple GPUs in system.
void Tokenizer::BuildEncodes_GPU(std::vector<std::string> Samples) {
#ifdef _DEBUG
	std::cout << "Building Tokenizer Encodes On GPU." << std::endl;
#endif // _DEBUG

	// Time keeping variables.
	int64_t StartTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	int64_t EndTime;
	int64_t TimeTaken;

	// SYCL initialisation.
	sycl::default_selector d_selector;
	sycl::queue q(d_selector);

	// Alert the user to device selection & other stuff.
	std::cout << "Using device: " << q.get_device().get_info<sycl::info::device::name>() << std::endl;
	int MaxWorkGroupSize = q.get_device().get_info<sycl::info::device::max_work_group_size>();
	std::cout << "Device Max workgroup size: " << MaxWorkGroupSize << std::endl;
	uint64_t DeviceMemorySize = q.get_device().get_info<sycl::info::device::global_mem_size>();
	std::cout << "Device Local Memory size: " << DeviceMemorySize << std::endl;

	// Convert Samples to char array and add in EOL / NULL tokens at the end of each word.
	std::vector<char> vSamplesChar;
	uint64_t vSamplesCharSize = 0;
	for (uint64_t i = 0; i < Samples.size(); i++) {
		vSamplesCharSize = vSamplesChar.size();
		vSamplesChar.resize(vSamplesCharSize + Samples[i].size());
		memcpy(&vSamplesChar[vSamplesCharSize], Samples[i].c_str(), sizeof(char) * Samples[i].size());
		vSamplesChar.push_back((char)32);
	}
	sycl::buffer<char> bSamplesChar(vSamplesChar);

	// Now we figure out the values for ND range kernels.
	// WGS = Work Group Size (set to max size for selected device for good thread occupancy.
	// N_Range = The range of work units across all WGS in the kernel.
	vSamplesCharSize = vSamplesChar.size();
	uint64_t WGS = MaxWorkGroupSize;
	uint64_t N_Range = 0;
	uint64_t N_Workgroups = 0;
	while (N_Workgroups * WGS < vSamplesCharSize) {
		N_Workgroups++;
	}
	N_Range = N_Workgroups * WGS;
	std::cout << "Work Group Size: " << WGS << std::endl;
	std::cout << "Global Range: " << N_Range << std::endl;

	// Set a value for the interval to report progress.
	uint64_t ProgressReportInterval = vSamplesCharSize / 100;

	// Setup a vector of found Encodes that have been found during the lifetime of this function.
	std::vector<std::string> vFoundEncodes;
	std::vector<uint64_t> vFoundEncodesIndices;

	// Value set to 1 if proposed encode is present at that point in the samples buffer. Sum reduced to another buffer.
	sycl::buffer<int> bEncodePresent(sycl::range<1> {N_Workgroups});

	// Creating a buffer to store the reduced commonality values in. It is set to the max possible size it could ever be.
	sycl::buffer<uint64_t> bEncodeCommonality(sycl::range<1> {vSamplesCharSize - 1});

	// Check that the device has enough memory for what we want to do.
	if (bSamplesChar.byte_size() + bEncodePresent.byte_size() + bEncodeCommonality.byte_size() > DeviceMemorySize) {
		std::cout << "Breaching device memory limitations, reduce the size of the corpus passed to this function." << std::endl;
		return;
	}

#ifdef _DEBUG
	std::cout << "Finished prepping long lived GPU buffers." << std::endl;
#endif // _DEBUG

	std::cout << "Launching GPU kernels." << std::endl;

	bool FoundAlready;
	std::string BytePair;
	BytePair.resize(2);
	BytePair.reserve(2);
	// We iterate over EVERY Unique Byte pair in the sequence and determine the commonality of it.
	for (uint64_t i = 0; i < (vSamplesChar.size() - 1); i++) {

		// Report progress.
		if (i % ProgressReportInterval == 0) {
			double PercentDone = (((double)i / vSamplesCharSize) * 100);
			std::cout << round(PercentDone) << " percent done." << std::endl;
		}

		// creating a string for the byte pair.
		BytePair = { vSamplesChar[i], vSamplesChar[i + 1] };

		// Check if the encode has already been found in this function.
		FoundAlready = false;
		for (auto& Encode : vFoundEncodes) {
			if (Encode == BytePair) { FoundAlready = true; break; }
		}

		// If the encode has not been found already in this functions lifetime.
		if (!FoundAlready) {

			// Set it as found in the vectors and record its index.
			vFoundEncodes.push_back(BytePair);
			vFoundEncodesIndices.push_back(i);

			// Now we launch the GPU kernels to check for commonality.
			auto CommonalityKernel = q.submit([&](sycl::handler& h) {

				sycl::accessor aSamplesChar(bSamplesChar, h, sycl::read_only);
				sycl::accessor aEncodePresent(bEncodePresent, h, sycl::write_only);

				// Local memory array in each workgroup.
				sycl::accessor<int, 1, sycl::access::mode::read_write, sycl::access::target::local> aLocal_Mem(sycl::range<1>(WGS), h);

				h.parallel_for(sycl::nd_range<1>{N_Range, WGS}, [=](sycl::nd_item<1> TID) {
					auto wg = TID.get_group();
					auto x = TID.get_global_id(0);
					auto localmemaccessindex = TID.get_local_id(0);

					uint64_t SamplesCharEncodeIndex = i;

					// If in range of the string buffer.
					if (x < vSamplesCharSize - 1) {
						if (aSamplesChar[SamplesCharEncodeIndex] == aSamplesChar[x] && aSamplesChar[SamplesCharEncodeIndex + 1] == aSamplesChar[x + 1]) {
							aLocal_Mem[localmemaccessindex] = 1;
						}
						else aLocal_Mem[localmemaccessindex] = 0;
					}
					else aLocal_Mem[localmemaccessindex] = 0;

					// synchronise all the kernels.
					TID.barrier(sycl::access::fence_space::local_space);

					// do a partial sum reduction from local memory into the device global memory (VRAM on a GPU).
					int sum_wg = sycl::reduce_over_group(wg, aLocal_Mem[localmemaccessindex], sycl::plus<>());

					if (localmemaccessindex == 0) { aEncodePresent[TID.get_group_linear_id()] = sum_wg; }

					});
				});

			// Final stage of sum reduction.
			q.submit([&](sycl::handler& h) {
				// Depends on the previous kernel.
				h.depends_on(CommonalityKernel);

				sycl::accessor aEncodePresent(bEncodePresent, h, sycl::read_write);
				sycl::accessor aEncodeCommonality(bEncodeCommonality, h, sycl::write_only);

				h.single_task([=]() {
					uint64_t sum = 0;
					uint64_t CommonalityAccessIndex = i;
					for (int j = 0; j < N_Workgroups; j++) {
						sum += aEncodePresent[j];
					}
					aEncodeCommonality[CommonalityAccessIndex] = sum;
					});
				});
		}
	}	

	std::cout << "GPU kernels launched." << std::endl;
	// Now we wait for the queue to finish and return the results to CPU.
	q.wait();

	std::cout << "GPU Kernels Finished." << std::endl;

	bool ExistingEncode = false;
	// Now we iterate over found encodes retrieveing their commonality back from the GPU.
	for (uint64_t i = 0; i < vFoundEncodes.size(); i++) {
		ExistingEncode = false;
		// Iterate over existing encodings vector to check if its already been found previously before this function run.
		for (uint64_t j = 0; j < Encodings.size(); j++) {
			if (Encodings[j] == vFoundEncodes[i]) {
				Commonalities[j] += bEncodeCommonality.get_host_access()[vFoundEncodesIndices[i]];
				ExistingEncode = true;
			}
		}

		// If it was not an existing encode then we append it to the encodes vector along with its commonality to the commonality vector.
		if (!ExistingEncode) {
			Commonalities.push_back(bEncodeCommonality.get_host_access()[vFoundEncodesIndices[i]]);
			Encodings.push_back(vFoundEncodes[i]);
		}
	}

	// Lets see how long it took.
	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	TimeTaken = (EndTime - StartTime) / 1000000000;
	std::cout << "Time Taken: " << TimeTaken << " Seconds." << std::endl;


	// Sort The Encodings & Commonality Vectors.

	std::cout << "Sorting The Encodes." << std::endl;

	_SortEncodings();

	std::cout << "Sorting Of Encodes Done." << std::endl;

#ifdef _DEBUG
	std::cout << "Finished building encodes on GPU." << std::endl;
#endif // _DEBUG
};

// Encode Samples Functions.

std::vector<std::vector<int>> Tokenizer::Encode(std::vector<std::string> Samples) {
#ifdef _DEBUG
	std::cout << "Encoding strings on CPU." << std::endl;
#endif // _DEBUG
	std::vector<std::vector<int>> vEncodedSamples(Samples.size());
	bool EncodeFound = false;

	// Iterate over each string.
	for (uint64_t i = 0; i < Samples.size(); i++) {
		// Ensure the string is of even length by padding the end.
		if (Samples[i].length() % 2 != 0) {
			Samples[i].push_back((char)32);
		}
		// Iterate over each byte pair in the string
		for (uint64_t j = 0; j < Samples[i].size(); j += 2) {
			EncodeFound = false;

			// Iterate over each potential encode and check for match.
			for (uint64_t k = 0; k < Encodings.size(); k++) {
				// Check for match.
				if (Encodings[k].c_str()[0] == Samples[i].c_str()[j] && Encodings[k].c_str()[1] == Samples[i].c_str()[j + 1]) {
					vEncodedSamples[i].push_back(k);
					EncodeFound = true;
					break;
				}
			}
			// If failed to find an encode we need to set a backup Val for not encodable.
			if (!EncodeFound){ vEncodedSamples[i].push_back(-1); }
		}
	}
#ifdef _DEBUG
	std::cout << "Done Encoding Strings On CPU." << std::endl;
#endif // _DEBUG
	return vEncodedSamples;
};


// Decode Samples Functions.
std::string Tokenizer::Decode(std::vector<int> Samples) {
#ifdef _DEBUG
	std::cout << "Decoding Integer Tokens To Strings (CPU)." << std::endl;
#endif // _DEBUG

	std::string DecodedSamples;

	// Convert the encodings to strings.
	for (size_t i = 0; i < Samples.size(); i++) {
		if (Samples[i] == -1) { DecodedSamples.append("<<Unknown Encode>>"); }
		else { DecodedSamples.append(Encodings[Samples[i]]); }
	}

#ifdef _DEBUG
	std::cout << "Decoding Of The Tokens To Strings Is Done." << std::endl;
#endif // _DEBUG

	return DecodedSamples;
};


// Sort Encodings Function.
void Tokenizer::_SortEncodings() {
#ifdef _DEBUG
	std::cout << "Sorting Encodings & Commonality Vectors." << std::endl;
#endif // _DEBUG

	bool WholeVectorSorted = false;

	while (!WholeVectorSorted) {
		WholeVectorSorted = true;
		// Iterate over every commonality.
		for (uint64_t i = 0; i < Encodings.size() - 1; i++) {
			// If the one closer to the front is smaller than the one next to it.
			if (Commonalities[i] < Commonalities[i + 1]){

				// Swap the commonalities.
				uint64_t TmpCommonality = Commonalities[i];
				Commonalities[i] = Commonalities[i + 1];
				Commonalities[i + 1] = TmpCommonality;

				// Swap the Encodes.
				std::string TmpEncode = Encodings[i];
				Encodings[i] = Encodings[i + 1];
				Encodings[i + 1] = TmpEncode;
				WholeVectorSorted = false;
			}
		}
	}

#ifdef _DEBUG
	std::cout << "Encodings & Commonality Vectors Sorted." << std::endl;
#endif // _DEBUG
}

// Tokenizer load and save functions.

void Tokenizer::SaveTokenizer() {
#ifdef _DEBUG
	std::cout << "Saving Tokenizer to disc." << std::endl;
#endif // _DEBUG

	// First we need to perform some checks to ensure that the tokenizer is actually able to be saved to the disc properly and that it is worth saving it.

	if (TokenizerName.size() <= 0 || Encodings.size() <= 0 && Commonalities.size() != Encodings.size()) {
		std::cout << "Tokenizer is unable to be saved, it is either un named, or does not contain any encodes." << std::endl;
		return;
	}

	std::string FileName = "./" + TokenizerName + ".TOKENIZER";

	// Now we open a file pointer to the local director on disc.
	std::fstream File(FileName, std::ios::binary | std::ios::out);

	// We need to store the total file length, the length of the commonalities section, the length of the encodings section and an offset for each of them.
	uint64_t TotalFileSize, CommonalitiesLength, EncodingsLength, CommonalitiesOffset, EncodingsOffset;
	CommonalitiesLength = (Commonalities.size() * sizeof(uint64_t));
	EncodingsLength = (Encodings.size() * 2);
	TotalFileSize = EncodingsLength + CommonalitiesLength + (sizeof(uint64_t) * 5);
	EncodingsOffset = sizeof(uint64_t) * 5;
	CommonalitiesOffset = EncodingsOffset + EncodingsLength;

	// Write the offsets and lengths to the file.
	File.write((const char*)&TotalFileSize, sizeof(uint64_t));
	File.write((const char*)&EncodingsOffset, sizeof(uint64_t));
	File.write((const char*)&EncodingsLength, sizeof(uint64_t));
	File.write((const char*)&CommonalitiesOffset, sizeof(uint64_t));
	File.write((const char*)&CommonalitiesLength, sizeof(uint64_t));

	// Write the encodings vector to the file.
	for (auto& Encode : Encodings) {
		File.write((const char*)Encode.c_str(), sizeof(char) * 2);
	}

	// Write the commonalities vector to the file.
	for (auto& Commonality : Commonalities) {
		File.write((const char*)&Commonality, sizeof(uint64_t));
	}

	File.close();

#ifdef _DEBUG
	std::cout << "Tokenizer Written To File." << std::endl;
#endif // _DEBUG
}

void Tokenizer::LoadTokenizer() {
#ifdef _DEBUG
	std::cout << "Loading Tokenizer from disc." << std::endl;
#endif // _DEBUG

	// Warn the programmer that they will be overwriting the tokenizer if it is already populated.
	if (Encodings.size() > 0 || Commonalities.size() > 0) std::cout << "Warning, you are loading a tokenizer from disk to an already populated tokenizer, this will overwrite the tokenizer." << std::endl;

	std::string FileName = "./" + TokenizerName + ".TOKENIZER";
	// Create a file pointer so we can read in data from the file.
	std::ifstream File(FileName, std::ios::binary | std::ios::in);

	uint64_t TotalFileSize, CommonalitiesLength, EncodingsLength, CommonalitiesOffset, EncodingsOffset;

	// Seek to the beginning of the file to load in data.
	File.seekg(0);

	File.read((char*)&TotalFileSize, sizeof(uint64_t));
	File.read((char*)&EncodingsOffset, sizeof(uint64_t));
	File.read((char*)&EncodingsLength, sizeof(uint64_t));
	File.read((char*)&CommonalitiesOffset, sizeof(uint64_t));
	File.read((char*)&CommonalitiesLength, sizeof(uint64_t));

	// Load in the actual data now.

	Encodings.resize(EncodingsLength /  2);
	Commonalities.resize(CommonalitiesLength / sizeof(uint64_t));

	// Loading in the Encodes.
	File.seekg(EncodingsOffset);
	for (auto& Encode : Encodings) {
		Encode.resize(2);
		File.read((char*)Encode.data(), sizeof(char) * 2);
	}

	// Loading in the encode commonalities.
	File.seekg(CommonalitiesOffset);
	for (auto& Commonality : Commonalities) {
		File.read((char*)&Commonality, sizeof(uint64_t));
	}

#ifdef _DEBUG
	std::cout << "Tokenizer values Loaded from dic." << std::endl;
#endif // _DEBUG
}