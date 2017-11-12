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
		
	cached = [int(line) for line in open(cached_filepath)]
	uncached = [int(line) for line in open(uncached_filepath)]
	
	fig = plt.figure()
	
	plt.plot(cached, color='green', label='cached')
	plt.plot(uncached, color='red', label='uncached')
	plt.ylabel('Clock ticks')
	plt.xlabel('Experiment number')
	plt.legend(loc='best')
	
	if os.path.exists(fig_filepath):
		os.remove(fig_filepath)
	
	fig.savefig(fig_filepath)

if __name__ == "__main__":
	main()

