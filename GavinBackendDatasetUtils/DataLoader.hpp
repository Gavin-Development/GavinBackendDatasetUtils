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

class Tokenizer {
public:

    Tokenizer(std::string iTokenizerName, uint64_t iVocabSize);  // iVocabSize is the maximum vocab size
    Tokenizer(std::string iTokenizerPath);  // Loads tokenizer from file

    void build_vocab(const std::list<std::string>& corpus);

    py::array_t<int> encode(std::string text);
    std::string decode(py::array_t<int> encoded);

    py::array_t<int> encode_batch(std::list<std::string> texts);
    std::list<std::string> decode_batch(py::array_t<int> encoded);

    std::map<int, std::string> get_vocab();
    uint64_t get_vocab_size();
    std::string get_name();



    //void SaveTokenizer();
    //void LoadTokenizer();
private:
    std::string END_OF_WORD = "</w>";
    std::string TokenizerName;
    std::map<int, std::string> Vocab = {{0, END_OF_WORD}};
    uint64_t MaxVocabSize;

    static std::list<std::list<std::string>> chunk_data(const std::list<std::string> &data, int chunk_size);
    static std::vector<std::map<int, std::string>> chunk_vocab(const std::list<std::map<int, std::string>> &data, int chunk_size);
    static std::vector<std::string> _split_sentence(const std::string &delimiter, std::string sentence);
    static std::vector<std::string> _split_sentences(const std::string &delimiter, const std::vector<std::string>& sentences);

    static std::map<int, std::string> merge(std::map<int, std::string> vok1, std::map<int, std::string> vok2);
    static std::map<int, std::string> _build_vocab_for_string(const std::vector<std::string>& sentences,
                                                              std::string end_of_word, uint64_t max_vocab_size);
};