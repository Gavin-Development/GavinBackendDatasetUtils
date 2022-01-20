#include "DataLoader.hpp"

Tokenizer::Tokenizer(std::string iTokenizerName, uint64_t iVocabSize) {
	std::cout << "Initialising a new tokenizer." << std::endl;
	TokenizerName = iTokenizerName;
	MaxVocabSize = iVocabSize;
};

Tokenizer::Tokenizer(std::string iTokenizerPath) {
	std::cout << "Loading existing tokenizer." << std::endl;
};

void Tokenizer::Tokenize(std::vector<std::string> Samples) {
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


void Tokenizer::Tokenize_MT(std::vector<std::string> Samples) {
	uint64_t NumberOfCoresToUse;

};