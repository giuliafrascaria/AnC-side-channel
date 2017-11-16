#include<errno.h>
#include<fcntl.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>

void profile_mem_access(volatile char* c, int cached, char* filename)
{
	int i;
	unsigned long long hi1, lo1;
	unsigned long long hi, lo;
	uint64_t t, old, new;
	off_t j, buf_size = 64 * 1024 * 1024, maxj = (64 * 1024 * 1024) / sizeof(int);
	int* buffer = (int*)malloc(buf_size);

	FILE *f = fopen(filename, "ab+");

	if(f == NULL)
	{
		perror("Failed to open file for printing memory access profiles");
		return;
	}

	for(i = 0; i < 1000; i++)
	{
		if(cached == 0)
		{
			for(j = 0; j<maxj; j++)
			{
				buffer[j] = rand();
			}
//			asm volatile("clflush 0(%0)\n" : : "c"(c) : "rax");
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

int profile_memory()
{
	volatile char* c = (char*)malloc(sizeof(char));

	if(c == NULL)
	{
		perror("Failed to allocate memory for profiling.");
		return -1;
	}

	profile_mem_access(c, 0, "uncached.txt");
	profile_mem_access(c, 1, "cached.txt");

	free((void*)c);

	return 0;
}

char* alloc_mem()
{
	off_t buffer_size = 1UL << 40; // >1 TB
	volatile char *buffer = (char*)mmap(NULL, (size_t) buffer_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if(buffer == MAP_FAILED)
	{
		perror("Failed to map memory.");
		return -1;
	}

	//printf("Buffer address: %p\n", (void*) buffer);

	/*if(munmap((void*) buffer, (size_t) buffer_size) < 0)
	{
		perror("Failed to unmap memory.");
		return -1;
	}*/
	return buffer;
	
}


unsigned long get_page_table_root(){
	unsigned long val;

	FILE* f = fopen("/proc/cr3", "rb");

	if(f == NULL)
	{
		return -1;
	}
	
	if(fscanf(f, "%lu\n", &val) < 0)
	{
		fclose(f);
		return -1;
	}

	printf("CR3 value is: %lx\n", val);
	fclose(f);

	return val;
}

unsigned long createMask(unsigned long a, unsigned long b)
{
   unsigned long r = 0;
   for (unsigned i=a; i<=b; i++)
       r |= 1 << i;

   return r;
}

int main(int argc, char* argv[])
{	
	unsigned long cr3, buffer_address;
	off_t buffer_size = 1UL << 40; // >1 TB
	volatile char *buffer = (char*)mmap(NULL, (size_t) buffer_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	buffer_address = &buffer;

	if(buffer == MAP_FAILED)
	{
		perror("Failed to map memory.");
		return -1;
	}

	/*if(alloc_mem(buffer_address) < 0)
	{
		return -1;
	}

	if(profile_memory() < 0)
	{
		return -1;
	}*/

	if((cr3 = get_page_table_root()) < 0)
	{
		return -1;
	}
	
	printf("Buffer address 0x%lx\n", buffer_address);
	printf("Cr3 0x%lx\n", cr3);
	int fd = open("/dev/mem", O_RDWR);
	unsigned long* page_table_root = (unsigned long*)mmap(NULL,  4 * 1024, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, cr3);
	printf("Root is %p\n", page_table_root);
	if(page_table_root == NULL){
		perror("BLA");	
	}
	printf("mask is 0x%lx\n", createMask(40, 49));
	unsigned long index_in_ptl4 = 0;
	//unsigned long ptl3_phys_addr = page_table_root[index_in_ptl4];
	
	printf("PLT4 index is %lu\n", buffer_address & createMask(40, 49));
/*
	printf("PLT3 table address is 0x%lx\n", ptl3_phys_addr);
	*/

	return 0;
}
