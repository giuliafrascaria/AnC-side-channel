#!/usr/bin/env python

import os
import matplotlib.pyplot as plt
import numpy as np

scan_filepath = "./scan.txt"
heat_filepath = "heatmap.png"

def main():

	if not os.path.exists(scan_filepath):
		print("%s not found" % scan_filepath)
		return
	
	scan_values = [int(line) for line in open(scan_filepath)]

	fig = plt.figure()
	a = np.reshape(scan_values, (-1, 64))
	plt.imshow(a, cmap='hot', interpolation='none')
	plt.colorbar()
	plt.show()

	if os.path.exists(heat_filepath):
		os.remove(heat_filepath)

	fig.savefig(heat_filepath)

if __name__ == "__main__":
	main()
