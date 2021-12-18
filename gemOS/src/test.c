#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<signal.h>
#include<fcntl.h>
#include<assert.h>
#include<stdlib.h>
#include<string.h>
int main()
{
    // int fd1=open("file.c",O_RDWR);
    // int fd2=open("pipe.c",O_RDWR);
    // close(2);
    // int x=dup(2);
    // int y=dup(2);
    // printf("%d %d\n",x,y);
    int fd[2];
    pipe(fd);
    write(fd[1],"Rakesh Tetarwal",strlen("Rakesh Tetarwal"));
    lseek(fd[0],SEEK_CUR,5);
    char buff[100];
    read(fd[0],buff,10);
    printf("%s\n",buff);

}