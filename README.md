# CPP-DataLoader
Data Loader for Gavin written in C++
This data loader is a WIP solution to speed up dataset load times for training Gavin Bot.
Currently the python modules Pickle & Base64 are called from the c++ functions to decompress the stored data. This reduces the speed of the ST implimentation and will eventually be changed once a workaround to the errors caused by using non python base 64 decode are fixed.

## Usage

### Old File Format
Currently there is good support for the OG file format (pickled by python) with functions to load it and convert it to a new .BIN format.

This is the LoadTrainDataST() function:
```python
samples = LTD.LoadTrainDataST(10000000, "C:/Users/User/Desktop/Gavin/GavinTraining/", "Tokenizer-3.to", 69108,66109, 52, 0)
```
This will load 10,000,000 samples from the specified file in the specified directory. It will add start token 69108 and end token 66109 and pad each sample to length 52 with 0s.

This is the ConvertDataSet_TEST() function:
```python
ConvertDataSet_TEST(10000, "Tokenizer-3.to", "Tokenizer-3.to.BIN")
```
This will open the legacy file and convert the first 10000 lines (samples) to the new .BIN format and save it with file name "Tokenizer-3.to.BIN".

**NOTE** Using a very large number will basically mean it will load all samples in the file then save to .BIN as the limit only exists for the purposes of expaditing testing.

### New File Format
In the last commit support was added for the new file format (.BIN) this format is almost identical to the old format with a few tweaks to allow greater parralelism in loading file contents, removing the need for embedded python code and thus allowing actual multithreaded loading of the file.

This is the LoadTrainDataST_Future() function:
```python
samples = LoadTrainDataST_Future(10000000, "C:/Users/User/Desktop/Gavin/GavinTraining/", "Tokenizer-3.to.BIN", 69108,66109, 52, 0)
```
This function has basically identical usage to the legacy function as to not cause issues with integrating it into existing code. The only difference is that it is designed (and optimised to an extent) to load and parse the .BIN files to achieve greater throughput of data.

## Work in progress functions
#### LoadTrainDataMT_Future
This function will eventually load the files using multiple logical cores / threads allieviating the processing bottleneck to take full advantage of high IO that might be avalible.

#### LoadTrainDataGPU_Accelerated_Future
This function will eventually load the files and use the GPU (cuda) for SIMD execution to speed up the serialising of file data into usable format.

## Known issues

* LoadTrainDataMT() runs into an issue where the worker threads terminate due to error caused by multiple threads calling embedded python code at the same time.
* LoadTrainDataGPU_Accelerated_Future Does not work, the reduction algo is still not complete for determining the number of lines / samples in the file.
