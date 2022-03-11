#include <utility>

#include "DataLoader.hpp"


// Helper function for getting largest key
template <typename T>
inline const typename T::key_type& last_key(const T& pMap)
{
    return pMap.rbegin()->first;
}

// Helper function for getting a key associated with value
template <typename T>
inline const typename T::key_type& key_of_value(const T& pMap, const typename T::mapped_type& pValue)
{
    for (const auto& pPair : pMap)
    {
        if (pPair.second == pValue)
        {
            return pPair.first;
        }
    }
}


Tokenizer::Tokenizer(std::string iTokenizerName, int iVocabSize) {
	std::cout << "Initialising a new tokenizer." << std::endl;
	TokenizerName = std::move(iTokenizerName);
	MaxVocabSize = iVocabSize;
};

Tokenizer::Tokenizer(std::string iTokenizerPath) {
	std::cout << "Loading existing tokenizer." << std::endl;
};


std::list<unsigned long long int> Tokenizer::_pad_incr(const std::list<unsigned long long>& encoded) {
    std::list<unsigned long long int> out;
    for (auto& item: encoded) {out.push_back(item+1);}
    return out;
}

std::list<unsigned long long int> Tokenizer::_pad_decr(const std::list<unsigned long long>& encoded) {
    std::list<unsigned long long int> out;
    for (auto& item: encoded) {out.push_back(item-1);}
    return out;
}


std::vector<unsigned long long int> Tokenizer::_to_bytes(const std::string& text) {
    unsigned long long int offset = Vocab.size();
    std::vector<unsigned long long int> out;
    for (char const& c: text) {
        out.push_back(c+offset);
    }
    return out;
}


std::vector<std::string> Tokenizer::_split_sentence_and_append_eow(const std::string &delimiter, std::string sentence,
                                                                   const std::string &eow) {
    std::size_t pos = 0;
    std::string token;
    std::vector<std::string> values;
    while ((pos = sentence.find(delimiter)) != std::string::npos) {
        token = sentence.substr(0, pos);
        if (!token.empty() && pos < sentence.length() - 1) {
            token += eow;
            values.push_back(token);
            sentence.erase(0, pos + delimiter.length());
        }
        else {
            break;
        }
    }
    values.push_back(sentence);
    values[values.size() - 1] += eow;
    return values;
}

std::vector<std::string> Tokenizer::_split_sentences(const std::string &delimiter,
                                                     std::vector<std::string> sentences,
                                                     const std::string &eow) {
    std::string token;
    std::vector<std::vector<std::string>> values;
    for (auto sentence : sentences) {
        if (!sentence.empty() && sentence.length() > 0 && sentence != " ") {
            values.push_back(_split_sentence_and_append_eow(delimiter, sentence, eow));
        }
    }
    std::vector<std::string> result;
    for (const auto& sentence : values) {
        for (const auto& word : sentence) {
            result.push_back(word);
        }
    }
    return result;
}


