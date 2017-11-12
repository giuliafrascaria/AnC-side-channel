#include<errno.h>
#include<fcntl.h>
#include<stdint.h>
#include<stdio.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>

static inline void flush(volatile char *p)
{
	asm volatile("clflush 0(%0)\n" : : "c"(p) : "rax");
}

static inline uint64_t access_memory(volatile char *p)
{
	unsigned long long hi1, lo1;
	unsigned long long hi, lo;
	uint64_t t, old, new;

	asm volatile ("mfence\n\t"
						"CPUID\n\t"
						"RDTSC\n\t"
						"mov %%rdx, %0\n\t"
						"mov %%rax, %1\n\t" : "=r"(hi1), "=r"(lo1) : : "%rax", "%rbx", "%rcx", "%rdx");

	asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");

	asm volatile ("RDTSCP\n\t"
						"mov %%rdx, %0\n\t"
						"mov %%rax, %1\n\t"
						"CPUID\n\t"
						"mfence" : "=r"(hi), "=r"(lo) : : "%rax", "%rbx", "%rcx", "%rdx");

	old = (uint64_t) (hi1 << 32) | lo1;
	new = (uint64_t) (hi << 32) | lo;

	uint64_t ret = new - old;
	return ret;

}

// static inline uint64_t rdtscp()
// {
//
//
// 	return ((uint64_t) hi << 32) | lo;
// }
//
// static inline uint64_t rdtscp2()
// {
//
//
// 	return ((uint64_t) hi << 32) | lo;
// }

void profile_mem_access(volatile char *buffer, off_t buffer_size, int cached, char *filename)
{
	int i;


	FILE *f = fopen(filename, "ab+");

	if(f == NULL)
	{
		perror("Failed to open file for printing memory access profiles");
		return;
	}


	unsigned long long hi1, lo1;
	unsigned long long hi, lo;
	uint64_t t, old, new;


	for(i = 0; i < 1000; i++)
	{
		if(cached == 0)
		{
			//flush(buffer);
			asm volatile("clflush 0(%0)\n" : : "c"(buffer) : "rax");
		}

		//old = rdtscp();
		//uint64_t t = access_memory(buffer);
		//new = rdtscp2();
		//t = new - old;


		asm volatile ("mfence\n\t"
							"CPUID\n\t"
							"RDTSC\n\t"
							"mov %%rdx, %0\n\t"
							"mov %%rax, %1\n\t" : "=r"(hi1), "=r"(lo1) : : "%rax", "%rbx", "%rcx", "%rdx");

		asm volatile("movq (%0), %%rax\n" : : "c"(buffer) : "rax");

		asm volatile ("RDTSCP\n\t"
							"mov %%rdx, %0\n\t"
							"mov %%rax, %1\n\t"
							"CPUID\n\t"
							"mfence" : "=r"(hi), "=r"(lo) : : "%rax", "%rbx", "%rcx", "%rdx");

		old = (uint64_t) (hi1 << 32) | lo1;
		new = (uint64_t) (hi << 32) | lo;

		uint64_t ret = new - old;
		//return ret;

		if(fprintf(f, "%llu\n", (unsigned long long) ret) < 0)
		{
			fclose(f);
			perror("Failed to print memory access");
		}
	}

	fclose(f);
}

int profile_memory()
{
	off_t buffer_size = 64 * 1024 * 1024; // 64 MB
	volatile char *buffer = (char*)mmap(NULL, (size_t) buffer_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if(buffer == MAP_FAILED)
	{
		perror("Failed to map memory.");
		return -1;
	}

	profile_mem_access(buffer, buffer_size, 0, "uncached.txt");
	profile_mem_access(buffer, buffer_size, 1, "cached.txt");

	if(munmap((void*) buffer, (size_t) buffer_size) < 0)
	{
		perror("Failed to unmap memory.");
		return -1;
	}

	return 0;
}

int alloc_mem()
{
	off_t buffer_size = 1UL << 40; // 1 TB
	volatile char *buffer = (char*)mmap(NULL, (size_t) buffer_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if(buffer == MAP_FAILED)
	{
		perror("Failed to map memory.");
		return -1;
	}

	printf("Buffer address: %p\n", (void*) buffer);

	if(munmap((void*) buffer, (size_t) buffer_size) < 0)
	{
		perror("Failed to unmap memory.");
		return -1;
	}

	return 0;
}

int print_phys_mem()
{
	int i;
	int phys_mem_fd;
	off_t phys_mem_offset = 1UL << 30; // 1GB
	unsigned char c;

	phys_mem_fd = open("/dev/mem", O_RDONLY);

	if(phys_mem_fd < 0)
	{
		perror("Failed to open /dev/mem");
		return -1;
	}

	if(lseek(phys_mem_fd, phys_mem_offset, SEEK_SET) < 0)
	{
		perror("Failed to seek in /dev/mem");
		return -1;
	}

	// print 10 bytes from physical memory at address >=1GB
	for(i = 0; i < 10; i++)
	{
		if(read(phys_mem_fd, &c, sizeof(c)) < 0)
		{
			perror("Failed to read from /dev/mem");
			close(phys_mem_fd);
			return -1;
		}

		printf("<0x%x>", c);
	}
	printf("\n");
	close(phys_mem_fd);

	return 0;
}

int print_page_table_root(){
	char buffer[8];
	FILE* f = fopen("/proc/cr3", "r");
	if(f == NULL){
		return -1;	
	}
        size_t root = fread(buffer, 1, 8, f);
        if(root < 0) {
		fclose(f);
		return -1; 	
	}
	printf("CR3 value is: %p\n", buffer);
	fclose(f);
	return 0;
}

int main(int argc, char* argv[])
{
	if(alloc_mem() < 0)
	{
		return 1;
	}

	if(profile_memory() < 0)
	{
		return 1;
	}

	if(print_phys_mem() < 0)
	{
		return 1;
	}

	if(print_page_table_root() < 0)
	{
		return 1;
	}

	return 0;
}
