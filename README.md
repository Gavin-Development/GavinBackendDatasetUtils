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

## Known issues
None
