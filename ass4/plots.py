#!/usr/bin/env python

import os
import matplotlib.pyplot as plt
import numpy as np

output_dir_path = '.'
output_file_prefix = 'scan_'

def main():

	files = [i for i in os.listdir(output_dir_path) if os.path.isfile(os.path.join(output_dir_path, i)) and output_file_prefix in i]
	
	for idx, f in enumerate(files):
		scan_values = [int(line) for line in open(f)]

		fig = plt.figure()
		a = np.reshape(scan_values, (-1, 64))
		plt.imshow(a, cmap='hot', interpolation='nearest')
		plt.colorbar()
		
		title = "PTL{0}".format(idx + 1)
		
		plt.title(title)
		plt.show()

		if os.path.exists(heat_filepath):
			os.remove(heat_filepath)

		fig.savefig('heatmap_' + title + '.png')

if __name__ == "__main__":
	main()
