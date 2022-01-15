#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(){
	printf("Hello, world\n");
	int fd = open("/dev/lab2_device", O_RDWR);
	char buf[15];
	int pid = fork( );
	if(fd < 0) printf("No file\n");
	if (pid < 0)
	{
		printf("No processes\n");
		return 0;
	}
	else if (pid > 0)
	{
		write(fd, "ploho", 5);
		write(fd, "player win game", 15);
		wait(0);
		read(fd, buf, 5);
		printf("%s\n", buf);
		close(fd);
	}
	else
	{
		read(fd, buf, 5);
		printf("%s\n", buf);
		read(fd, buf, 15);
		printf("%s\n", buf);
		write(fd, "ploho", 5);
	}

	return 0;
}
