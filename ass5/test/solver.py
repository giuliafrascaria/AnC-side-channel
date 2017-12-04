#!/usr/bin/env python

from __future__ import division
import os
import numpy as np
import subprocess

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
	a = np.reshape(scan_values, (-1, 64)).transpose()
	
	for offset in range(len(a)):
		i = 0
		j = len(a[offset]) - 1
		next_offset = (offset + 1) % len(a[offset])
		
		while a[offset][i] > a[next_offset][i]:
			i += 1
		
		while a[offset][j] < a[next_offset][j]:
			j -= 1
		
		if j == i - 1:
			return 8 - j
			
	return -1
	
	
def make_prog():
	make_process = subprocess.Popen(['make', 'clean', 'all'], stderr=subprocess.STDOUT)
	
	if make_process.wait() != 0:
		print("Failed to make prog")
		return -1
		
	return 0
	

def run_prog():
	return subprocess.check_call(["./prog"])


def main():
	#print("Compiling prog..")

	#if make_prog() < 0:
	#	return -1
		
	#print("Successfully compiled prog")
	#print("Executing prog, profiling memory accesses..")
	
	#if run_prog() < 0:
	#	print("Failed to run prog")
	#	return -1
		
	#print("Finished running prog, solving target address based on memory profiles..")
	
	result = 0
	offsets = find_cacheline_offsets()
	
	for i in range(4):
		result = result << 3
		slot = find_slot_offset(4 - i)
		
		if slot < 0:
			print("Failed to correctly identify offset within cacheline for level {0}".format(4 - i))
			return
		
		result |= slot
	
	print(format(result << 12, '#04x')) #TODO identify offset within frame


if __name__ == "__main__":
	main()

