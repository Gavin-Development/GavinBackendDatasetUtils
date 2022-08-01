#pragma once

#include <corecrt.h>

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <stdio.h>
#include <cstdlib>
#include <stdexcept>
#include <chrono>
#include <tuple>
#include <vector>
#include <math.h>



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

//future

py::array_t<int> LoadTrainDataST(uint64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);
py::array_t<int> LoadTrainDataMT(int64_t samplesToRead, std::string dataPath, std::string tokenizerName, int startToken, int endToken, int sampleLength, int paddingValue);



namespace BIN {
	struct SampleHeaderData {
        // Offset from data section start in bytes.
		uint64_t OffsetFromDataSectionStart;
        // Sample length in bytes.
		uint16_t SampleLength;
        // flag for D-type.
		uint8_t dtypeint16;

        //SampleHeaderData(int a, int b, int c) : OffsetFromDataSectionStart(a), SampleLength(b), dtypeint16(c) {};
	};
};



class BINFile {
public:
    uint64_t NumberOfSamplesInFile, MaxNumberOfSamples;
    int startToken, endToken, sampleLength, paddingVal;

    BINFile(std::string dataPath, int startToken, int endToken, int sampleLength, int paddingVal);
    BINFile(std::string dataPath, uint64_t numberOfSamples, int startToken, int endToken, int sampleLength, int paddingVal);
    ~BINFile();



    // Writes the data at that index to the file.
    bool append(py::array_t<int> Data);

    // read the data at indices / slice from the file.

    py::array_t<int> get_slice(uint64_t StartIndex, uint64_t EndIndex);


    // Operator overloads

    // When array index syntax is called with 1 value for index.
    py::array_t<int> operator [](uint64_t Index);


private:
    uint64_t _HeaderSectionLength, _DataSectionPosition, _FileLength, _MaxNumSamples;
    std::fstream _File;
    BIN::SampleHeaderData* _pSampleHeaderData;
    //bool _WriteOnlyMode;


    int* _pBuffer_int32;
    uint24_t* _pBuffer_int24;
    uint16_t* _pBuffer_int16;

    bool _createnewfile(std::string& pDataPath);
    
    // Reads sample from index. No error checking is done in this function.
    inline int* _readsample(uint64_t Index);

    // Writes sample to given Index. No error checking is done in this function.
    inline void _writesample(uint64_t Index, py::array_t<int> Arr);
};

class Tokenizer {
public:
    std::string TokenizerName;
    std::vector<std::string> Encodings;
    std::vector<uint64_t> Commonalities;

    Tokenizer(std::string iTokenizerName);
    Tokenizer();

    // Get Vocab Size function.
    int GetVocabSize() { return Encodings.size(); };

    // Build Encodes Functions.
    void BuildEncodes(std::vector<std::string> Samples);
    void BuildEncodes_GPU(std::vector<std::string> Samples);

    // Encode Strings Functions.
    //std::vector<int> Encode_GPU(std::vector<std::string> Samples); // Needs re write.
    std::vector<std::vector<int>> Encode(std::vector<std::string> Samples);

    // Decode Strings Functions
    std::string Decode(std::vector<int> Samples); // Needs re write.
    
    // Tokenizer load and save functions.

    bool SaveTokenizer();
    bool SaveTokenizer(std::string iTokenizerName);

    bool LoadTokenizer();
    bool LoadTokenizer(std::string iTokenizerName);

private:
    struct _Encode {
        std::string Encode;
        uint64_t Index;

        _Encode(std::string str, uint64_t val) { Encode = str; Index = val; }
    };
    // Sort the Encodings & Commonalities vectors to be in the order of descending commonality.
    void _SortEncodings();

    // Save and load functions
    bool _SaveTokenizer();
    bool _LoadTokenizer();


    //bool _CheckMGPUCapability(std::vector<sycl::device>* pGPUs);

};
