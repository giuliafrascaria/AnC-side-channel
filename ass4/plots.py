#!/usr/bin/env python

import os
import matplotlib.pyplot as plt
import numpy as np

scan_filepath_prefix = "scan_"

def main():

	files = [i for i in os.listdir('./') if os.path.isfile(os.path.join('./', i)) and scan_filepath_prefix in i]

	for idx,filename in enumerate(files):
		f = open(filename, 'r')
		scan_values = [int(line) for line in f]

		fig = plt.figure()
		a = np.reshape(scan_values, (-1, 64))
		plt.imshow(a, cmap='hot', interpolation='none')
		plt.colorbar()
		#plt.show()

		fig_filename = "heatmap_ptl{0}.png".format(idx + 1)		
		
		if os.path.exists(fig_filename):
			os.remove(fig_filename)

		fig.savefig(fig_filename)
		f.close()

if __name__ == "__main__":
	main()
