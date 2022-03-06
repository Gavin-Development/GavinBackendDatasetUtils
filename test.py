import sys
import numpy as np
from pathlib import Path

root_path = Path(__file__).resolve()
sys.path.append(str(root_path))

import GavinBackendDatasetUtils as LTD


test_vocab = """A short story is a piece of prose fiction that
    typically can be read in one sitting and focuses
    on a self-contained incident or series of linked
    incidents, with the intent of evoking a single
    effect or mood. Tokenization, when applied to data security,
    is the process of substituting a sensitive
    data element with a non-sensitive equivalent,
    referred to as a token, that has no extrinsic
    or exploitable meaning or value. The token is
    a reference that maps back to the sensitive data
    through a tokenization system."""

sentences = test_vocab.split('.')
sentences = [s.strip() for s in sentences]
sentences = [s for s in sentences if len(s) > 0]
sentences = [s + '.' for s in sentences]


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
tokenizer.build_vocab(sentences)
print("Tokenizer values: ")
print(f"Vocab Size: {tokenizer.get_vocab_size()}")
print(f"Vocab: {tokenizer.get_vocab()}")
