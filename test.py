import sys
from pathlib import Path

root_path = Path(__file__).resolve()
sys.path.append(str(root_path))

import GavinBackendDatasetUtils as LTD

print("Starting Single Thread: ")
samples_one = LTD.LoadTrainDataST(800, "./", "Test.BIN", 69108, 66109, 52, 0)
print("Starting Multi Thread: ")
samples_two = LTD.LoadTrainDataMT(800, "./", "Test.BIN", 69108, 66109, 52, 0)
