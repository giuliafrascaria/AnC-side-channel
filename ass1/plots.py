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

	fig, (cached, uncached) = plt.subplots(2, sharex=True, sharey=True)
	
	cached.hist(cached_values, color='green', bins=40)
	cached.set_title('Cached')
	
	uncached.hist(uncached_values, color='red', bins=40)
	uncached.set_title('Uncached')
	
	fig.subplots_adjust(hspace=0.5)
	plt.setp([a.get_xticklabels() for a in fig.axes], visible=True)

	fig.text(0.5, 0.01, 'Clock ticks', ha='center')
	fig.text(0.03, 0.5, 'Experiment number', va='center', rotation='vertical')
	
	if os.path.exists(fig_filepath):
		os.remove(fig_filepath)
	
	fig.savefig(fig_filepath)

if __name__ == "__main__":
	main()

