#include "DataLoader.hpp"

#include <CL/sycl.hpp>


// Helper functions for GPU stuff.
bool _CheckMGPUCapability(std::vector<sycl::device>* pGPUs) {
	std::vector<std::string> UniqueDeviceNames;
	std::vector<int> UniqueDeviceCount;


	bool DeviceExistsAlready;
	// List available devices.
	for (auto device : sycl::device::get_devices(sycl::info::device_type::gpu)) {
		DeviceExistsAlready = false;
		for (uint32_t i = 0; i < UniqueDeviceNames.size(); i++) {
			if (device.get_info<sycl::info::device::name>() == UniqueDeviceNames[i]) {
				UniqueDeviceCount[i]++;
				DeviceExistsAlready = true;
			}
		}

		if (!DeviceExistsAlready) {
			UniqueDeviceNames.push_back(device.get_info<sycl::info::device::name>());
			UniqueDeviceCount.push_back(1);
		}
	}

	// pick the most common device in the system.
	int DeviceToUseIndex = 0;
	for (uint32_t i = 0; i < UniqueDeviceNames.size(); i++) {
		if (UniqueDeviceCount[i] > UniqueDeviceCount[DeviceToUseIndex]) {
			DeviceToUseIndex = i;
		}
	}

	// If there are multiple of this device type then we are good to go and push them back to the vector.
	if (UniqueDeviceCount[DeviceToUseIndex] > 1) {
		for (auto device : sycl::device::get_devices(sycl::info::device_type::gpu)) {
			if (device.get_info<sycl::info::device::name>() == UniqueDeviceNames[DeviceToUseIndex]) {
				pGPUs->push_back(device);
			}
		}
		return true;
	}
	// Just incase there are no duplicate devices.
	else { return false; }
};

// Constructors.
Tokenizer::Tokenizer(std::string iTokenizerName) {
#ifdef _DEBUG
	std::cout << "Initialising a new tokenizer and loading data from disk." << std::endl;
#endif // _DEBUG

	// Set up some Tokenizer variables.
	TokenizerName = iTokenizerName;

	if (TokenizerName.size() <= 0) { std::cout << TokenizerName << " is an invalid tokenizer name." << std::endl; throw std::runtime_error(TokenizerName + " is an invalid tokenizer name."); }

	Encodings.resize(0);
	Commonalities.resize(0);

	if (!_LoadTokenizer()) { std::cout << "Unable to load tokenizer from on disk." << std::endl; throw std::runtime_error("No Tokenizer found on disk."); }
	else std::cout << "Tokenizer successfully loaded from disk." << std::endl;

#ifdef _DEBUG
	std::cout << "Initialisation Done." << std::endl;
#endif // _DEBUG
}

