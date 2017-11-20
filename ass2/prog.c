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

#define PAGE_SIZE 4096

typedef void (*fp)(void);

// returns the CR3 register value (physical address of PGD)
unsigned long get_page_table_root()
{
	unsigned long val;

	FILE* f = fopen("/proc/cr3", "rb");

	if(f == NULL)
	{
		return 0;
	}

	if(fscanf(f, "%lu\n", &val) < 0)
	{
		fclose(f);
		return 0;
	}

	fclose(f);

	return val;
}

unsigned long get_physical_addr(int fd, unsigned long page_phys_addr, unsigned long offset)
{
	unsigned long ret;
	unsigned long *page;

	page = (unsigned long*) mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, page_phys_addr);

	if(page == NULL)
	{
		return 0;
	}

	ret = page[offset] & 0xfffffffffffff000; // discard lowest 12 bits

	munmap((void*) page, PAGE_SIZE);

	return ret;
}

unsigned char* create_eviction_set(size_t size)
{
	unsigned char *buffer = (unsigned char*)mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);;

	if(buffer != NULL)
	{
		for(int i = 0; i < size; i += PAGE_SIZE)
		{
			buffer[i] = 0x90; //nop
			buffer[i+1] = 0xc3; //ret
		}
	} else {
		perror("Failed to allocate memory for eviction set");
	}
	
	return buffer;
}

int evict_itlb(unsigned char *buffer, size_t size)
{
	if(buffer == NULL)
	{
		return -1;
	}
	
	// execute eviction set instructions
	for(int i = 0; i < size; i += PAGE_SIZE)
	{
			fp ptr = (fp)(&(buffer[i]));
			ptr();
	}
	
	return 0;
}

void profile_mem_access(volatile unsigned char* c, int* buffer, size_t cache_flush_set_size, unsigned char* ev_set, size_t ev_set_size, int touch, char* filename)
{
	int i, j, k;
	unsigned long long hi1, lo1;
	unsigned long long hi, lo;
	uint64_t t, old, new;
	int maxj = cache_flush_set_size / sizeof(int);
	FILE *f = fopen(filename, "ab+");

	if(f == NULL)
	{
		perror("Failed to open file for printing memory access profiles");
		return;
	}
	
	if(touch == 1)
	{
		c[0] = 0x90; //nop
		c[1] = 0xc3; //ret
	}

	for(i = 0; i < 1000; i++)
	{
		if(evict_itlb(ev_set, ev_set_size) < 0)
		{
			printf("Failed to evict TLB\n");
			fclose(f);
			return;
		}
		// evict cache
		for(j = 0; j<maxj; j++)
		{
			buffer[j] = rand();
		}

		if(touch == 1)
		{
			fp ptr = (fp)&(c[0]);
			ptr();
		}

		asm volatile ("mfence\n\t"
							"CPUID\n\t"
							"RDTSC\n\t"
							"mov %%rdx, %0\n\t"
							"mov %%rax, %1\n\t" : "=r"(hi1), "=r"(lo1) : : "%rax", "%rbx", "%rcx", "%rdx");

		asm volatile("movq (%0), %%rax\n" : : "c"(c) : "rax");

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

	fclose(f);
}

unsigned long get_phys_addr(volatile unsigned char *buffer, off_t buffer_size)
{
	unsigned long buffer_address = (unsigned long) &buffer;
	unsigned long page_table_offset;
	unsigned long page_table_offset_mask = 0x1ff; // last 9 bits
	unsigned long frame_offset_mask = 0xfff; // last 12 bits
	unsigned long aux_phys_addr;
	int fd;

	if((aux_phys_addr = get_page_table_root()) < 0)
	{
		return -1;
	}

	fd = open("/dev/mem", O_RDONLY);

	printf("buffer virtual address: 0x%lx\n", buffer_address);

	// level 4
	printf("PT4 physical address: 0x%lx\n", aux_phys_addr);
	page_table_offset = (buffer_address >> 39) & page_table_offset_mask;
	aux_phys_addr = get_physical_addr(fd, aux_phys_addr, page_table_offset);

	// level 3
	printf("PT3 physical address: 0x%lx\n", aux_phys_addr);
	page_table_offset = (buffer_address >> 30) & page_table_offset_mask;
	aux_phys_addr = get_physical_addr(fd, aux_phys_addr, page_table_offset);

	// level 2
	printf("PT2 physical address: 0x%lx\n", aux_phys_addr);
	page_table_offset = (buffer_address >> 21) & page_table_offset_mask;
	aux_phys_addr = get_physical_addr(fd, aux_phys_addr, page_table_offset);

	// level 1
	printf("PT1 physical address: 0x%lx\n", aux_phys_addr);
	page_table_offset = (buffer_address >> 12) & page_table_offset_mask;
	aux_phys_addr = get_physical_addr(fd, aux_phys_addr, page_table_offset);

	aux_phys_addr = aux_phys_addr | (buffer_address & frame_offset_mask);
	printf("buffer physical address: 0x%lx\n", aux_phys_addr);
	
	return aux_phys_addr;
}

int main(int argc, char* argv[])
{
	size_t target_size = 1UL << 40; // >1 TB
	size_t cache_flush_set_size = 10 * 1024 * 1024; // 10MB
	size_t ev_set_size = 1024 * PAGE_SIZE; // 1024 TLB entries 
	int * cache_flush_set;
	volatile unsigned char *target = (unsigned char*)mmap(NULL, target_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	unsigned char *ev_set;
	
	if(target == MAP_FAILED)
	{
		perror("Failed to map memory.");
		return -1;
	}
	
	cache_flush_set = (int*)malloc(cache_flush_set_size);
	
	if(cache_flush_set == NULL)
	{
		perror("Failed to allocate memory for cache flush set");
		munmap((void*) target, target_size);
		return -1;
	}
	
	ev_set = create_eviction_set(ev_set_size);
	
	if(ev_set == NULL)
	{
		perror("Failed to initialize TLB eviction set");
		munmap((void*) target, target_size);
		free(cache_flush_set);
		return -1;
	}
	
	//get_phys_addr(target, target_size);

	// TODO store convenient instructions in 512(?) page offsets in buffer

	profile_mem_access(target, cache_flush_set, cache_flush_set_size, ev_set, ev_set_size, 0, "uncached.txt");
	profile_mem_access(target, cache_flush_set, cache_flush_set_size, ev_set, ev_set_size, 1, "hopefully_cached.txt");
	
	munmap((void*) target, target_size);
	munmap((void*) ev_set, ev_set_size);
	free(cache_flush_set);

	//profiling(buffer, "cached.txt", "uncached.txt", 0);

	return 0;
}
