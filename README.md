# CPP-DataSetManager
Data Loader for Gavin written in C++

This tool set is a WIP suite of tools for authoring, managing & loading datasets for Gavin bot.

# Current build is failing on Release mode. Please build in debug mode for it to work for testing.

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

```
Samples = LTD.BINFile("Tokenizer-3-to.BIN",69,420,50,0)

Sample = Samples[0]
```
This will setup the BINFile class and point it to a file with start token set to 69, end token to 420, desired sample length set to 50, and padding val set to 0. As can be seen in the code snippet it supports array index operator overloading to stream in from the disk.

```
Data_Slice = Samples.get_slice(0,2)
```
This will get the first 2 samples from the BIN file and return them as a 2D array of size (slize_size, sample_length).


**NOTE** Support for modifying BIN files with this class is a thing and code samples are on their way soonish.

### Tokenizer class
This is to replace the TF BPE tokenizer, it is a very simple BPE algo tokenizer that is in its first stages of deployment and development, it can utilise both CPU & GPU to build the vocab with the ability to progressively build the vocab by batching the corpus.
So far this class supports GPU accelerated encode building, CPU only for encode and decode of strings, support for unknown encodes, support for saving and loading Tokenizers from local path, some performance optimisations.

**NOTE** This is class is still in its first iteration and may have bugs, just open an issue or DM me if it does not work.

```
GPUTokenizer = LTD.Tokenizer("Sus")

GPUTokenizer.BuildEncodes_GPU(The_Bible)

for i in range(0,10):
	print(f"{GPUTokenizer.Words[i]}  {GPUTokenizer.Occurrences[i]}")

GPUTokenizer.Save()
```
This sample creates a tokenizer called Sus and then builds the vocab on the GPU passing a list of words split by spaces as the corpus. Then it prints out the first 10 encodes and their associated commonalities, then saves the tokenizer to local dir.


```
ATokenizer = LTD.Tokenizer("Sus")

ATokenizer.Load()

for i in range(0,10):
	print(f"{ATokenizer.Words[i]}  {ATokenizer.Occurrences[i]}")
```
This sample creates the Tokenizer class, then loads the file named "Sus.TOKENIZER" from the local dir then proceeds to print out the top 10 most common encodes.

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
* Add functionality to data streaming class to improve usability and performance while adding more functionality.
* Create a directory containing samples to guide the use of the tools provided.
* Optimise Tokenizer class loading and saving functions for potentially better performance.
* Optimise GPU kernel dispatcher for GPU accelerated Vocab building.
* Optimise BINFile class where possible to improve performance.
* Usability pass on BINFile class and Tokenizer class.

## Known issues
None