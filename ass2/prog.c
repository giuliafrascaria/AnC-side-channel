#include<errno.h>
#include<fcntl.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>

unsigned long get_page_table_root(){
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
	
	page = (unsigned long*) mmap(NULL,  4 * 1024, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, page_phys_addr);
	
	if(page == NULL)
	{
		return 0;
	}
	
	ret = page[offset] & 0xfffffffffffff000;
	
	munmap((void*) page, 4 * 1024);
	
	return ret;
}

void profile_mem_access(volatile char* c, int touch, char* filename)
{
	int i,k;
	unsigned long long hi1, lo1;
	unsigned long long hi, lo;
	uint64_t t, old, new;
	off_t j, buf_size = 64 * 1024 * 1024, maxj = (64 * 1024 * 1024) / sizeof(int);
	int* buffer = (int*)malloc(buf_size);
	volatile char *p;

	FILE *f = fopen(filename, "ab+");

	if(f == NULL)
	{
		perror("Failed to open file for printing memory access profiles");
		return;
	}

	for(i = 0; i < 1000; i++)
	{
		for(j = 0; j<maxj; j++)
		{
			buffer[j] = rand();
		}
		
		if(touch == 1)
		{
			p = c;
			
			// assuming 8-way associative iTLB with 64 entries
			// assuming page sizes of 4KB
			for(k = 0; k < 512; k += 4096)
			{
				//execute "convenient" instruction stored at offset k in p
			}
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
			fclose(f);
			perror("Failed to print memory access");
		}
	}

	fclose(f);
}

int main(int argc, char* argv[])
{	
	off_t buffer_size = 1UL << 40; // >1 TB
	volatile char *buffer = (char*)mmap(NULL, (size_t) buffer_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	unsigned long buffer_address = (unsigned long) &buffer;
	unsigned long page_table_offset;
	unsigned long page_table_offset_mask = 0x1ff; // last 9 bits
	unsigned long frame_offset_mask = 0xfff; // last 12 bits
	unsigned long aux_phys_addr;
	int fd;
	

	if(buffer == MAP_FAILED)
	{
		perror("Failed to map memory.");
		return -1;
	}
	
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
	
	// frame
	printf("frame physical address: 0x%lx\n", aux_phys_addr);
	page_table_offset = buffer_address & frame_offset_mask;
	aux_phys_addr = get_physical_addr(fd, aux_phys_addr, page_table_offset);
	printf("buffer physical address: 0x%lx\n", aux_phys_addr);
	
	// TODO store convenient instructions in 512(?) page offsets in buffer
	
	profile_mem_access(buffer, 0, "uncached.txt");
	profile_mem_access(buffer, 1, "hopefully_cached.txt");

	return 0;
}
