#!/usr/bin/env python

from __future__ import division
import os
import numpy as np

scan_filename = 'scan.txt'

def main():
	if not os.path.isfile(scan_filename):
		print("scan.txt does not exist")
		return

	f = open(scan_filename, 'r')
	scan_values = [int(line) for line in f]
	a = np.reshape(scan_values, (-1, 64))
	
	f.close()
	
	sig_lv4 = []
	sig_lv3 = []
	sig_lv2 = []
	sig_lv1 = []
	
	for (page, offset), ticks in np.ndenumerate(a):
		if page == 0:
			sig_lv4.append(ticks)
			sig_lv3.append(ticks)
			sig_lv2.append(ticks)
			sig_lv1.append(ticks)
		else:
			sig_lv4[(offset + 64 - page) % 64] += ticks
			sig_lv3[(offset + 64 - 2 * page) % 64] += ticks
			sig_lv2[(offset + 64 - 3 * page) % 64] += ticks
			sig_lv1[(offset + 64 - 4 * page) % 64] += ticks
	
	offset_lv4 = [i for i,j in enumerate(sig_lv4) if j == max(sig_lv4)]
	offset_lv3 = [i for i,j in enumerate(sig_lv3) if j == max(sig_lv3)]
	offset_lv2 = [i for i,j in enumerate(sig_lv2) if j == max(sig_lv2)]
	offset_lv1 = [i for i,j in enumerate(sig_lv1) if j == max(sig_lv1)]
	
	print("Possible level 4 offsets are: {0}\n".format(offset_lv4))
	print("Possible level 3 offsets are: {0}\n".format(offset_lv3))
	print("Possible level 2 offsets are: {0}\n".format(offset_lv2))
	print("Possible level 1 offsets are: {0}\n".format(offset_lv1))

if __name__ == "__main__":
	main()

