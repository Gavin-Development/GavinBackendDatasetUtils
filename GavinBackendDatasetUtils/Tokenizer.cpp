#include "DataLoader.hpp"

// Constructors.
Tokenizer::Tokenizer(std::string iTokenizerName, uint64_t iVocabSize) {
#ifdef _DEBUG
	std::cout << "Initialising a new tokenizer." << std::endl;
#endif // _DEBUG

	// set the max vocab sizes.
	TokenizerName = iTokenizerName;
	MaxVocabSize = iVocabSize;

	// Set the array sizes.
	Encodings.resize(0);
	Commonalities.resize(0);
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
		vSamplesChar.push_back((char)0);
	}

	// Setup a vector of encodes found during THIS tokenizer functions lifespan.
	// This is used if you are modifying an existing tokenizers encodes by adding data to them.
	std::vector<std::string> vFoundEncodes;

	// We iterate over EVERY unique byte pair in the sequence and determine the commonality of it.
	for (uint64_t i = 0; i < (vSamplesChar.size() - 1); i++) {
		// Setup the byte pair string to make syntax easier for comparisons.
		std::string BytePair{ vSamplesChar[i], vSamplesChar[i + 1] };

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

	// Some code to figure out workgroup stuffs.
	int MaxWorkGroupSize = q.get_device().get_info<sycl::info::device::max_work_group_size>();
	std::cout << "Device Max workgroup size: " << MaxWorkGroupSize << std::endl;

	// Convert Samples to char array and add in EOL / NULL tokens at the end of each word.
	std::vector<char> vSamplesChar;

	uint64_t vSamplesCharSize = 0;
	for (uint64_t i = 0; i < Samples.size(); i++) {
		vSamplesCharSize = vSamplesChar.size();
		vSamplesChar.resize(vSamplesCharSize + Samples[i].size());
		memcpy(&vSamplesChar[vSamplesCharSize], Samples[i].c_str(), sizeof(char) * Samples[i].size());
		vSamplesChar.push_back((char)0);
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

	// Creating a Commonality Buffer to store how many occurences of that encode there are, it is sum reduced after stuff is done.
	// Set to N_Range instead of the exact size of vSamplesChar to make sum reduction easier.
	sycl::buffer<int> bWordCommonality(sycl::range<1> {N_Range});

	// Specify a shader range.
	sycl::range<1> ShaderRange{vSamplesChar.size() - 1};

#ifdef _DEBUG
	std::cout << "Finished prepping long lived GPU buffers." << std::endl;
#endif // _DEBUG


	// We iterate over EVERY Unique Byte pair in the sequence and determine the commonality of it.
	for (uint64_t i = 0; i < (vSamplesChar.size() - 1); i++) {

		// Report progress.
		if ( i % ProgressReportInterval == 0) {
			std::cout << (((double)i / vSamplesCharSize) * 100) << " percent done." << std::endl;
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

			// Check if the Byte pair is an existing encode.
			bool ExistingEncode = false;
			uint64_t EncodeIndex;
			for (uint64_t j = 0; j < Encodings.size(); j++) {
				if (Encodings[j] == BytePair) { ExistingEncode = true; EncodeIndex = j; }
			}

			// Creating Byte pair buffer on GPU.
			sycl::buffer<char> bBytePair(BytePair);

			// Now we launch the GPU kernels to check for commonality.
			auto CommonalityKernel = q.submit([&](sycl::handler& h) {

				sycl::accessor aSamplesChar(bSamplesChar, h, sycl::read_only);
				sycl::accessor aBytePair(bBytePair, h, sycl::read_only);
				sycl::accessor aWordCommonality(bWordCommonality, h, sycl::write_only);

				h.parallel_for(ShaderRange, [=](sycl::id<1> TID) {
					int x = TID[0];

					if (aBytePair[0] == aSamplesChar[x] && aBytePair[1] == aSamplesChar[x + 1]) {
						aWordCommonality[x] = 1;
					}
					else aWordCommonality[x] = 0;
				});
			});

			//std::cout << "Commonality Found, Reduction Is Next." << std::endl;


			// Now we do sum reduction on the commonality buffer to get a final value.
			auto WorkGroupReduce = q.submit([&](sycl::handler& h) {
				h.depends_on(CommonalityKernel);

				sycl::accessor aWordCommonality(bWordCommonality, h, sycl::read_write);


				// Global work group size of the length of the buffer, local workgroup size of 8.
				h.parallel_for(sycl::nd_range<1>{N_Range, WGS}, [=](sycl::nd_item<1> TID) {
					auto wg = TID.get_group();
					auto i = TID.get_global_id(0);

					int sum_wg = sycl::reduce_over_group(wg, aWordCommonality[i], sycl::plus<>());

					if (TID.get_local_id(0) == 0) { aWordCommonality[i] = sum_wg; };
				});
			});

			// Create Commonality sum buffer.
			sycl::buffer<int> bCommonalitySum(sycl::range<1>{1});

			// Final stage of sum reduction.
			q.submit([&](sycl::handler& h) {
				// Depends on the previous kernel.
				h.depends_on(WorkGroupReduce);

				sycl::accessor aWordCommonality(bWordCommonality, h, sycl::read_write);
				sycl::accessor aCommonalitySum(bCommonalitySum, h);

				h.single_task([=]() {
					int sum = 0;
					// The 8 here is for the local workgroup size specified in the previous kernel.
					for (int i = 0; i < aWordCommonality.get_count(); i += WGS) {
						sum += aWordCommonality[i];
					}
					aCommonalitySum[0] = sum;
				});
			});


			// Wait for Q to finish.
			q.wait();

			// If the encoding is not already in the encodes vector
			if (!ExistingEncode) {
				//Push back the commonality and new unique byte pair encode.
				Commonalities.push_back(bCommonalitySum.get_host_access()[0]);
				Encodings.push_back(BytePair);
			}
			// If the encode is in the encodes vector.
			else {
				Commonalities[EncodeIndex] += bCommonalitySum.get_host_access()[0];
			}

			// Finally mark the Encode as found in the functions lifetime.
			vFoundEncodes.push_back(BytePair);

		}
	}

	// Lets see how long it took.
	EndTime = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
	TimeTaken = (EndTime - StartTime) / 1000000000;
	std::cout << "Time Taken: " << TimeTaken << " Seconds." << std::endl;


#ifdef _DEBUG
	for (uint64_t i = 0; i < Encodings.size(); i++) {
		std::cout << Encodings[i] << "  " << Commonality[i] << std::endl;
	}

	std::cout << "Finished building encodes on GPU." << std::endl;
#endif // _DEBUG
};

// Encode Samples Functions.


std::vector<int> Tokenizer::Encode(std::vector<std::string> Samples) {
#ifdef _DEBUG
	std::cout << "Encoding strings on CPU." << std::endl;
#endif // _DEBUG

	std::vector<std::vector<char>> vSamplesChar;

	for (uint64_t i = 0; i < Samples.size(); i++) {
		std::vector<char> vSampleChar(Samples[i].size());
		memcpy(vSampleChar.data(), &Samples[i], sizeof(char) * Samples.size());
	}

};


// Decode Samples Functions.
std::vector<std::string> Tokenizer::Decode(std::vector<int> Samples) {
#ifdef _DEBUG
	std::cout << "Decoding Integer Tokens To Strings (CPU)." << std::endl;
#endif // _DEBUG

	std::vector<std::string> DecodedSamples(Samples.size());

	// Convert the encodings to strings.
	for (size_t i = 0; i < Samples.size(); i++) {
		if (Samples[i] != -1 && Samples[i] < MaxVocabSize) DecodedSamples[i] = Encodings[Samples[i]];
	}

#ifdef _DEBUG
	std::cout << "Decoding Of The Tokens To Strings Is Done." << std::endl;
#endif // _DEBUG

	return DecodedSamples;
};