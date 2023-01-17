# CPP-DataSetManager
Data Loader for Gavin written in C++

This tool set is a WIP suite of tools for authoring, managing & loading datasets for Gavin bot.

This toolset is built using assistance from Intel, and thus is only optimised and guaranteed to work on Intel based systems, it may work on AMD based systems but NO validation is being done. This module is only validated to run on Intel CPUs / GPUs and Nvidia GPUs.

## 3rd party code used

* https://github.com/nlohmann/json
* https://github.com/pybind/pybind11

## Usage

### BIN File Format
The default dataset / file format is .BIN, it is designed to be highly compressable and efficient to load. The format is mostly complete but additions and tweaks are still being made.

|Section|What it is|Length|
|------|------|-----------|
|FileHeaderSectionLength|Uint64_t which contains info on the length of the header section (Header section length + 8 for the bytes to store length).| 8 |
|NumberOfSamplesInFile|Uint64_t which contains info on the number of samples in the file| 8 |
|Header Section| Contains header info on each of the samples in the file| variable |
|Data Section| Contains the data in mixed precision that the headers reference| variable |

### BINFile class
This is to replace the generator and enable extended functionality such as array syntax overloading, array slicing and full memory management for loaded data. This object can be treated very much like an array but the data backing it is actually stored on disk and streamed in and decompressed upon user request for data. Current version only has full support for reading existing files with full operator overloading and slicing capabilities via the `get_slice(Start, End)` method.

**NOTE** This is the preferred method for reading from BIN files now as all other methods are now not being updated and should be considered depreciated.

**NOTE** Support for modifying BIN files with this class is a thing and code samples are on their way soonish.

Please check BINFile_Samples.md in /Samples to see usage.

### Tokenizer class
This is to replace the TF BPE tokenizer, it is a very simple WordPiece implimentation that is not particularly tuned for speed nor efficieny yet. The class has been re designed from the ground up to be more pythonic and robust for usage in python. It supports JSON format for saving its information to disc allowing interoperability with other implimentations with minimal translation of data. 

**NOTE** This is class is still in its first iteration and may have bugs, just open an issue or DM me if it does not work.

Please check Tokenizer_Samples.md in /Samples to see usage.

### DOP functions
This is the LoadTrainDataST() function:
```python
samples = LTD.LoadTrainDataST(10000000, "C:/Users/User/Desktop/Gavin/GavinTraining/", "Tokenizer-3.to.BIN", 69108,66109, 52, 0)
```
This will load 10,000,000 samples from the specified file in the specified directory. It will add start token 69108 and end token 66109 and pad each sample to length 52 with 0s.

This is the LoadTrainDataMT() function:
```python
samples = LTD.LoadTrainDataMT(10000000, "C:/Users/User/Desktop/Gavin/GavinTraining/", "Tokenizer-3.to.BIN", 69108,66109, 52, 0)
```
This function takes inputs in exactly the same way as LoadTrainDataST() but instead has a differing internal mechanism to be more memory efficient and utilise multiple cores to speed up loading of the dataset. It will not always return the full number of samples if it is now divisible by the number of threads avalible without a remainder. 

**NOTE** This is the most performant function for loading the dataset but offers minimal flexibility and high RAM usage for large datasets.

This is the ConvertDataSet() function:
```python
LTD.ConvertDataSet_TEST(10_000_000,"C:/Users/user/Desktop/Gavin/GavinTraining/Tokenizer-3.from", "./10_MILLION.BIN")
```
This function will load up the Tokenizer-3.from file, load it and transcode it to BIN format and save it to 10_MILLION.BIN, it will load the first 10 million samples.
It has minimal optimisation as its only meant to be a temporary measure for compatibility as we transition to BIN format.

## Progress / Roadmap

## To Do
* Create a directory containing samples to guide the use of the tools provided.
* Optimise Tokenizer class loading and saving functions for potentially better performance.
* Complete overhaul of BINFile class to fix numerous issues with memory and 1 or 2 performance.
* Impliment GPU acceleration for WordPiece Tokenizer.

## Known issues
None