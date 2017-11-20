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

void nop()
{
	asm volatile("nop");
	return;
}

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

fp* create_eviction_set(off_t size, int offset)
{
	fp *buffer = (fp *)mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

	if(buffer != NULL)
	{
		for(off_t i = offset; i < size; i += PAGE_SIZE)
		{
			buffer[i] = nop;
		}
	} else {
		perror("Failed to allocate memory for eviction set");
	}
	
	return buffer;
}

int evict_itlb(fp *buffer, off_t size, int offset)
{
	if(buffer == NULL)
	{
		return -1;
	}
	
	// execute eviction set instructions
	for(off_t j = offset; j < size; j += PAGE_SIZE)
	{
		buffer[j]();
	}
	
	return 0;
}

void profile_mem_access(volatile fp* c, int touch, char* filename)
{
	int i,k;
	unsigned long long hi1, lo1;
	unsigned long long hi, lo;
	uint64_t t, old, new;
	off_t j, buf_size = 64 * 1024 * 1024, maxj = (64 * 1024 * 1024) / sizeof(int);
	int* buffer;
	fp* ev_set;
	int offset = 0;
	off_t ev_set_size = offset + 512 * PAGE_SIZE; // TODO figure out unified TLB size
	FILE *f = fopen(filename, "ab+");

	if(f == NULL)
	{
		perror("Failed to open file for printing memory access profiles");
		return;
	}
	
	buffer = (int*)malloc(buf_size);
	
	if(buffer == NULL)
	{
		perror("Failed to allocate buffer for flushing cache");
		fclose(f);
		return;
	}
	
	ev_set = create_eviction_set(ev_set_size, offset);
	
	if(ev_set == NULL)
	{
		printf("Failed to create eviction set\n");
		free(buffer);
		fclose(f);
		return;
	}
	
	if(touch == 1)
	{
		c[offset] = nop;
	}

	for(i = 0; i < 10; i++)
	{
		if(evict_itlb(ev_set, ev_set_size, offset) < 0)
		{
			printf("Failed to evict TLB\n");
			free(buffer);
			fclose(f);
			munmap((void*)ev_set, ev_set_size);
			return;
		}
		
		// evict cache
		for(j = 0; j<maxj; j++)
		{
			buffer[j] = rand();
		}

		if(touch == 1)
		{
			c[offset]();
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
			free(buffer);
			fclose(f);
			munmap((void*)ev_set, ev_set_size);
			return;
		}
	}

	free(buffer);
	fclose(f);
	munmap((void*)ev_set, ev_set_size);
}

unsigned long get_phys_addr(volatile fp *buffer, off_t buffer_size)
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
	off_t buffer_size = 1UL << 40; // >1 TB
	volatile fp *buffer = (fp*)mmap(NULL, (size_t) buffer_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	if(buffer == MAP_FAILED)
	{
		perror("Failed to map memory.");
		return -1;
	}
	
	//get_phys_addr(buffer, buffer_size);

	// TODO store convenient instructions in 512(?) page offsets in buffer

	profile_mem_access(buffer, 0, "uncached.txt");
	profile_mem_access(buffer, 1, "hopefully_cached.txt");

	//profiling(buffer, "cached.txt", "uncached.txt", 0);

	return 0;
}
