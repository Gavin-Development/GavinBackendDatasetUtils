# CPP-DataSetManager
Data Loader for Gavin written in C++

This tool set is a WIP suite of tools for authoring, managing & loading datasets for Gavin bot.

# Current build is failing on Release mode. Please build in debug mode for it to work for testing.

## Usage

### New File Format
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


**NOTE** Future support for creating and modifying BIN files with this class is coming soonish when i get on with implimenting it.

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

### Generator
The data generator. It isnt blazing fast (yet) but it will do the trick where system RAM is a limiting factor for loading the dataset. It has 2 arrays exposed to the user (ToSampleBuffer, FromSampleBuffer), it has initialisation and buffer update methods exposed aswell.

```python
Generator = LTD.DataGenerator("./", "To_Samples.BIN", "From_Samples.BIN", 100_000, 69108, 66109, 52, 0)
```
This will create the Generator and all its associated memory objects, open file pointers to the question samples (To_Samples.BIN) and the answer samples (From_Samples.BIN). The rest of the parameters mirror that of LoadTrainDataST().

```python
Generator.UpdateBuffer()
```
This will read the next *100_000* samples from each of the files and load them up into the respective buffers.

**NOTE** This is being depreciated and succeeded by the BINFile class.

### Old File Format
Currently there is good support for the OG file format (pickled by python) with functions to load it and convert it to a new .BIN format.

This is the LoadTrainDataST() function:
```python
samples = LTD.LoadTrainDataST_Legacy(10000000, "C:/Users/User/Desktop/Gavin/GavinTraining/", "Tokenizer-3.to", 69108,66109, 52, 0)
```
This will load 10,000,000 samples from the specified file in the specified directory. It will add start token 69108 and end token 66109 and pad each sample to length 52 with 0s.

## Work in progress functions
#### SaveTrainData()
This function will save training data sets authored in python into the BIN format for later ingest by the training script.

## To Do
* Add functionality to data streaming class to improve usability and performance while adding more functionality.
* Investigate memory usage of ST impl to posibaly optimise & restructure code to eliminate un neccesary operations.

## Known issues
- Generator is a bit slow at loading.
- Generator is not 100% compatible with tensorflow and the way it does things.
