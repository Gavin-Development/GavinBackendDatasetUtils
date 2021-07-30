# CPP-DataLoader
Data Loader for Gavin written in C++
This data loader is a WIP solution to speed up dataset load times for training Gavin Bot.

## Usage
Currently there is only 1 working function that has been tested and verified to work properly. This is the LoadTrainDataST() function.

```python
samples = LTD.LoadTrainDataST(10000000, "C:/Users/User/Desktop/Gavin/GavinTraining/", "Tokenizer-3.to", 69108,66109, 52, 0)
```
This will load 10,000,000 samples from the specified file in the specified directory. It will add start token 69108 and end token 66109 and pad each sample to length 52 with 0s.

## Known issues

* LoadTrainDataMT() runs into an issue where the worker threads randomly terminate when set to a large sample size.
