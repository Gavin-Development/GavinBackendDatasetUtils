#include "DataLoader.hpp"

Tokenizer::Tokenizer(std::string iTokenizerName, uint64_t iVocabSize) {
	std::cout << "Initialising a new tokenizer." << std::endl;
	TokenizerName = iTokenizerName;
	MaxVocabSize = iVocabSize;
};

Tokenizer::Tokenizer(std::string iTokenizerPath) {
	std::cout << "Loading existing tokenizer." << std::endl;
};