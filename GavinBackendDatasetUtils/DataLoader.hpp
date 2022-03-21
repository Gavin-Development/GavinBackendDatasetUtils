#pragma once

#ifdef _WIN32
#include <corecrt.h>
#endif

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <future>
#include <stdio.h>
#include <cstdlib>
#include <stdexcept>
#include <chrono>
#include <mutex>
#include <tuple>
#include <vector>
#include <regex>

#if defined(ONEAPI_ROOT)
#include <CL/sycl.hpp>
#endif

//#include "base64.hpp"

#define BIN_FILE_DTYPE_INT16  (uint8_t)1
#define BIN_FILE_DTYPE_INT32  (uint8_t)0
#define BIN_FILE_DTYPE_INT24  (uint8_t)2

#define UINT24_MAX (uint32_t)16777215



// Thanks to https://stackoverflow.com/questions/2682725/int24-24-bit-integral-datatype
// Used their code as base for this class as i cba to spend all my time implementing uint24_t from scratch.

class uint24_t
{
protected:
    unsigned char m_Internal[3];
public:
    uint24_t()
    {
    }

    uint24_t(const int val)
    {
        *this = val;
    }

    uint24_t(const uint24_t& val)
    {
        *this = val;
    }

    operator int() const
    {
        return (m_Internal[2] << 16) | (m_Internal[1] << 8) | (m_Internal[0] << 0);
    }

    operator float() const
    {
        return (float)this->operator int();
    }

    uint24_t& operator =(const uint24_t& input)
    {
        m_Internal[0] = input.m_Internal[0];
        m_Internal[1] = input.m_Internal[1];
        m_Internal[2] = input.m_Internal[2];

        return *this;
    }

    uint24_t& operator =(const int input)
    {
        m_Internal[0] = ((unsigned char*)&input)[0];
        m_Internal[1] = ((unsigned char*)&input)[1];
        m_Internal[2] = ((unsigned char*)&input)[2];

        return *this;
    }

    /***********************************************/

    uint24_t operator +(const uint24_t& val) const
    {
        return uint24_t((int)*this + (int)val);
    }

    uint24_t operator -(const uint24_t& val) const
    {
        return uint24_t((int)*this - (int)val);
    }

    uint24_t operator *(const uint24_t& val) const
    {
        return uint24_t((int)*this * (int)val);
    }

    uint24_t operator /(const uint24_t& val) const
    {
        return uint24_t((int)*this / (int)val);
    }

    /***********************************************/

    uint24_t operator +(const int val) const
    {
        return uint24_t((int)*this + val);
    }

    uint24_t operator -(const int val) const
    {
        return uint24_t((int)*this - val);
    }

    uint24_t operator *(const int val) const
    {
        return uint24_t((int)*this * val);
    }

    uint24_t operator /(const int val) const
    {
        return uint24_t((int)*this / val);
    }

    /***********************************************/
    /***********************************************/


    uint24_t& operator +=(const uint24_t& val)
    {
        *this = *this + val;
        return *this;
    }

    uint24_t& operator -=(const uint24_t& val)
    {
        *this = *this - val;
        return *this;
    }

    uint24_t& operator *=(const uint24_t& val)
    {
        *this = *this * val;
        return *this;
    }

    uint24_t& operator /=(const uint24_t& val)
    {
        *this = *this / val;
        return *this;
    }

    /***********************************************/

    uint24_t& operator +=(const int val)
    {
        *this = *this + val;
        return *this;
    }

    uint24_t& operator -=(const int val)
    {
        *this = *this - val;
        return *this;
    }
    uint24_t& operator *=(const int val)
    {
        *this = *this * val;
        return *this;
    }

    uint24_t& operator /=(const int val)
    {
        *this = *this / val;
        return *this;
    }

    /***********************************************/
    /***********************************************/

    uint24_t operator >>(const int val) const
    {
        return uint24_t((int)*this >> val);
    }

    uint24_t operator <<(const int val) const
    {
        return uint24_t((int)*this << val);
    }

    /***********************************************/

    uint24_t& operator >>=(const int val)
    {
        *this = *this >> val;
        return *this;
    }

    uint24_t& operator <<=(const int val)
    {
        *this = *this << val;
        return *this;
    }

    /***********************************************/
    /***********************************************/

    operator bool() const
    {
        return (int)*this != 0;
    }

    bool operator !() const
    {
        return !((int)*this);
    }

    uint24_t operator -()
    {
        return uint24_t(-(int)*this);
    }

    /***********************************************/
    /***********************************************/

    bool operator ==(const uint24_t& val) const
    {
        return (int)*this == (int)val;
    }

    bool operator !=(const uint24_t& val) const
    {
        return (int)*this != (int)val;
    }

    bool operator >=(const uint24_t& val) const
    {
        return (int)*this >= (int)val;
    }

    bool operator <=(const uint24_t& val) const
    {
        return (int)*this <= (int)val;
    }

    bool operator >(const uint24_t& val) const
    {
        return (int)*this > (int)val;
    }

    bool operator <(const uint24_t& val) const
    {
        return (int)*this < (int)val;
    }

    /***********************************************/

    bool operator ==(const int val) const
    {
        return (int)*this == val;
    }

    bool operator !=(const int val) const
    {
        return (int)*this != val;
    }

    bool operator >=(const int val) const
    {
        return (int)*this >= val;
    }

