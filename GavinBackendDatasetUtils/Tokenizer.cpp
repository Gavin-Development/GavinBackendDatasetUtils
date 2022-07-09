#include "DataLoader.hpp"

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
		std::string BytePair{ vSamplesChar[i], vSamplesChar[i + 1] };

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


// Needs re designing to take into account GPUs of various sizes.
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
	while (N_Range * WGS < vSamplesCharSize) {
		N_Range++;
	}
	N_Range = N_Range * WGS;
	std::cout << "Work Group Size: " << WGS << std::endl;
	std::cout << "Global Range: " << N_Range << std::endl;

	// Set a value for the interval to report progress.
	uint64_t ProgressReportInterval = vSamplesCharSize / 100;

	// Setup a vector of found Encodes that have been found during the lifetime of this function.
	std::vector<std::string> vFoundEncodes;
	std::vector<uint64_t> vFoundEncodesIndices;

	// Value set to 1 if proposed encode is present at that point in the samples buffer. Sum reduced to another buffer.
	sycl::buffer<int> bEncodePresent(sycl::range<1> {N_Range});

	// Creating a buffer to store the reduced commonality values in. It is set to the max possible size it could ever be.
	sycl::buffer<int> bEncodeCommonality(sycl::range<1> {vSamplesCharSize - 1});

	// Specify a shader range.
	sycl::range<1> ShaderRange{vSamplesChar.size() - 1};

#ifdef _DEBUG
	std::cout << "Finished prepping long lived GPU buffers." << std::endl;
#endif // _DEBUG


	// We iterate over EVERY Unique Byte pair in the sequence and determine the commonality of it.
	for (uint64_t i = 0; i < (vSamplesChar.size() - 1); i++) {

		// Report progress.
		if (i % ProgressReportInterval == 0) {
			double PercentDone = (((double)i / vSamplesCharSize) * 100);
			std::cout << round(PercentDone) << " percent done." << std::endl;
		}

		// creating a string for the byte pair.
		std::string BytePair{ vSamplesChar[i], vSamplesChar[i + 1] };

		// Check if the encode has already been found in this function.
		bool FoundAlready = false;
		for (auto& Encode : vFoundEncodes) {
			if (Encode == BytePair) { FoundAlready = true; }
		}

		// If the encode has not been found already in this functions lifetime.
		if (!FoundAlready) {

			// Set it as found in the vectors and record its index.
			vFoundEncodes.push_back(BytePair);
			vFoundEncodesIndices.push_back(i);

			// We need to wait for the GPU to finish using the aEncodePresent buffer from the previous launch before we can make it do anything else.
			q.wait();


			// Now we launch the GPU kernels to check for commonality.
			auto CommonalityKernel = q.submit([&](sycl::handler& h) {

				sycl::accessor aSamplesChar(bSamplesChar, h, sycl::read_only);
				sycl::accessor aEncodePresent(bEncodePresent, h, sycl::write_only);

				h.parallel_for(ShaderRange, [=](sycl::id<1> TID) {
					int x = TID[0];

					int SamplesCharEncodeIndex = i;

					if (aSamplesChar[SamplesCharEncodeIndex] == aSamplesChar[x] && aSamplesChar[SamplesCharEncodeIndex + 1] == aSamplesChar[x + 1]) {
						aEncodePresent[x] = 1;
					}
					else aEncodePresent[x] = 0;
				});
			});

			// Now we do sum reduction on the commonality buffer to get a final value.
			auto WorkGroupReduce = q.submit([&](sycl::handler& h) {
				h.depends_on(CommonalityKernel);

				sycl::accessor aEncodePresent(bEncodePresent, h, sycl::read_write);


				h.parallel_for(sycl::nd_range<1>{N_Range, WGS}, [=](sycl::nd_item<1> TID) {
					auto wg = TID.get_group();
					auto i = TID.get_global_id(0);

					int sum_wg = sycl::reduce_over_group(wg, aEncodePresent[i], sycl::plus<>());

					if (TID.get_local_id(0) == 0) { aEncodePresent[i] = sum_wg; };
				});
			});

			// Final stage of sum reduction.
			q.submit([&](sycl::handler& h) {
				// Depends on the previous kernel.
				h.depends_on(WorkGroupReduce);

				sycl::accessor aEncodePresent(bEncodePresent, h, sycl::read_write);
				sycl::accessor aEncodeCommonality(bEncodeCommonality, h);

				h.single_task([=]() {
					int sum = 0;
					int CommonalityAccessIndex = i;
					for (int i = 0; i < aEncodePresent.get_count(); i += WGS) {
						sum += aEncodePresent[i];
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
				int TmpCommonality = Commonalities[i];
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