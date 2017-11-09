#include<errno.h>
#include<fcntl.h>
#include<stdio.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>

int main(int argc, char* argv[])
{
	off_t buffer_size = 1UL << 40; // 1 TB
	off_t phys_mem_offset = 1UL << 30; // 1GB
	char *buffer = (char*)mmap(NULL, (size_t) buffer_size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	int phys_mem_fd;
	int i;
	unsigned char c;
	
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
	
	return 0;
}

