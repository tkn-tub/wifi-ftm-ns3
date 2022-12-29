import numpy as np

data = np.loadtxt('johnsonsu')

with np.printoptions(precision=3, suppress=True):
    print(data[:,[0, 2, 3, 4, 5]])