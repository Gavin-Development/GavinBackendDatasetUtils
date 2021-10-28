#pragma once
#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <stdio.h>
#include <cstdlib>
#include <stdexcept>
#include <chrono>
#include <mutex>
#include <tuple>
#include <vector>


#include "base64.hpp"

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
		uint64_t alignas(uint64_t) OffsetFromDataSectionStart;
		uint16_t alignas(uint16_t) SampleLength;
		uint8_t alignas(uint8_t) dtypeint16;
	};
};

class DataGenerator {
    std::ifstream File;
    uint64_t NumberOfSamplesInFile;
    int* SampleBuffer;


    DataGenerator();
    ~DataGenerator();

    void UpdateDataBuffer();
};