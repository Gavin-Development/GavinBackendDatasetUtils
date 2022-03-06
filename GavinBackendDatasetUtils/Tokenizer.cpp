#include "DataLoader.hpp"

Tokenizer::Tokenizer(std::string iTokenizerName, uint64_t iVocabSize) {
	std::cout << "Initialising a new tokenizer." << std::endl;
	TokenizerName = iTokenizerName;
	MaxVocabSize = iVocabSize;
};

Tokenizer::Tokenizer(std::string iTokenizerPath) {
	std::cout << "Loading existing tokenizer." << std::endl;
};

std::vector<std::string> Tokenizer::_split_sentence(const std::string &delimiter, std::string sentence) {
    std::size_t pos = 0;
    std::string token;
    std::vector<std::string> values;
    while ((pos = sentence.find(delimiter)) != std::string::npos) {
        token = sentence.substr(0, pos);
        values.push_back(token);
        sentence.erase(0, pos + delimiter.length());
    }
    values.push_back(sentence);
    return values;
}


std::vector<std::vector<std::string>> Tokenizer::_split_sentences(const std::string &delimiter, const std::vector<std::string>& sentences) {
    std::size_t pos = 0;
    std::string token;
    std::vector<std::vector<std::string>> values;
    values.reserve(sentences.size());
    for (const auto& sentence : sentences) {
        values.push_back(_split_sentence(delimiter, sentence));
    }
    return values;
}


int Tokenizer::get_token_id(const std::string& token) {
    return Vocab_inv[token];
}

