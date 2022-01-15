#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
int main(){
	printf("Hello, world\n");
	int fd = open("/dev/lab2_device", O_RDWR);
	char buf[4];
	if(fd < 0) printf("No file\n");
	int i = write(fd, "play", 4);
	printf("%d\n", i);
	i = read(fd, buf, 4);
	printf("%d\n", i);
	printf("%s\n", buf);
	printf("%c\n", buf[1]);
	close(fd);
	return 0;
}
