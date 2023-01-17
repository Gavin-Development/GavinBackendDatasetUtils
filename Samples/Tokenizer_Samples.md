# GavinBackendDatasetUtils Tokenizer Samples.

This README contains info on examples of how to use the module.

## Tokenizer class

```
Tokenizer(kwargs **)
Tokenizer(args)
Tokenizer(string)
Tokenizer()
```

kwargs can be:
	* TargetVocabSize, (int)
	* UnknownToken, ((int, str) or (int) or (str))
	* NewLineToken, ((int, str) or (int) or (str))
	* SpaceToken, ((int, str) or (int) or (str))

For the case of a single string passed the constructor implicitly sets the corresponding integer value automatically.

```
Tokenizer.Save()
Tokenizer.Save(path)
Tokenizer.Load()
Tokenizer.Load(path)
```

**Note** If calling method without path passed on an instance without filepath already set error will be thrown.

```
Tokenizer.Build(list)
Tokenizer.Encode(list)
Tokenizer.Decode(numpy arr)
```

**Note** Recommended usage of Encode(list) is shown below. Please follow for best results.

# Innit

```
import GavinBackendDatasetUtils as LTD

Test_Tokenizer = LTD.Tokenizer("./TokenizerName.TKNZR")
```
This loads the tokenizer from disk.

# Build Encodes

```
ListOfWords = ["A", "Dog", "is", "sus"]

Test_Tokenizer.Build(ListOfWords)
```
This sample takes in the corpus `ListOfWords` and builds the vocab in the tokenizer.

# Encoding & decoding a string

```
Encodes = Test_Tokenizer.Encode("I am a human being.".split(" "))

Decodes = Test_Tokenizer.Decode(Encodes)
```
This sample takes in the string and encodes it to integers then decodes it back to a string.

# Saving & Loading the Tokenizer

```
Test_Tokenizer.Save()

Test_Tokenizer.Load()
```
This sample saves, then loads up the tokenizer. If there was no path specified at instantiation of the tokenizer (i.e the tokenizer is loaded from disk) the function will throw an error.

```
Test_Tokenizer.Save("./TestTokenizer.TKNZR")

Second_Tokenizer = LTD.Tokenizer("./TestTokenizer.TKNZR")

```

This sample saves the Test_Tokenizer to "./TestTokenizer.TKNZR" then loads it back up into a second tokenizer instance.