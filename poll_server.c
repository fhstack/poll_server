#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>

#define INIT -1
typedef struct pollfd pollfd;

int startup(int port)
{
    int listen_sock = socket(AF_INET,SOCK_STREAM,0);
    struct sockaddr_in addr;
    if(listen_sock < 0)
    {
        perror("socket");
        exit(1);
    }
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if(bind(listen_sock,(struct sockaddr*)&addr,sizeof(addr)))
    {
        perror("bind");
        return 2;
    }
    
    int opt = 1;
    setsockopt(listen_sock,SOL_SOCKET,SO_REUSEADDR, &opt,sizeof(opt));
    
    if(listen(listen_sock,5) < 0)
    {
        perror("listen");
        return 3;
    }

    return listen_sock;
}

void Init(pollfd* fdlist,int size)
{
    //printf("Init size:%d",size);
    int i = 0;
    for(;i < size;i++)
    {
        fdlist[i].fd = INIT;
        fdlist[i].events = 0;
        fdlist[i].revents = 0;
    }
    //初始化完成
}

int Add(pollfd* fdlist,int fd,int size)
{
    int i = 0;
    for(;i < size;i++)
    {
        if(fdlist[i].fd == INIT)
        {
            fdlist[i].fd = fd;
            fdlist[i].events = POLLIN;
            break;
        }
    }

    if(i == size)
        return 1;   //无法继续加入监听的描述符

    return 0;
}

void ServiceIO(pollfd* fdlist,int i)
{
    char buf[1024];

    ssize_t read_size = read(fdlist[i].fd,buf,sizeof(buf)-1);

    if(read_size < 0)
    {
        perror("read");
    }
    else if(read_size == 0)
    {
        printf("clietn close connect\n");
        fdlist[i].fd = INIT;
        close(fdlist[i].fd);
    }
    else
    {
        buf[read_size-1] = 0;
        printf("client: %s\n",buf);
        write(fdlist[i].fd,buf,strlen(buf));
    }
}

int main(int argc,char* argv[])
{
    if(argc != 2)
    {
        perror("Usage: ./poll_server [port]");
        return 1;
    }

    int listen_sock = startup(atoi(argv[1])); 
    
    pollfd fd_list[1024];

    Init(fd_list,sizeof(fd_list)/sizeof(fd_list[0]));
    
    Add(fd_list,listen_sock,sizeof(fd_list)/sizeof(fd_list[0]));
    
    for(;;)
    {
        int ret = poll(fd_list,sizeof(fd_list)/sizeof(fd_list[0]),1000);
        
        if(ret < 0)
        {
            perror("poll");
            continue;
        }

        if(ret == 0)
        {
            printf("timeout\n");
            continue;
        }

        //说明有时间就绪
        size_t i = 0;
        for(i = 0;i < sizeof(fd_list)/sizeof(fd_list[0]);i++)
        {
            //对就绪的描述符一一进行处理
            if(fd_list[i].fd == INIT)
                continue;
            
            //如果就绪的不是数据可读
            if(!(fd_list[i].revents & POLLIN))
                continue;

            //监听套接字有连接来了
            if(fd_list[i].fd == listen_sock)
            {
                printf("a link is comming\n");
                struct sockaddr_in client;
                socklen_t len = sizeof(client);
                int sock = accept(listen_sock,\
                                  (struct sockaddr*)&client\
                                  ,&len);
                if(sock < 0)
                {
                    perror("accpet");
                    continue;
                }
                printf("get a new link\n");
                Add(fd_list,sock,sizeof(fd_list)/sizeof(fd_list[0]));
            }
            else
            {
                ServiceIO(fd_list,i);
            }
        }
    }
}
