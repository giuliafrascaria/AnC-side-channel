#include<errno.h>
#include<fcntl.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<string.h>
#include<limits.h>

#define UNIFIED_TLB_SIZE (8 * 4096)
#define I_TLB_SIZE (8 * 64)
#define CACHE_LINE_SIZE 64
#define NUMBER_OF_CACHE_OFFSETS 64
#define KB (1 << 10)
#define MB (1 << 20)
#define GB (1 << 30)
#define TB (1L << 40)

typedef void (*fp)(void);

int evict_itlb(volatile unsigned char *buffer, size_t size, unsigned short int cache_line_offset, uint64_t page_size)
{
	int i;
	fp ptr;

	if(buffer == NULL)
	{
		return -1;
	}
	
	if(cache_line_offset >= NUMBER_OF_CACHE_OFFSETS)
	{
		printf("Cache line offset must be between 0 and 63 (incl.)\n");
		return -1;
	}
	
	cache_line_offset *= CACHE_LINE_SIZE; // convert offset to number of  bytes

	// store eviction set return instructions and flush unified TLB
	for(i = cache_line_offset; i < UNIFIED_TLB_SIZE * page_size; i += page_size)
	{
		buffer[i] = 0xc3;
	}

	// execute i-tlb eviction set
	for(i = cache_line_offset; i < I_TLB_SIZE * page_size; i += page_size)
	{
		ptr = (fp)(&(buffer[i]));
		ptr();
	}

	return 0;
}

void profile_mem_access(volatile unsigned char* c, volatile unsigned char* ev_set, size_t ev_set_size, char* filename, unsigned short int pte_offset, uint64_t page_size)
{
	int i, j, k;
	int NUM_MEASUREMENTS = 5; // make 5 measurements and take mean
	unsigned long long hi1, lo1;
	unsigned long long hi, lo;
	uint64_t old, new, base;
	uint64_t t[NUM_MEASUREMENTS];
	uint64_t ret; // mean value for measurements
	fp ptr; // pointer to function stored in the target buffer
	FILE *f = fopen(filename, "ab+");

	if(f == NULL)
	{
		perror("Failed to open file for printing memory access profiles");
		return;
	}

	//we chose the target instruction at offset 0 within a page
	c[pte_offset * page_size] = 0xc3;
	ptr = (fp)&(c[pte_offset * page_size]);

	for(i = -1; i < NUMBER_OF_CACHE_OFFSETS; i++){
		if(i >= 0){
			//checking target addresss at a different offset than the i-th
			c[(((i + 1) % NUMBER_OF_CACHE_OFFSETS) * CACHE_LINE_SIZE) + pte_offset * page_size] = 0xc3;
			ptr = (fp)&(c[(((i + 1) % NUMBER_OF_CACHE_OFFSETS) * CACHE_LINE_SIZE) + pte_offset * page_size]);
		}
		for(j = 0; j < NUM_MEASUREMENTS; j++){

			//evict the i-th cacheline for each page in the eviction set
			//evict tlb
			if(i >= 0 && evict_itlb(ev_set, ev_set_size, i, page_size) < 0)
			{
				printf("Failed to evict TLB\n");
				fclose(f);
				return;
			} else if (i < 0) {
				ptr();
			}

			asm volatile ("mfence\n\t"
								"CPUID\n\t"
								"RDTSC\n\t"
								"mov %%rdx, %0\n\t"
								"mov %%rax, %1\n\t" : "=r"(hi1), "=r"(lo1) : : "%rax", "%rbx", "%rcx", "%rdx");
			ptr();

			asm volatile ("RDTSCP\n\t"
								"mov %%rdx, %0\n\t"
								"mov %%rax, %1\n\t"
								"CPUID\n\t"
								"mfence" : "=r"(hi), "=r"(lo) : : "%rax", "%rbx", "%rcx", "%rdx");

			old = (uint64_t) (hi1 << 32) | lo1;
			new = (uint64_t) (hi << 32) | lo;
			t[j] = new - old;
		}

		ret = 0;
		
		for(j = 0; j < NUM_MEASUREMENTS; j++){
			ret += t[j];		
		}
		
		ret /= NUM_MEASUREMENTS;
		
		if(i >= 0 && fprintf(f, "%d\n", abs(ret - base)) < 0)
		{
			perror("Failed to print memory access");
			fclose(f);
			return;
		} else if (i < 0) {
			base = ret;
		}
	}

	fclose(f);
}


void scan_target(volatile unsigned char* c, volatile unsigned char* ev_set, size_t ev_set_size, uint64_t page_size, char* filename)
{
		//move 1 page at a time, for now 64 pages should be enough
		for(int i = 0; i < 64; i++)
		{
			// offset is 8x page size in order to cross cache line
			profile_mem_access(c, ev_set, ev_set_size, filename, 8 * i, page_size);
		}
}


int main(int argc, char* argv[])
{
	size_t ev_set_size = 128 * MB;
	uint64_t target_size = TB; // 1 TB target buffer
	volatile unsigned char *ev_set;
	volatile unsigned char *target = (unsigned char*)mmap(NULL, target_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if(target == MAP_FAILED)
	{
		perror("Failed to map memory.");
		return -1;
	}

	ev_set = (unsigned char*)mmap(NULL, ev_set_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if(ev_set == NULL)
	{
		perror("Failed to initialize TLB eviction set");
		munmap((void*) target, ev_set_size);
		return -1;
	}

	scan_target(target, ev_set, ev_set_size, 4 * KB, "scan.txt"); // measure for PTL1

	// munmap((void*) target, target_size);
	// munmap((void*) ev_set, ev_set_size);
	// free(cache_flush_set);
	// free(pages);

	return 0;
}
