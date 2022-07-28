# GavinBackendDatasetUtils Tokenizer Samples.

This README contains info on examples of how to use the module.

## Tokenizer class

# Innit

```
import GavinBackendDatasetUtils as LTD

Test_Tokenizer = LTD.Tokenizer("TokenizerName")
```
This creates the tokenizer and initialises it as a class for the programmer to use.

# Build Encodes

```
ListOfWords = ["A", "Dog", "is", "sus"]

Test_Tokenizer.BuildEncodes(ListOfWords)
```
This sample takes in the corpus `ListOfWords` and builds the vocab in the tokenizer.

**Note** The Tokenizer supports progressive building of the vocab so the `BuildEncodes()` Function can be called mutliple times to progressively build the vocab, passing sections of the corupus each time.

# BuildEncodes on GPU

```
ListOfWords = ["A", "Dog", "is", "sus"]

Test_Tokenizer.BuildEncodes_GPU(ListOfWords)
```
This sample takes in the corpus `ListOfWords` and builds the vocab in the tokenizer using GPU acceleration, this is up to 12x faster than CPU based vocab builder (testing on RTX 3090 with 12900ks, 32GB 6400c16)

**Note** As with the non GPU accelerated vocab builder, this function support progressively building the vocab over many launches of the function.

# Encoding & decoding a string

```
Encodes = Test_Tokenizer.Encode("I am a human being.")

Decodes = Test_Tokenizer.Decode(Encodes)
```
This sample takes in the string and encodes it to integers then decodes it back to a string.

# Saving & Loading the Tokenizer

```
Test_Tokenizer.Save()

Test_Tokenizer.Load()
```
This sample saves, then loads up the tokenizer. By default the tokenizer saves as "TokenizerName.TOKENIZER" into the scripts local DIR, You can load up an existing Tokenizer by creating a new class and passing the same Tokenizer name as a Tokenizer saved on disk in the local DIR as shown below.

```
Test_Tokenizer.Save()

Second_Tokenizer = LTD.Tokenizer("TokenizerName")

Second_Tokenizer.Load()
```

# Accessing Encodes & Commonality

```
for i in range(0, Test_Tokenizer.GetVocabSize()):
	print(f"{Test_Tokenizer.Words[i]}  {Test_Tokenizer.Occurences[i]}")
```
This sample loops over all the possible encodes the Tokenizer knows and prints the encode and its associated commonality or number of occurances to console.
