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

#define PAGE_SIZE 4096
#define CACHE_LINE_SIZE 64
#define NUMBER_OF_CACHE_OFFSETS 64

typedef void (*fp)(void);

int evict_itlb(volatile unsigned char *buffer, size_t size, int offset)
{
	int i, j, maxj;
	fp ptr;

	if(buffer == NULL)
	{
		return -1;
	}

	// execute eviction set instructions
	for(i = 0; i < size; i += PAGE_SIZE)
	{

		buffer[i + offset] = 0xc3;
		ptr = (fp)(&(buffer[i + offset]));

		ptr();
	}

	return 0;
}

void profile_mem_access(volatile unsigned char** c, volatile unsigned char* ev_set, size_t ev_set_size, char* filename, int offset)
{
	int i, j, k;
	unsigned long long hi1, lo1;
	unsigned long long hi, lo;
	uint64_t t, old, new;
	fp ptr; // pointer to function stored in the target buffer
	FILE *f = fopen(filename, "ab+");


	if(f == NULL)
	{
		perror("Failed to open file for printing memory access profiles");
		return;
	}

	//we chose the target instruction at offset 0 within a page
	(*c)[offset] = 0xc3;
	ptr = (fp)&((*c)[offset]);


	for(j = 0; j < 1; j++)
	{
		for(i = 0; i < NUMBER_OF_CACHE_OFFSETS; i++){

			//evict the i-th cacheline for each page in the eviction set
			//evict cache line i

			//evict tlb
			if(evict_itlb(ev_set, ev_set_size, i + NUMBER_OF_CACHE_OFFSETS) < 0)
			{
				printf("Failed to evict TLB\n");
				fclose(f);
				return;
			}

			// //target address with different offset than the one from which we evicted
			// (*c)[((i + 1) % NUMBER_OF_CACHE_OFFSETS) * NUMBER_OF_CACHE_OFFSETS] = 0xc3;
			// ptr = (fp)&((*c)[((i + 1) % NUMBER_OF_CACHE_OFFSETS) * NUMBER_OF_CACHE_OFFSETS]);


			asm volatile ("mfence\n\t"
								"CPUID\n\t"
								"RDTSC\n\t"
								"mov %%rdx, %0\n\t"
								"mov %%rax, %1\n\t" : "=r"(hi1), "=r"(lo1) : : "%rax", "%rbx", "%rcx", "%rdx");

			//asm volatile("movq (%0), %%rax\n" : : "c"(ptr) : "rax");

			//time execution
			ptr();

			asm volatile ("RDTSCP\n\t"
								"mov %%rdx, %0\n\t"
								"mov %%rax, %1\n\t"
								"CPUID\n\t"
								"mfence" : "=r"(hi), "=r"(lo) : : "%rax", "%rbx", "%rcx", "%rdx");

			old = (uint64_t) (hi1 << 32) | lo1;
			new = (uint64_t) (hi << 32) | lo;
			t = new - old;

			if(fprintf(f, "%llu\n", (unsigned long long) t) < 0)
			{
				perror("Failed to print memory access");
				fclose(f);
				return;
			}

		}


	}

	fclose(f);
}


void scan_target(volatile unsigned char** c, volatile unsigned char* ev_set, size_t ev_set_size, char* filename)
{
		//move 1 page at a time, for now 10 pages should be enough
		for(int i = 0; i < 10; i++)
		{
			printf("new page\n");
			profile_mem_access(c, ev_set, ev_set_size, filename, i*PAGE_SIZE);
		}
}


int main(int argc, char* argv[])
{
	size_t ev_set_size = 32768 * PAGE_SIZE; // 8192 TLB entries
	volatile unsigned char *ev_set;

	volatile unsigned char *target = (unsigned char*)mmap(NULL, ev_set_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	target[0] = "F";
	target[1] = "U";

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


	profile_mem_access(&target, ev_set, ev_set_size, "uncached.txt", 0);
	//profile_mem_access(&target, ev_set, ev_set_size, "hopefully_cached.txt");

	scan_target(&target, ev_set, ev_set_size, "scan.txt");

	// free((void*)pte);
	// munmap((void*)page_ptr, PAGE_SIZE);
	// munmap((void*) target, target_size);
	// munmap((void*) ev_set, ev_set_size);
	// free(cache_flush_set);
	// free(pages);

	return 0;
}
