#include "DataLoader.hpp"


// Helper function for getting largest key
template <typename T>
inline const typename T::key_type& last_key(const T& pMap)
{
    return pMap.rbegin()->first;
}


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

std::vector<std::string> Tokenizer::_split_sentences(const std::string &delimiter, const std::vector<std::string>& sentences) {
    std::size_t pos = 0;
    std::string token;
    std::vector<std::vector<std::string>> values;
    values.reserve(sentences.size());
    for (const auto& sentence : sentences) {
        values.push_back(_split_sentence(delimiter, sentence));
    }
    std::vector<std::string> result;
    result.reserve(sentences.size() * values[0].size());
    for (const auto& sentence : values) {
        for (const auto& word : sentence) {
            result.push_back(word);
        }
    }
    return result;
}


int Tokenizer::get_token_id(const std::string& token) {
    return Vocab_inv[token];
}

std::map<int, std::string> Tokenizer::build_vocab_for_string(const std::vector<std::string>& sentences) {
    std::vector<std::string> words = _split_sentences(" ", sentences);
    std::map<int, std::string> vocab = {{0, END_OF_WORD}};
    int uid = last_key(vocab);
    while (true) {
        std::map<std::tuple<std::string, std::string>, int> pairs;
        int best_freq = 0;
        std::tuple<std::string, std::string> best_pair = std::make_tuple("", "");
        for (const auto& word: words) {
            for (int i =0; i<word.size(); i++) {
                std::tuple<std::string, std::string> pair = std::make_tuple(word.substr(i), word.substr(i+2));
                int count = std::count(pairs.begin(), pairs.end(), pair);
                pairs[pair] = count;
                if (count > best_freq) {
                    best_freq = count;
                    best_pair = pair;
                }
            }
        }
        if (best_pair == std::make_tuple("", "")) {
            break;
        }
        uid++;
        std::string string_best_pair = std::get<0>(best_pair) + std::get<1>(best_pair);
        vocab[uid] = string_best_pair;

        for (auto word: words) {
            if (word == string_best_pair) {
                word.erase(0, 2);
            }
            else {
                for (int i=0; i<word.size(); i++) {
                    if (word.substr(i, 2) == string_best_pair) {
                        std::replace(word.begin(), word.end(), string_best_pair, std::to_string(uid));
                    }
                }
            }
        }

    }

    return vocab;
}


std::map<int, std::string> Tokenizer::merge(std::map<int, std::string> vok1, std::map<int, std::string> vok2) {
    std::set<int> keys_1;
    std::set<int> keys_2;
    for (const auto& key: vok1) {
        keys_1.insert(key.first);
    }
    for (const auto& key: vok2) {
        keys_2.insert(key.first);
    }
    std::set<int> keys_union;
    std::set_union(keys_1.begin(), keys_1.end(), keys_2.begin(), keys_2.end(),
                   std::inserter(keys_union, keys_union.begin()));
    std::map<int, std::string> result;
    for (const auto& key: keys_union) {
        if (vok1.find(key) != vok1.end()) {
            result[key] = vok1[key];
        }
        else {
            result[key] = vok2[key];
        }
    }
    return result;
}


std::list<std::list<std::string>> Tokenizer::chunk_data(std::list<std::string> data, int chunk_size) {
    std::list<std::list<std::string>> result;
    std::list<std::string> chunk;
    for (const auto& sentence: data) {
        if (chunk.size() == chunk_size) {
            result.push_back(chunk);
            chunk.clear();
        }
        chunk.push_back(sentence);
    }
    if (!chunk.empty()) {
        result.push_back(chunk);
    }
    return result;
}


uint64_t Tokenizer::get_vocab_size() {
    return Vocab.size();
}

std::map<int, std::string> Tokenizer::get_vocab() {
    return Vocab;
}
