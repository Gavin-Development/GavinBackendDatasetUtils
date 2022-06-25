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
	Commonality.resize(0);
};

// Build Encodes Functions.
void Tokenizer::BuildEncodes(std::vector<std::string> Samples) {
	bool EncodingExists;

	for (std::string Word : Samples) {
		EncodingExists = false;
		for (int i = 0; i < Encodings.size(); i++) {
			if (Encodings[i] == Word) {
				Commonality[i]++;
				EncodingExists = true;
				break;
			}
		}

		if (EncodingExists == false && Encodings.size() < MaxVocabSize) {
			Encodings.push_back(Word);
			Commonality.push_back(1);
		}
	}
};

// Encode Samples Functions.
std::vector<int> Tokenizer::Encode_GPU(std::vector<std::string> Samples) {
#ifdef _DEBUG
	std::cout << "SYCL GPU accelerated Tokenizer function launched." << std::endl;
#endif // _DEBUG

	// Vector containing the integer encodes of the samples.
	std::vector<int> EncodedSamples(Samples.size());

	// Setting default values to -1 such that we can indicate if encode failed.
	for (auto& Encode : EncodedSamples) {
		Encode = -1;
	}



	// Setup some SYCL stuff
	sycl::default_selector d_selector;
	sycl::queue q(d_selector);

	// Print the selected device to console.
	std::cout << "Using Device: " << q.get_device().get_info<sycl::info::device::name>() << std::endl;

	sycl::buffer bEncodingDict(Encodings);
	sycl::buffer bSamples(Samples);
	sycl::buffer bEncodedSamples(EncodedSamples);

	sycl::range<2> ShaderRange{ Encodings.size(), Samples.size() };

#ifdef _DEBUG
	std::cout << "Launching SYCL kernels." << std::endl;
#endif // _DEBUG

	q.submit([&](sycl::handler& h) {

		sycl::accessor aEncodingDict(bEncodingDict, h, sycl::read_only);
		sycl::accessor aSamples(bSamples, h, sycl::read_only);
		sycl::accessor aEncodedSamples(bEncodedSamples, h, sycl::read_write);

		// 2D loop to check all samples against all encodes in a single shader. Allow the GPU to schedule this mess :)
		h.parallel_for(ShaderRange, [=](sycl::id<2> TID) {
			int x, y;
			// X is which sample you are checking
			// Y is which encode you are checking the sample against.
			x = TID[1];
			y = TID[0];

			if (aSamples[x] == aEncodingDict[y]) {
				// If The sample matches the encode then we set the samples encoded value to the index of the encode.
				aEncodedSamples[x] = y;
			}

			});
	});

	// Wait for the kernels to complete since the Q is asynchronous and not synchronised with the host.
	q.wait();

	// Destruct the buffer to force write back.
	bEncodedSamples.~buffer();

#ifdef _DEBUG
	std::cout << "SYCL Kernels Complete & Write Back Complete." << std::endl;
#endif // _DEBUG


	return EncodedSamples;

};


// Decode Samples Functions.
std::vector<std::string> Tokenizer::Decode_CPU(std::vector<int> Samples) {
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