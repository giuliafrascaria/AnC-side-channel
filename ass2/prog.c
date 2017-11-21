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

typedef struct
{
	unsigned long address;
	unsigned long offset;
} Page;

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

int evict_itlb(volatile unsigned char *buffer, size_t size)
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
		maxj = i + PAGE_SIZE - 1;
		
		for(j = i; j < maxj; j++)
		{
			buffer[j] = 0x90;	
		}
		
		buffer[maxj] = 0xc3;
		ptr = (fp)(&(buffer[i]));
		
		ptr();
	}
	
	return 0;
}

volatile unsigned long* get_pointer_to_pte(Page page)
{
	volatile unsigned long* ret;
	int fd = open("/dev/mem", O_RDONLY);
	
	if(fd < 0)
	{
		perror("Failed to open /dev/mem");
		return NULL;
	}
	
	ret = (unsigned long*) mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, page.address);
	
	return ret;
}

void profile_mem_access(volatile unsigned char* c, volatile unsigned long* pte, int* buffer, size_t cache_flush_set_size, volatile unsigned char* ev_set, size_t ev_set_size, int touch, char* filename)
{
	int i, j, k;
	unsigned long long hi1, lo1;
	unsigned long long hi, lo;
	uint64_t t, old, new;
	int maxj = cache_flush_set_size / sizeof(int);
	fp ptr; // pointer to function stored in the target buffer
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
		ptr = (fp)&(c[0]);
	}

	for(i = 0; i < 100; i++)
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
			ptr(); // execute function in target buffer
		}

		asm volatile ("mfence\n\t"
							"CPUID\n\t"
							"RDTSC\n\t"
							"mov %%rdx, %0\n\t"
							"mov %%rax, %1\n\t" : "=r"(hi1), "=r"(lo1) : : "%rax", "%rbx", "%rcx", "%rdx");

		asm volatile("movq (%0), %%rax\n" : : "c"(pte) : "rax");

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

Page* get_phys_addr(volatile unsigned char *buffer, off_t buffer_size)
{
	unsigned long buffer_address;
	unsigned long page_table_offset_mask = 0x1ff; // last 9 bits
	unsigned long frame_offset_mask = 0xfff; // last 12 bits
	Page* physical_addresses;
	int fd;
	
	if(buffer == NULL)
	{
		printf("Cannot get physical address of null pointer\n");
		return NULL;
	}
	
	buffer_address = (unsigned long) &buffer;
	
	// array containing 4 physical addresses of PT pages and the translated VA
	physical_addresses = (Page*)malloc(5 * sizeof(Page));
	
	if(physical_addresses == NULL)
	{
		perror("Failed to allocate memory for storing physical addresses");
		return NULL;
	}

	fd = open("/dev/mem", O_RDONLY);
	
	if(fd < 0)
	{
		perror("Failed to open /dev/mem");
		free(physical_addresses);
		return NULL;
	}
	
	if((physical_addresses[0].address = get_page_table_root()) < 0)
	{
		return NULL;
	}

	printf("buffer virtual address: 0x%lx\n", buffer_address);

	// level 4
	printf("PT4 physical address: 0x%lx\n", physical_addresses[0].address);
	physical_addresses[0].offset = (buffer_address >> 39) & page_table_offset_mask;
	physical_addresses[1].address = get_physical_addr(fd, physical_addresses[0].address, physical_addresses[0].offset);

	// level 3
	printf("PT3 physical address: 0x%lx\n", physical_addresses[1].address);
	physical_addresses[1].offset = (buffer_address >> 30) & page_table_offset_mask;
	physical_addresses[2].address = get_physical_addr(fd, physical_addresses[1].address, physical_addresses[1].offset);

	// level 2
	printf("PT2 physical address: 0x%lx\n", physical_addresses[2].address);
	physical_addresses[2].offset = (buffer_address >> 21) & page_table_offset_mask;
	physical_addresses[3].address = get_physical_addr(fd, physical_addresses[2].address, physical_addresses[2].offset);

	// level 1
	printf("PT1 physical address: 0x%lx\n", physical_addresses[3].address);
	physical_addresses[3].offset = (buffer_address >> 12) & page_table_offset_mask;
	physical_addresses[4].address = get_physical_addr(fd, physical_addresses[3].address, physical_addresses[3].offset);

	physical_addresses[4].offset = buffer_address & frame_offset_mask;
	printf("buffer physical address: 0x%lx\n", physical_addresses[4].address | physical_addresses[4].offset);
	
	return physical_addresses;
}

int main(int argc, char* argv[])
{
	size_t target_size = 1UL << 40; // >1 TB
	size_t cache_flush_set_size = 10 * 1024 * 1024; // 10MB
	size_t ev_set_size = 8192 * PAGE_SIZE; // 1024 TLB entries
	int * cache_flush_set;
	volatile unsigned char *ev_set;
	Page* pages;
	volatile unsigned long* page_ptr;
	volatile unsigned long* pte;
	volatile unsigned char *target = (unsigned char*)mmap(NULL, target_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
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
	
	ev_set = (unsigned char*)mmap(NULL, ev_set_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	if(ev_set == NULL)
	{
		perror("Failed to initialize TLB eviction set");
		munmap((void*) target, target_size);
		free(cache_flush_set);
		return -1;
	}
	
	pages = get_phys_addr(target, target_size);
	
	if(pages == NULL)
	{
		printf("Failed to get physical address of target VA\n");
		munmap((void*) target, target_size);
		munmap((void*) ev_set, ev_set_size);
		free(cache_flush_set);
	}
	
	page_ptr = get_pointer_to_pte(pages[2]); // using PTL2 to trigger side effect
	
	if(page_ptr == NULL)
	{
		printf("Failed to map target page table page\n");
		munmap((void*) target, target_size);
		munmap((void*) ev_set, ev_set_size);
		free(cache_flush_set);
		free(pages);
	}
	
	pte = page_ptr + pages[2].offset;

	profile_mem_access(target, pte, cache_flush_set, cache_flush_set_size, ev_set, ev_set_size, 0, "uncached.txt");
	profile_mem_access(target, pte, cache_flush_set, cache_flush_set_size, ev_set, ev_set_size, 1, "hopefully_cached.txt");
	
	free((void*)pte);
	munmap((void*)page_ptr, PAGE_SIZE);
	munmap((void*) target, target_size);
	munmap((void*) ev_set, ev_set_size);
	free(cache_flush_set);
	free(pages);

	return 0;
}
