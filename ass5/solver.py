#!/usr/bin/env python

from __future__ import division
import os
import numpy as np

scan_filename = 'scan.txt'


def find_cacheline_offsets():
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
	
	offsets = []
	offsets.append(offset_lv4[0])
	offsets.append(offset_lv3[0])
	offsets.append(offset_lv2[0])
	offsets.append(offset_lv1[0])
	return offsets


def find_slot_offset(lvl):
	filename = 'scan_{0}.txt'.format(lvl)
	
	if not os.path.isfile(filename):
		print("{0} does not exist".format(filename))
		return -1

	f = open(scan_filename, 'r')
	scan_values = [int(line) for line in f]
	a = np.reshape(scan_values, (-1, 64))
	a_summed = a.sum(axis=0)
	enum = enumerate(a_summed)
	offsets = [i for i,j in enum if j == max(a_summed)]
	
	prev_index = (offsets[0] + 63) % 64
	next_index = (offsets[0] + 65) % 64
	
	page = 0
	
	if a_summed[prev_index] > a_summed[next_index]:
		while a[page][prev_index] > a[page][offsets[0]]:
			page += 1
	else:
		while a[page][offsets[0]] > a[page][next_index]:
			page += 1
	
	return 8 - page


def main():
	result = 0
	offsets = find_cacheline_offsets()
	result = (offsets[0] << 3) | find_slot_offset(4)
	result = result << 9
	result = (offsets[1] << 3) | find_slot_offset(3)
	result = result << 9
	result = (offsets[2] << 3) | find_slot_offset(2)
	result = result << 9
	result = (offsets[3] << 3) | find_slot_offset(1)
	
	print(format(result << 12, '#04x'))

if __name__ == "__main__":
	main()

