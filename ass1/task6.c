#include<errno.h>
#include<stdio.h>
#include<sys/mman.h>
#include<sys/types.h>

int main(int argc, char* argv[])
{
	off_t buffer_size = 1UL << 40; // 1 TB
	char *buffer = (char*)mmap(NULL, (size_t) buffer_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	
	if(buffer == MAP_FAILED)
	{
		perror("Failed to map memory.");
		return -1;
	}
	
	printf("Buffer address: %p\n", (void*) buffer);
	
	if(munmap(buffer, (size_t) buffer_size) < 0)
	{
		perror("Failed to unmap memory.");
		return -1;
	}
	
	return 0;
}

