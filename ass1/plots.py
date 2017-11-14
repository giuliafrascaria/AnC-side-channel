#!/usr/bin/env python

import os
import matplotlib.pyplot as plt

cached_filepath = "./cached.txt"
uncached_filepath = "./uncached.txt"
fig_filepath = "plots.png"

def main():
	cached = []
	uncached = []
	
	if not os.path.exists(cached_filepath):
		print("%s not found" % cached_filepath)
		return
		
	if not os.path.exists(uncached_filepath):
		print("%s not found" % uncached_filepath)
		return
		
	cached_values = [int(line) for line in open(cached_filepath)]
	uncached_values = [int(line) for line in open(uncached_filepath)]
	
	fig = plt.figure()
	
	fig.suptitle('Memory access profiles for 1000 cached and 1000 uncached accesses')
	plt.hist(cached, bins=range(min(cached), max(cached), 5), color='green', label='cached')
	plt.hist(uncached, bins=range(min(uncached), max(uncached), 5), color='red', label='uncached')
	plt.xlabel('Clock ticks')
	plt.ylabel('Frequency (number of experiments)')
	plt.legend(loc='best')

	if os.path.exists(fig_filepath):
		os.remove(fig_filepath)
	
	fig.savefig(fig_filepath)

if __name__ == "__main__":
	main()

