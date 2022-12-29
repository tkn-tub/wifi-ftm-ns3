import numpy as np
import glob
import matplotlib.pyplot as plt
import pandas as pd
import seaborn as sns

error_models = ["wired", "wireless", "sig_str", "sig_str_fading"]
# rename files to 3 digit size for sorting correctly with 100m
# import os
# for curr_error_model in error_models:
#     files = sorted(glob.glob(curr_error_model + "/*m"))
#     for f in files:
#         split_f = f.split('/')
#         if len(split_f[1][:-1]) < 3:
#             os.rename(f, split_f[0] + "/0" + split_f[1])

for curr_error_model in error_models:
    files = sorted(glob.glob(curr_error_model + "/*m"))
    files = files[24:75:5]
    data = np.zeros((len(files) * 14220, 4))
    i = 0
    for f in files:
        curr_measurement = np.loadtxt(f)
        curr_distance = int(f.split('\\')[1][:-1])
        for m in curr_measurement:
            data[i,0] = m[0] #RTT
            data[i,1] = m[1] #sig_str
            data[i,2] = curr_distance #real distance
            data[i,3] = m[0] / 2 / 1000 * 0.3 #convert RTT to distance in meters
            i += 1
    
    df = pd.DataFrame(data, columns=["RTT", "SigStr", "RealDist", "MeasuredDist"])
    df = df.loc[(df!=0).any(axis=1)]
    
    fig = plt.figure(figsize=(2.5,2.5*12/8))
    plt.rcParams['font.size'] = '7'
    sns.boxplot(data=df, x='RealDist', y='MeasuredDist', fliersize=0.5, linewidth=0.2)
    sns.lineplot(x=np.arange(0,len(files)), y=np.arange(25,75+1,5), linewidth=0.2, alpha=0.75, label="ground truth")
    plt.xlabel("Real distance [m]")
    plt.ylabel("Measured distance [m]")
    tick_labels = np.arange(25,75+1,5)

    plt.xticks(ticks=np.arange(0,len(files)), labels=tick_labels, ha='center', fontsize=5)
    
    plt.ylim(20,80)
    y_tick_labels = []
    for i in np.arange(20,80):
        if i in tick_labels:
            y_tick_labels.append(str(i))
        else:
            y_tick_labels.append("")

    plt.yticks(ticks=np.arange(20,80), labels=y_tick_labels, fontsize=5)
    plt.grid(linewidth=0.2, alpha=0.25, linestyle='--')
    plt.legend()
    plt.tight_layout()
    plt.savefig(curr_error_model + ".pdf")
    # break
