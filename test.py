import sys
import numpy as np
from pathlib import Path

root_path = Path(__file__).resolve()
sys.path.append(str(root_path))

import GavinBackendDatasetUtils as LTD


handle = open("test-lines.txt", "r")
test_vocab = [line for line in handle.readlines()]


print(LTD.__dict__)
print(LTD.__version__)

print("Starting Single Thread: ")
samples_one = LTD.LoadTrainDataST(800, "./", "Test.BIN", 69108, 66109, 52, 0)
print("Starting Multi Thread: ")
samples_two = LTD.LoadTrainDataMT(800, "./", "Test.BIN", 69108, 66109, 52, 0)
array_d = np.array(samples_two)
print(array_d.shape)

tokenizer = LTD.Tokenizer("Test", 500)
print("Building tokenizer values...")
tokenizer.build_vocab(test_vocab)
print("Tokenizer values: ")
print(f"Vocab Size: {tokenizer.get_vocab_size()}")
print(f"Vocab: {tokenizer.get_vocab()}")
encoded = tokenizer.encode("This is a test")
decoded = tokenizer.decode(encoded)
print(f"Encoded: {encoded}")
print(f"Decoded: {decoded}")
