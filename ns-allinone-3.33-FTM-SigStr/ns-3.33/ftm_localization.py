import pathlib
import subprocess
import time
import glob
import csv
import numpy as np
from scipy.optimize import minimize
import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt
import os

results_path = "sig_str_localization/"
error_models = ["wired/", "wireless/", "sig_str/", "sig_str_fading/"]

def performTests():
    error_model_index = 0
    for curr_error_model in error_models:
        for seed in range(1, 101):
            output = results_path + curr_error_model
            if seed < 10:
                output += "0" + str(seed)
            else:
                output += str(seed)

            waf_command = ["--run=ftm-sig-str-localization",
                           "--seed=" + str(seed),
                           "--error=" + str(error_model_index),
                           "--filename=" + output
                           ]
            waf_string = ' '.join(waf_command)
            subprocess.run(["./waf", waf_string])
        error_model_index += 1

def main():
    performTests()

if __name__ == "__main__":
    start_time = time.perf_counter()
    main()
    end_time = time.perf_counter()
    time_elapsed = round(end_time - start_time, 3)
    print("DONE")
    print("Time elapsed: " + str(time_elapsed) + 's')