Tokenizer::Tokenizer() {
#ifdef _DEBUG
	std::cout << "Initialising a new nameless tokenizer." << std::endl;
#endif // _DEBUG

	// Set some values.
	Encodings.resize(0);
	Commonalities.resize(0);
	TokenizerName.resize(0);

#ifdef _DEBUG
	std::cout << "Initialisation Done." << std::endl;
#endif // _DEBUG
}

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

	// Check device MGPU capabilities.
	std::vector<sycl::device> GPUs;

	bool MGPUSupport = _CheckMGPUCapability(&GPUs);

	// Setup a vector of found Encodes that have been found during the lifetime of this function.
	std::vector<std::string> vFoundEncodes;
	std::vector<uint64_t> vFoundEncodesCommonalities;

	// Do the OG algo.
	if (!MGPUSupport) {
		std::cout << "There is no multi GPU support detected." << std::endl;

		// SYCL initialisation. Using the defulat selector since doing this on 1 GPU. Let it choose the best for us.
		sycl::default_selector d_selector;
		sycl::queue q(d_selector);

		// Setup a vector of found encode indices during the lifetime of this function.
		std::vector<uint64_t> vFoundEncodesIndices;

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

		// Loop over all the found Encodes and push them back to the main vector.
		for (uint64_t i = 0; i < vFoundEncodes.size(); i++) {
			vFoundEncodesCommonalities.push_back(bEncodeCommonality.get_host_access()[vFoundEncodesIndices[i]]);
		}
	}

	// Do the ultra special mGPU algo :)
	else {
		std::cout << "Multi GPU support detected, splitting Ops between " << GPUs.size() << "x " << GPUs[0].get_info<sycl::info::device::name>() << std::endl;

		sycl::queue q(GPUs[0]);

		// Get some useful device info for performing checks and other stuff later on.
		int MaxWorkGroupSize = q.get_device().get_info<sycl::info::device::max_work_group_size>();
		uint64_t DeviceMemory = GPUs[0].get_info<sycl::info::device::global_mem_size>();
		uint64_t TotalMemoryAvailable = DeviceMemory * GPUs.size();
		uint64_t GPUDataSliceSize = Samples.size() / GPUs.size();

		std::vector<std::vector<char>> vSamplesChar;
		uint64_t VsamplesCharSize = 0;
		for (uint32_t i = 0; i < GPUs.size(); i++) {
			// Resize along dim 0
			vSamplesChar.resize(vSamplesChar.size() + 1);
			// If NOT doing the final GPU data prep.
			if (i < GPUs.size() - 1) {
				// Iterate over adding samples to the vector.
				for (uint64_t j = i * GPUDataSliceSize; j < (i + 1) * GPUDataSliceSize; j++) {
					VsamplesCharSize = vSamplesChar[i].size();
					// Resize the vector so that it can take the extra sample.
					vSamplesChar[i].resize(VsamplesCharSize + Samples[j].size());
					// Copy the sample into the vector at the end position.
					memcpy(&vSamplesChar[i][VsamplesCharSize], Samples[j].c_str(), sizeof(char) * Samples[j].size());
					// Push back a termination token.	
					vSamplesChar[i].push_back((char)32);
				}
			}
			// If we ARE doing the final GPU data prep.
			else {
				// Iterate over adding samples to the vector. The limits are changed for the final GPU data prep as it extends to the end of the vector instead of to the end of its block.
				for (uint64_t j = i * GPUDataSliceSize; j < Samples.size(); j++) {
					VsamplesCharSize = vSamplesChar[i].size();
					// Resize the vector so that it can take the extra sample.
					vSamplesChar[i].resize(VsamplesCharSize + Samples[j].size());
					// Copy the sample into the vector at the end position.
					memcpy(&vSamplesChar[i][VsamplesCharSize], Samples[j].c_str(), sizeof(char) * Samples[j].size());
					// Push back a termination token.	
					vSamplesChar[i].push_back((char)32);
				}
			}
		}

		// Iterate over each of the GPU devices and launch a thread to schedule tasks for them.
		std::vector<std::thread> vThreads;
		uint32_t ThreadNum = 0;
		std::mutex EndOfThreadLockMutex;
		for (sycl::device& GPU : GPUs) {
			// Push back the thread to do work on the GPU.
			vThreads.push_back(std::thread([&vSamplesChar, ThreadNum, GPU, MaxWorkGroupSize, &EndOfThreadLockMutex, &vFoundEncodes, &vFoundEncodesCommonalities]() {
				// Setup the size of the buffer thingy.
				uint64_t vThreadSamplesCharSize = vSamplesChar[ThreadNum].size();

				// Setup the SYCL queue with the device.
				sycl::queue q(GPU);

				// Setup the buffer full of samples on the device.
				sycl::buffer<char> bSamplesChar(vSamplesChar[ThreadNum]);

				// Determine the WGS and N_Range of the ND kernel.
				uint64_t WGS = MaxWorkGroupSize;
				uint64_t N_Range = 0;
				uint64_t N_WorkGroups = 0;
				while (N_WorkGroups * WGS < vThreadSamplesCharSize) {
					N_WorkGroups++;
				}
				N_Range = N_WorkGroups * WGS;

				// Setup the CPU side vectors for tracking the already found encodes.
				std::vector<std::string> vThreadFoundEncodes;
				std::vector<uint64_t> vThreadFoundEncodesIndices;

				// Setup the buffer to record if the encode is present in the WGP, this buffer will store how many of the encode are present in the WGP items.
				sycl::buffer<int> bEncodePresent(sycl::range<1> {N_WorkGroups});

				// Setup the buffer to store the encode overall commonality.
				sycl::buffer<uint64_t> bEncodeCommonality(sycl::range<1> {vThreadSamplesCharSize - 1});

				bool FoundAlready;
				std::string BytePair;
				BytePair.resize(2);
				BytePair.reserve(2);
				// Loop over all the samples in the buffer and check how many of them appear if they are unique.
				for (uint64_t i = 0; i < (vThreadSamplesCharSize - 1); i++) {
					// Creating the byte pair string.
					BytePair = { vSamplesChar[ThreadNum][i], vSamplesChar[ThreadNum][i + 1] };

					// Check if the encode has already been found in this thread.
					FoundAlready = false;
					for (auto& Encode : vThreadFoundEncodes) {
						if (Encode == BytePair) {
							FoundAlready = true; break;
						}
					}

					// If the encode has not already been found in this thread.
					if (!FoundAlready) {

						vThreadFoundEncodes.push_back(BytePair);
						vThreadFoundEncodesIndices.push_back(i);

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
								if (x < vThreadSamplesCharSize - 1) {
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

						// Final stage of sum reduciton that was partially completed in the main kernel.

						q.submit([&](sycl::handler& h) {
							// Depends on the previous Kernel.
							h.depends_on(CommonalityKernel);

							sycl::accessor aEncodePresent(bEncodePresent, h, sycl::read_write);
							sycl::accessor aEncodeCommonality(bEncodeCommonality, h, sycl::write_only);

							h.single_task([=]() {
								uint64_t sum = 0;
								uint64_t CommonalityAccessIndex = i;
								for (int j = 0; j < N_WorkGroups; j++) {
									sum += aEncodePresent[j];
								}
								aEncodeCommonality[CommonalityAccessIndex] = sum;
								});
							});
					}

				}
				// Now that all the GPU kernels have launched we need to synchronise them back to the device.
				q.wait();

				// Try to acquire the mutex to append stuff to the main threads vectors.
				bool AcquiredMutex = false;
				while (!AcquiredMutex) { AcquiredMutex = EndOfThreadLockMutex.try_lock(); };

				// Loop over all the found Encodes and push them back to the main vector.
				for (uint64_t i = 0; i < vThreadFoundEncodes.size(); i++) {
					vFoundEncodes.push_back(vThreadFoundEncodes[i]);
					vFoundEncodesCommonalities.push_back(bEncodeCommonality.get_host_access()[vThreadFoundEncodesIndices[i]]);
				}

				// Finally we unlock the mutex so that the other thread can continue to do work.
				EndOfThreadLockMutex.unlock();
				}));
			ThreadNum++;
		}

		std::cout << "Threads launched." << std::endl;

		// Join the threads back to the main thread and continue on with our day.
		for (auto& th : vThreads) {
			th.join();
		}

		std::cout << "Threads joined." << std::endl;
	}

	bool ExistingEncode = false;
	// Now we iterate over found encodes retrieveing their commonality back from the GPU.
	for (uint64_t i = 0; i < vFoundEncodes.size(); i++) {
		ExistingEncode = false;
		// Iterate over existing encodings vector to check if its already been found previously before this function run.
		for (uint64_t j = 0; j < Encodings.size(); j++) {
			if (Encodings[j] == vFoundEncodes[i]) {
				Commonalities[j] += vFoundEncodesCommonalities[i];
				ExistingEncode = true;
			}
		}

		// If it was not an existing encode then we append it to the encodes vector along with its commonality to the commonality vector.
		if (!ExistingEncode) {
			Commonalities.push_back(vFoundEncodesCommonalities[i]);
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
			if (!EncodeFound) { vEncodedSamples[i].push_back(-1); }
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
			if (Commonalities[i] < Commonalities[i + 1]) {

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

bool Tokenizer::_SaveTokenizer() {
#ifdef _DEBUG
	std::cout << "Saving Tokenizer to disc." << std::endl;
#endif // _DEBUG

	std::string FileName = "./" + TokenizerName + ".TOKENIZER";

	// Now we open a file pointer to the local director on disc.
	std::fstream File(FileName, std::ios::binary | std::ios::out);

	// Check if the file was opened correctly, if not return false to the calling function.
	if (!File.is_open()) { std::cout << "Failed to create file on disk." << std::endl; return false; }

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

	return true;
}

bool Tokenizer::_LoadTokenizer() {
#ifdef _DEBUG
	std::cout << "Loading Tokenizer from disc." << std::endl;
#endif // _DEBUG

	std::string FileName = "./" + TokenizerName + ".TOKENIZER";
	// Create a file pointer so we can read in data from the file.
	std::ifstream File(FileName, std::ios::binary | std::ios::in);

	// check if the file opened correctlym, if not return false to the calling function.
	if (!File.is_open()) { std::cout << "Failed to open file on disk." << std::endl; return false; }

	uint64_t TotalFileSize, CommonalitiesLength, EncodingsLength, CommonalitiesOffset, EncodingsOffset;

	// Seek to the beginning of the file to load in data.
	File.seekg(0);

	File.read((char*)&TotalFileSize, sizeof(uint64_t));
	File.read((char*)&EncodingsOffset, sizeof(uint64_t));
	File.read((char*)&EncodingsLength, sizeof(uint64_t));
	File.read((char*)&CommonalitiesOffset, sizeof(uint64_t));
	File.read((char*)&CommonalitiesLength, sizeof(uint64_t));

	// Load in the actual data now.

	Encodings.resize(EncodingsLength / 2);
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

	return true;
}

bool Tokenizer::SaveTokenizer() {
#ifdef _DEBUG
	std::cout << "Python exposed save Tokenizer function called." << std::endl;
#endif // _DEBUG

	// First we need to perform some checks to ensure that the tokenizer can be saved to disk.

	if (Encodings.size() <= 0 && Commonalities.size() != Encodings.size()) std::cout << "Tokenizer can not be saved, contains no enodes." << std::endl; return false;

	if (TokenizerName.size() <= 0) std::cout << "No tokenizer name specified." << std::endl; return false;


	// Now we attempt to save the tokenizer to disk.
	if (!_SaveTokenizer()) std::cout << "An error occured while trying to save tokenizer." << std::endl; return false;

#ifdef _DEBUG
	std::cout << "Python exposed tokenizer save function complete." << std::endl;
#endif // _DEBUG

	return true;
}

bool Tokenizer::SaveTokenizer(std::string iTokenizerName) {
#ifdef _DEBUG
	std::cout << "Python exposed save tokenizer function with name override called." << std::endl;
#endif // _DEBUG

	// Set the tokenizer name.
	TokenizerName = iTokenizerName;

	// perform some checks to ensure that the tokenizer can actually be saved.
	if (Encodings.size() <= 0 && Commonalities.size() != Encodings.size()) std::cout << "Tokenizer can not be saved, contains no enodes." << std::endl; return false;

	if (TokenizerName.size() <= 0) std::cout << "Invalid tokenizer name specified." << std::endl; return false;

	// Now we attempt to save the tokenizer to disk.
	if (!_SaveTokenizer()) std::cout << "An error occured while trying to save tokenizer." << std::endl; return false;

#ifdef _DEBUG
	std::cout << "Python exposed tokenizer save function complete." << std::endl;
#endif // _DEBUG

	return true;
}

bool Tokenizer::LoadTokenizer() {
#ifdef _DEBUG
	std::cout << "Python function to load Tokenizer from disk called." << std::endl;
#endif // _DEBUG

	// Warn the programmer that they will be overwriting the tokenizer if it is already populated.
	if (Encodings.size() > 0 || Commonalities.size() > 0) { std::cout << "Warning, you are loading a tokenizer from disk to an already populated tokenizer, this will overwrite the tokenizer." << std::endl; return false; }

	if (TokenizerName.size() <= 0) { std::cout << "Un named tokenizer, please pass a name for the tokenizer you wish to load." << std::endl; return false; }

	// Now we attempt to load the tokenizer from disk.
	if (!_LoadTokenizer()) { std::cout << "An error occured while trying to load tokenizer." << std::endl; return false; }


#ifdef _DEBUG
	std::cout << "Python function to load Tokenizer from disk finished." << std::endl;
#endif // _DEBUG

	return true;
}

bool Tokenizer::LoadTokenizer(std::string iTokenizerName) {
#ifdef _DEBUG
	std::cout << "Python exposed load tokenizer function with name override called." << std::endl;
#endif // _DEBUG

	// Warn the programmer that they will be overwriting the tokenizer if it is already populated.
	if (Encodings.size() > 0 || Commonalities.size() > 0) { std::cout << "Warning, you are loading a tokenizer from disk to an already populated tokenizer, this will overwrite the tokenizer." << std::endl; return false; }

	// check that the tokenizer can be loaded and that it wont overwrite anything.
	if (iTokenizerName.size() <= 0) { std::cout << "Invalid tokenizer name." << std::endl; return false; }

	// Set the tokenizer name.
	TokenizerName = iTokenizerName;

	// Now we attempt to load the tokenizer from disk.
	if (!_LoadTokenizer()) { std::cout << "An error occured while trying to load tokenizer." << std::endl; return false; }

#ifdef _DEBUG
	std::cout << "Python function to load tokenizer from disk with name override finished." << std::endl;
#endif // _DEBUG
	return true;
}