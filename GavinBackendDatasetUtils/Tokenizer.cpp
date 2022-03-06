#include <utility>

#include "DataLoader.hpp"


// Helper function for getting largest key
template <typename T>
inline const typename T::key_type& last_key(const T& pMap)
{
    return pMap.rbegin()->first;
}


Tokenizer::Tokenizer(std::string iTokenizerName, uint64_t iVocabSize) {
	std::cout << "Initialising a new tokenizer." << std::endl;
	TokenizerName = std::move(iTokenizerName);
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


std::map<int, std::string> Tokenizer::_build_vocab_for_string(const std::vector<std::string>& sentences,
                                                              std::string end_of_word, uint64_t max_vocab_size) {
    std::vector<std::string> words = _split_sentences(" ", sentences);
    std::map<int, std::string> vocab = {{0, end_of_word}};
    int uid = last_key(vocab);
    while (true) {
        std::map<std::tuple<std::string, std::string>, int> pairs;
        int best_freq = 0;
        std::tuple<std::string, std::string> best_pair = std::make_tuple("", "");
        for (const auto& word: words) {
            for (int i =0; i<word.size(); i++) {
                std::tuple<std::string, std::string> pair = std::make_tuple(word.substr(i), word.substr(i+2));
                int count = pairs[pair]+1;
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
                        std::string uid_str = std::to_string(uid);
                        word.replace(i, 2, uid_str);
                    }
                }
            }
        }
        if (uid >= max_vocab_size) {
            break;
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


std::list<std::list<std::string>> Tokenizer::chunk_data(const std::list<std::string> &data, int chunk_size) {
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

std::vector<std::map<int, std::string>> Tokenizer::chunk_vocab(const std::list<std::map<int, std::string>> &data,
                                                             int chunk_size) {
    std::vector<std::map<int, std::string>> result;
    std::map<int, std::string> chunk;
    for (const auto& vocab: data) {
        if (chunk.size() == chunk_size) {
            result.push_back(chunk);
            chunk.clear();
        }
        chunk.insert(vocab.begin(), vocab.end());
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


void Tokenizer::build_vocab(const std::list<std::string>& corpus) {
    unsigned int CHUNK_SIZE = std::thread::hardware_concurrency();
    if (CHUNK_SIZE == 0) {
        CHUNK_SIZE = 1;
    }
    else if (CHUNK_SIZE % 2 == 0 && CHUNK_SIZE > 1) {
        CHUNK_SIZE--;
    }
    std::list<std::list<std::string>> chunks = chunk_data(corpus, (int)CHUNK_SIZE);
    std::vector<std::future<std::map<int, std::string>>> futures;
    futures.reserve(chunks.size());
    for (const auto& chunk: chunks) {
        std::future future = std::async([this, chunk]() {
            return _build_vocab_for_string((const std::vector<std::basic_string<char>> &) chunk, END_OF_WORD, MaxVocabSize);
        });
        futures.push_back(std::move(future));
    }
    std::list<std::map<int, std::string>> vocabs;
    for (auto& future: futures) {
        vocabs.push_back(future.get());
    }
    bool done = false;
    while (!done) {
        futures.clear();
        CHUNK_SIZE = ceil(CHUNK_SIZE/2);
        std::vector<std::map<int, std::string>> vocab_chunks = chunk_vocab(vocabs, (int)CHUNK_SIZE);
        for (int i=0; i<ceil(vocab_chunks.size()/2); i++) {
            std::future future = std::async([this, vocab_chunks, &i]() {
                return merge(vocab_chunks[i], vocab_chunks[i+1]);
            });
            futures.push_back(std::move(future));
        }
        vocabs.clear();
        for (auto& future: futures) {
            vocabs.push_back(future.get());
        }
        if (vocabs.size() == 1) {
            done = true;
        }
        else if (CHUNK_SIZE == 0) {
            done = true;
        }
    }
    Vocab = vocabs.front();
}

std::string Tokenizer::get_name() {
    return TokenizerName;
}
