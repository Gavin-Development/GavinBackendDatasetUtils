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


std::list<uint64_t> Tokenizer::_pad_incr(const std::list<uint64_t>& encoded) {
    std::list<uint64_t> out;
    for (auto& item: encoded) {out.push_back(item+1);}
    return out;
}

std::list<uint64_t> Tokenizer::_pad_decr(const std::list<uint64_t>& encoded) {
    std::list<uint64_t> out;
    for (auto& item: encoded) {out.push_back(item-1);}
    return out;
}


std::vector<uint64_t> Tokenizer::_to_bytes(const std::string& text) {
    uint64_t offset = Vocab.size();
    std::vector<uint64_t> out;
    for (char const& c: text) {
        out.push_back(c+offset);
    }
    return out;
}


std::string Tokenizer::_from_bytes(const std::vector<uint64_t>& bytes) {
    std::string out;
    for (auto& byte: bytes) {
        out.push_back(byte-Vocab.size());
    }
    return out;
}

std::string Tokenizer::_from_bytes(const uint64_t& byte) {
    std::string out;
    out.push_back(byte-Vocab.size());
    return out;
}

std::string Tokenizer::_remove_object_special_chars(const std::string &text) {
    std::regex regexp("[`]");
    return std::regex_replace(text, regexp, "");
}

