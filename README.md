# CPP-DataSetManager
Data Loader for Gavin written in C++

This tool set is a WIP suite of tools for authoring, managing & loading datasets for Gavin bot.

## Usage

### New File Format
The default dataset / file format is .BIN, it is designed to be highly compressable and efficient to load. The format is mostly complete but additions and tweaks are still being made.

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

**NOTE** This is the preferred method of loading the dataset due to its speed, memory efficiency and overall performance.

This is the ConvertDataSet() function:
```python
LTD.ConvertDataSet_TEST(10_000_000,"C:/Users/user/Desktop/Gavin/GavinTraining/Tokenizer-3.from", "./10_MILLION.BIN")
```
This function will load up the Tokenizer-3.from file, load it and transcode it to BIN format and save it to 10_MILLION.BIN, it will load the first 10 million samples.
It has minimal optimisation as its only meant to be a temporary measure for compatibility as we transition to BIN format.

### Generator
The last commit adds in the first version of a working data generator. It isnt blazing fast (yet) but it will do the trick where system RAM is a limiting factor for loading the dataset. It has 2 array exposed to the user (ToSampleBuffer, FromSampleBuffer), it has initialisation and buffer update methods exposed aswell.

```python
Generator = LTD.DataGenerator("./", "To_Samples.BIN", "From_Samples.BIN", 100_000, 69108, 66109, 52, 0)
```
This will create the Generator and all its associated memory objects, open file pointers to the question samples (To_Samples.BIN) and the answer samples (From_Samples.BIN). The rest of the parameters mirror that of LoadTrainDataST().

```python
Generator.UpdateBuffer()
```
This will read the next *100_000* samples from each of the files and load them up into the respective buffers.

**NOTE** This is the first working implimentation so it will be slow and it wont error handle for you. The generator will allocate 2 buffers that are exposed to python and re fill the *same* 2 buffers with new samples each time UpdateBuffer() is called. You will need to call UpdateBuffer() after initialisation to fill the buffers.

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
* Impliment a data stream class that dynamically streams in larger datasets to allow significantly lower memory usage during training of gavin. This mechanism is under investigation and not currently being implimented nor guarenteed as a feature.
* Investigate memory usage of ST impl to posibaly optimise & restructure code to eliminate un neccesary operations.
* Change the way how datasets are handled to allow OOP usage of dataset files from python to simplify the way the programmer handles the datasets in python. This involves abstracting the opening, loading and modification and saving of datasets.

## Known issues
- Generator is a bit slow at loading.
- Generator is not 100% compatible with tensorflow and the way it does things.

## Changes
* A small tweak was done to the data generator such that it now loads all the headers in 1 go rather than loading them all one at a time and then processing the sample loads, this has a small effect on memory profile of the generator but it is insignificant enough that the roughly 8x speedup is preferable.