std::map<int, std::string> Tokenizer::_build_vocab_for_string(std::vector<std::string> sentences,
                                                              std::string end_of_word, uint64_t max_vocab_size) {
    std::vector<std::string> words = _split_sentences(" ", std::move(sentences), end_of_word);
    std::map<int, std::string> vocab = {{0, end_of_word}};
    int uid = last_key(vocab);
    while (true) {
        std::map<std::tuple<std::string, std::string>, int> pairs;
        int best_freq = 0;
        std::tuple<std::string, std::string> best_pair = std::make_tuple("", "");
        for (const auto& word: words) {
            for (int i =0; i<word.size()-1; i++) {
                if (!word.empty()) {
                    std::tuple<std::string, std::string> pair = std::make_tuple(word.substr(i), word.substr(i + 1));
                    int count = pairs[pair] + 1;
                    pairs[pair] = count;
                    if (count > best_freq) {
                        best_freq = count;
                        best_pair = pair;
                    }
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


std::list<unsigned long long int> Tokenizer::encode(std::string text) {
    std::vector<std::string> words = _split_sentence_and_append_eow(" ", std::move(text),
                                                                        END_OF_WORD);
    std::vector<std::vector<unsigned long long int>> encoded_words;
    // First pass, handle all in-vocabulary byte pairs.
    for (const auto& word: words) {
        std::vector<unsigned long long int> encoded_word;
        bool break_loop = false;
        for (int i = 0; i < (word.size() - 1)/2; i++) {
            std::size_t pos = 0;
            std::string byte_pair = word.substr(i*2, 2);
            if ((pos = byte_pair.find('<')) != std::string::npos) {
                pos = word.find('>');
                byte_pair += word.substr(i*2 + 2, pos - i*2 - 1);
                break_loop = true;
            }
            bool oov = false;
            for (const auto& pair: Vocab) {
                if (pair.second == byte_pair) {
                    encoded_word.push_back(pair.first);
                    oov = true;
                    break;
                }
            }
            if (!oov) {
                std::vector<unsigned long long int> byte_pair_byte = _to_bytes(byte_pair);
                encoded_word.push_back(byte_pair_byte[0]);
                encoded_word.push_back(byte_pair_byte[1]);
            }
            if (break_loop) {
                break;
            }
        }
        encoded_words.push_back(encoded_word);
    }
    // Convert 2D vector to 1D list.
    std::list<unsigned long long int> encoded_text;
    for (const auto& encoded_word: encoded_words) {
        for (const auto& byte: encoded_word) {
            encoded_text.push_back(byte);
        }
    }
    return _pad_incr(encoded_text);
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


std::vector<std::vector<std::string>> Tokenizer::chunk_data(std::vector<std::string> data, int number_of_chunks) {
    std::vector<std::vector<std::string>> result;
    int size = floor(data.size()/number_of_chunks);
    for (int i=0; i < number_of_chunks; i++) {
        unsigned long int start = i*size;
        unsigned long int end = (i+1)*size;
        if (end > data.size()) {
            end = data.size();
        }
        else if (start > data.size()) {
            break;
        }
        auto start_it = data.begin();
        std::advance(start_it, start);
        auto end_it = data.begin();
        std::advance(end_it, end);
        std::vector<std::string> chunk = std::vector<std::string>(start_it, end_it);
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


void Tokenizer::build_vocab(std::vector<std::string> corpus) {
    unsigned int CHUNK_SIZE = std::thread::hardware_concurrency();
    if (CHUNK_SIZE == 0) {
        CHUNK_SIZE = 1;
    }
    else if (CHUNK_SIZE % 2 != 0 && CHUNK_SIZE > 1) {
        CHUNK_SIZE--;
    }
    if (CHUNK_SIZE > 1) {
        std::vector<std::vector<std::string>> chunks = chunk_data(corpus, (int) CHUNK_SIZE);
        std::vector<std::future<std::map<int, std::string>>> futures;
        for (auto chunk: chunks) {
            std::future future = std::async([this, chunk]() {

                return _build_vocab_for_string(chunk, END_OF_WORD, MaxVocabSize);
            });
            futures.push_back(std::move(future));
        }
        std::vector<std::map<int, std::string>> vocabs;
        for (auto &future: futures) {
            vocabs.push_back(future.get());
        }
        bool done = false;
        while (!done) {
            futures.clear();
            CHUNK_SIZE = ceil(CHUNK_SIZE/2);
            if (CHUNK_SIZE == 0) {
                CHUNK_SIZE = 1;
            }
            else if (CHUNK_SIZE % 2 != 0 && CHUNK_SIZE > 1) {
                CHUNK_SIZE--;
            }
            int process_max = ceil(vocabs.size()/2);
            if (process_max == 0) {
                Vocab = vocabs[0];
                done = true;
            } else {
                for (int i=0; i<process_max; i++) {
                    std::future future = std::async([vocabs, i]() {
                        return merge(vocabs[i], vocabs[i+1]);
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
        }
        Vocab = vocabs.front();
    }
    else {
        Vocab = _build_vocab_for_string(corpus, END_OF_WORD, MaxVocabSize);
    }
}

std::string Tokenizer::get_name() {
    return TokenizerName;
}
