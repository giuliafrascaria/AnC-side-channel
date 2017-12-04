#!/usr/bin/env python

import os
from matplotlib import pyplot as plt
import numpy as np

scan_filename = 'scan.txt'
fig_filename = 'heatmap.png'
explanation = 'The memorygram displays signals on all PT levels. \n' \
	'With every row, we move from the target address at offsets such that \n' \
	'we cross 1 cacheline at PTL4, 2 cachelines at PTL3, \n2 cachelines at PTL2, ' \
	'and 4 cachelines at PTL1. \nThis results in a staircase moving one cacheline \n' \
	'with every row, corresponding to the PTL4 signal, \none moving 2 cachelines, ' \
	'corresponding to the PTL3 signal, \none staircase moving 3 cachelines with ' \
	'every row, indicating the PTL2 signal, \nand a final staircase moving 4 ' \
	'cachelines with every row, \ncorresponding to the PTL1 signal. \n Since ' \
	'our buffer is mapped exactly 4TB high in memory, \nthe lower 3 PTL signals ' \
	'diverge from cacheline offset 0, \nwhile the PTL4 signal begins at cacheline ' \
	'offset 1.'

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
	fig.text(.5, -0.4, explanation, ha='center')
	#plt.show()
	
	if os.path.exists(fig_filename):
		os.remove(fig_filename)

	fig.savefig(fig_filename, bbox_inches='tight')
	f.close()

if __name__ == "__main__":
	main()