    bool operator <=(const int val) const
    {
        return (int)*this <= val;
    }

    bool operator >(const int val) const
    {
        return ((int)*this) > val;
    }

    bool operator <(const int val) const
    {
        return (int)*this < val;
    }

    /***********************************************/
    /***********************************************/
};

namespace py = pybind11;

// Legacy

std::vector<py::list> LoadTrainDataST_Legacy(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);

//future

py::array_t<int> LoadTrainDataST(uint64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);
py::array_t<int> LoadTrainDataMT(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);

void ConvertToBinFormat(int64_t samplesToRead, std::string fileToLoad, std::string fileToSave);


namespace BIN {
	struct SampleHeaderData {
		uint64_t OffsetFromDataSectionStart;
		uint16_t SampleLength;
		uint8_t dtypeint16;
	};

};

class DataGenerator {
public:
    std::ifstream ToFile, FromFile;
    uint64_t NumberOfSamplesInFile, ToFileLength, FromFileLength, BufferSize, FileHeaderLength, CurrentSampleNumber;
    int* ToSampleBuffer,* FromSampleBuffer;
    int startToken, endToken, sampleLength, paddingValue;

    py::capsule ToSampleBufferCapsule, FromSampleBufferCapsule;
    py::array_t<int> ToSampleBufferArray_t, FromSampleBufferArray_t;


    DataGenerator(std::string dataPath, std::string tokenizertoName, std::string tokenizerfromName, uint64_t iBufferSize, int istartToken, int iendToken, int isampleLength, int ipaddingValue);
    ~DataGenerator();

    void UpdateDataBuffer();

private:

    int* Buffer_int32;
    uint24_t* Buffer_int24;
    uint16_t* Buffer_int16;

    void ReadSampleFromFile(std::ifstream* File, BIN::SampleHeaderData HeaderData, int* BufferToLoadTo);

};


enum token_type {
    TT_CHAR = 0,
    TT_UID = 1,
};

typedef struct {
    token_type type;
    union {
        char letter;
        int uid;
    };
} uid_letter_token;


class Tokenizer {
public:

    Tokenizer(std::string iTokenizerName, int iVocabSize);  // iVocabSize is the maximum vocab size
    Tokenizer(std::string iTokenizerPath);  // Loads tokenizer from file

    void build_vocab(std::vector<std::string> corpus);

    std::list<uint64_t> encode(std::string text);
    std::string decode(std::list<uint64_t> encoded_text);

    std::list<std::list<uint64_t>> encode_batch(std::list<std::string> texts);
    std::list<std::string> decode_batch(std::list<std::list<uint64_t>> encoded);

    std::map<int, std::string> get_vocab();
    uint64_t get_vocab_size();
    std::string get_name();



    //void SaveTokenizer();
    //void LoadTokenizer();
private:
    std::string END_OF_WORD = "</w>";
    std::string TokenizerName;
    std::vector<std::string> reversed_tokens = {END_OF_WORD};
    std::map<int, std::string> Vocab = {{0, END_OF_WORD}};
    int MaxVocabSize;

    static std::string _remove_object_special_chars(const std::string& text);
    static std::list<uint64_t> _pad_incr(const std::list<uint64_t>& encoded);
    static std::list<uint64_t> _pad_decr(const std::list<uint64_t>& encoded);
    std::vector<uint64_t> _to_bytes(const std::string& text);
    std::string _from_bytes(const std::vector<uint64_t>& bytes);
    std::string _from_bytes(const uint64_t& bytes);

    static std::vector<std::vector<std::string>> chunk_data(std::vector<std::string> data, int number_of_chunks);
    static std::vector<std::string> _split_sentence_and_append_eow(const std::string &delimiter, std::string sentence,
                                                                   const std::string &eow);
    static std::vector<std::string> _split_sentences(const std::string &delimiter, std::vector<std::string> sentences,
                                                     const std::string &eow);
    static std::vector<std::vector<uid_letter_token>> _split_sentences_token(const std::string &delimiter, std::vector<std::string> sentences,
                                                                       const std::string &eow);

    static std::map<int, std::string> merge(std::map<int, std::string> vok1, std::map<int, std::string> vok2);
    static std::map<int, std::string> _build_vocab_for_string(std::vector<std::string> sentences,
                                                              std::string end_of_word, uint64_t max_vocab_size);
};

class TokenSequence {
private:
    std::vector<uid_letter_token> tokens;
public:
    TokenSequence();
    explicit TokenSequence(std::vector<uid_letter_token> tokens);
    explicit TokenSequence(const std::string& text);
    ~TokenSequence();

    [[nodiscard]] std::size_t size() const { return tokens.size(); };
    std::size_t find(const char &c);

    uid_letter_token &operator[](std::size_t idx) { return tokens[idx]; };

    const uid_letter_token &operator[](std::size_t idx) const { return tokens[idx]; };

    void push_back(const uid_letter_token &t) { tokens.push_back(t); };

    void push_back(const char &c) { tokens.push_back({.type=TT_CHAR, .letter=c}); };

    void push_back(const int &uid) { tokens.push_back({.type=TT_UID, .uid=uid}); };

    void clear() { tokens.clear(); };

    void erase(std::size_t idx) { tokens.erase(tokens.begin() + (int)idx); };

    void replace_with_uid(TokenSequence &replace_chars, int uid);
};