std::vector<std::string> Tokenizer::_split_sentence_and_append_eow(const std::string &delimiter, std::string sentence,
                                                                   const std::string &eow) {
    std::size_t pos = 0;
    std::string token;
    std::vector<std::string> values;
    while ((pos = sentence.find(delimiter)) != std::string::npos) {
        token = sentence.substr(0, pos);
        if (!token.empty() && pos < sentence.length() - 1) {
            token = _remove_object_special_chars(token);
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


std::vector<TokenSequence> Tokenizer::_split_sentences_token(const std::string &delimiter,
                                                                             std::vector<std::string> sentences,
                                                                             const std::string &eow) {
    std::string token;
    std::vector<TokenSequence> values;
    for (auto sentence : sentences) {
        if (!sentence.empty() && sentence.length() > 0 && sentence != " ") {
            std::vector<std::string> words_from_sentence = _split_sentence_and_append_eow(delimiter, sentence, eow);
            for (auto word : words_from_sentence) {
                auto token_sequence_from_word = TokenSequence(word);
                values.push_back(token_sequence_from_word);
            }
        }
    }
    return values;
}


std::map<int, std::string> Tokenizer::_build_vocab_for_string(std::vector<std::string> sentences,
                                                              std::string end_of_word, uint64_t max_vocab_size) {
    std::vector<TokenSequence> words = _split_sentences_token(" ", std::move(sentences), end_of_word);
    std::map<int, std::string> vocab = {{0, end_of_word}};
    int uid = last_key(vocab);
    while (true) {
        std::map<std::string, int> pairs;
        for (TokenSequence word: words) {
            for (int i = 0; i < (word.size() - 1)/2; i++) {
                if (!word.empty()) {
                    auto pair = TokenSequence(std::string() + word[i].letter + word[i+1].letter);
                    if (pair.find('<') != pair.size()|| pair.find('>') != pair.size() || pair.find('/') != pair.size()) {
                        std::size_t start_pos = 0;
                        for (int j = 0; j < word.size(); j++) {
                            if (word[j].letter == '<') {
                                start_pos = j;
                                break;
                            }
                        }
                        std::string test_pair = std::string() + word[start_pos].letter;
                        for (std::size_t t=start_pos+1; t < word.size(); t++) {
                            if (word[t].letter == '>') {
                                test_pair += word[t].letter;
                                break;
                            }
                            else {
                                test_pair += word[t].letter;
                            }
                        }
                        if (test_pair == end_of_word) {
                            pair = test_pair;
                        }
                    }
                    int count = pairs[pair] + 1;
                    pairs[pair] = count;
                }
                else {
                    break;
                }
            }
        }
        auto pr = std::max_element(pairs.begin(), pairs.end(),
                                   [] (const std::pair<std::string, int>& p1, const std::pair<std::string, int>& p2)
                                   {return p1.second < p2.second;});
        std::string best_pair = pr->first;
        if (best_pair.empty()) {
            break;
        }
        uid++;
        if (best_pair != end_of_word) {
            vocab[uid] = best_pair;
        } else {
            vocab[uid] = end_of_word;
        }
        std::vector<uid_letter_token> token_best_pair;
        std::vector<std::vector<uid_letter_token>> new_words;
        bool is_same_pair = true;
        for (auto letter: best_pair) {
            uid_letter_token token_from_letter;
            token_from_letter.type = TT_CHAR;
            token_from_letter.uid = -1;
            token_from_letter.letter = letter;
            token_best_pair.push_back(token_from_letter);
        }
        for (auto word: words) {
            if (word.size() == token_best_pair.size()) {
                for (int i = 0; i < word.size(); i++) {
                    if (((word[i].letter != token_best_pair[i].letter) ||
                        (word[i].type != token_best_pair[i].type)) && word[i].type == TT_CHAR) {
                        is_same_pair = false;
                        break;
                    }
                }
            }
            if (!is_same_pair) {
                word = std::vector<uid_letter_token>();
                new_words.push_back(word);
                continue;
            }else {
                std::size_t start_pos = -1;
                for (int i = 0; i < word.size(); i++) {
                    if (word[i].letter == token_best_pair[0].letter) {
                        start_pos = i;
                        break;
                    }
                }
                if (start_pos != -1) {
                    // build string from
                    std::string test_word = std::string();
                    for (auto & i : word) {
                        if (i.type == TT_CHAR) {
                            test_word += i.letter;
                        }
                        else {
                            test_word += std::to_string(i.uid);
                        }
                    }
                    if (test_word == best_pair) {
                        new_words.push_back(word);
                    }
                    else {
                        std::size_t first_letter_pos = -1;
                        std::size_t last_letter_pos = -1;
                        std::vector<uid_letter_token> new_word = word;
                        while (((first_letter_pos = test_word.find(token_best_pair[0].letter)) != std::string::npos)
                                && ((last_letter_pos = test_word.find(token_best_pair[token_best_pair.size()-1].letter)) != std::string::npos)) {
                            if (first_letter_pos != last_letter_pos && first_letter_pos >= 0
                                && last_letter_pos >= 0) {
                                auto length = (last_letter_pos - first_letter_pos)+1;
                                auto token = uid_letter_token{.type=TT_UID};
                                token.uid = uid;
                                for (std::size_t i = 0; i < length; i++) {
                                    new_word.erase(new_word.begin() + (int)first_letter_pos);
                                }
                                new_word.insert(new_word.begin() + (int)first_letter_pos, token);
                                new_words.push_back(new_word);
                            }
                        }
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


std::list<uint64_t> Tokenizer::encode(std::string text) {
    // Complexity Of this function is O(n+m) where n is the length of the text and m is the size of the vocabulary
    std::vector<std::string> words = _split_sentence_and_append_eow(" ", std::move(text),
                                                                        END_OF_WORD);
    std::vector<std::vector<uint64_t>> encoded_words;
    // First pass, handle all in-vocabulary byte pairs.
    for (const auto& word: words) {
        std::vector<uint64_t> encoded_word;
        bool break_loop = false;
        for (int i = 0; i < (word.size() - 1)/2; i++) {
            std::size_t pos = 0;
            std::string byte_pair = word.substr(i*2, 2);
            if ((pos = byte_pair.find('<')) != std::string::npos) {
                pos = word.find('>');
                byte_pair += word.substr(i*2 + 2, pos - i*2 - 1);
                break_loop = true;
            }
            bool oov = true;
            for (const auto& pair: Vocab) {
                if (pair.second == byte_pair) {
                    encoded_word.push_back(pair.first);
                    oov = false;
                    break;
                }
            }
            if (oov) {
                if (break_loop) {
                    std::vector<uint64_t> byte_char = _to_bytes(byte_pair.substr(0, 1));
                    encoded_word.push_back(byte_char[0]);
                    encoded_word.push_back(key_of_value(Vocab, END_OF_WORD));
                }
                else {
                    std::vector<uint64_t> byte_pair_byte = _to_bytes(byte_pair);
                    encoded_word.push_back(byte_pair_byte[0]);
                    encoded_word.push_back(byte_pair_byte[1]);
                }
            }
            if (break_loop) {
                break;
            }
        }
        encoded_words.push_back(encoded_word);
    }
    // Convert 2D vector to 1D list.
    std::list<uint64_t> encoded_text;
    for (const auto& encoded_word: encoded_words) {
        for (const auto& byte: encoded_word) {
            encoded_text.push_back(byte);
        }
    }
    return _pad_incr(encoded_text);
}


std::string Tokenizer::decode(std::list<uint64_t> encoded_text) {
    // Complexity of this function is O(n) where n is the size of the encoded text.
    std::string decoded_text;
    encoded_text = _pad_decr(encoded_text); // Decrement the tokens to get the original values.
    for (auto token: encoded_text) {
        if (Vocab.find(token) != Vocab.end()) {
            decoded_text += Vocab[token];
        }
        else {
            std::string byte_char = _from_bytes(token);
            decoded_text += byte_char;
        }
    }
    std::size_t pos;
    while ((pos = decoded_text.find(END_OF_WORD)) != std::string::npos) {
        decoded_text.replace(pos, END_OF_WORD.size(), " ");
    }
    decoded_text = decoded_text.substr(0, decoded_text.size() - 1);
    return decoded_text;
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
    // unsigned int CHUNK_SIZE = std::thread::hardware_concurrency();
    unsigned int CHUNK_SIZE = 1; // For testing
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
