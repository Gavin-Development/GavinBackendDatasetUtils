# GavinBackendDatasetUtils Tokenizer Samples.

This README contains info on examples of how to use the module.

## BINFile class

#Innit
```
import GavinBackendDatasetUtils as LTD

Samples = LTD.BINFile("TestFile.BIN", 69, 420, 50, 0)
```
This sample opens a BIN file called TestFile in the local script directory and specified to use the start token 69 and end token 420 with sample length of 50 and padding val of 0 for reading samples.


#BINFile stats
```
print(Samples.MaxNumberOfSamples)
print(Samples.NumberOfSamples)
``` 
This code sample prints out the max size the BIN file can be and the number of samples in the BIN file currently.

#Reading from BINFile
```
A_Sample = Samples[0]
```
This code sample reads the first sample from the BIN file, it is 50 integers in length with the start, end & padding tokens set as specified in the BINFile constructor.

# Slicing from a BINfile
```
A_Slice = Samples.get_slice(0,1000)
```
This code sample reads the first 1000 samples from the BIN file using the slice method, this is done due to there being no easy way to operator overload python array slicing syntax.