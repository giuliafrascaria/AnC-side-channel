#!/usr/bin/env python

import os
import matplotlib.pyplot as plt

cached_filepath = "./hopefully_cached.txt"
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
	max_timing = max(max(cached_values), max(uncached_values))
	hist_bar_width = int(0.008 * max_timing)
	bins = [i for i in range(max_timing + hist_bar_width) if i % hist_bar_width == 0]

	cached.hist(uncached_values, color='red', bins=bins)
	cached.set_title('Uncached')

	uncached.hist(cached_values, color='blue', bins=bins)
	uncached.set_title('Touched')

	fig.subplots_adjust(hspace=0.5)
	plt.setp([a.get_xticklabels() for a in fig.axes], visible=True)

	fig.text(0.5, 0.01, 'Clock ticks', ha='center')
	fig.text(0.03, 0.5, 'Experiment number', va='center', rotation='vertical')

	if os.path.exists(fig_filepath):
		os.remove(fig_filepath)

	fig.savefig(fig_filepath)

if __name__ == "__main__":
	main()

