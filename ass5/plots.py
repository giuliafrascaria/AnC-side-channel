#!/usr/bin/env python

import os
import matplotlib.pyplot as plt
import numpy as np

scan_filename = 'scan.txt'
fig_filename = 'heatmap.png'

def main():
	if not os.path.isfile(scan_filename):
		print("scan.txt does not exist")
		return

	f = open(scan_filename, 'r')
	scan_values = [int(line) for line in f]

	fig = plt.figure()
	a = np.reshape(scan_values, (-1, 64))
	plt.imshow(a, cmap='hot', interpolation='none')
	plt.colorbar()
	plt.show()
	
	if os.path.exists(fig_filename):
		os.remove(fig_filename)

	fig.savefig(fig_filename)
	f.close()

if __name__ == "__main__":
	main()
