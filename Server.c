#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>   //socket, setsockop
#include <unistd.h>
#include <netinet/in.h>   //struct sockaddr_in
#include <netinet/tcp.h>  //TCP_NODELAY
#include <signal.h>
#include "extFunc.c"
#include <string.h>

extern void sig_proccess(int signo);
extern void process_conn_server(int s);

#define SERVERPORT 8888       /* 侦听端口地址 */
#define BACKLOG 2       /* 侦听队列长度 */

int main(int argc, char *argv[])
{
    int serSock,cliSock, tcpFlag;      /* serSock为服务器的socket描述符,cliSock为客户端的socket描述符 */
    struct sockaddr_in server_addr; /* 服务器地址结构 */
    struct sockaddr_in client_addr; /* 客户端地址结构 */
    int err;    /* 返回值 */
    pid_t pid1,pid2;  /* 分叉的进行id */

    signal(SIGINT, sig_proccess);
    signal(SIGPIPE, sig_proccess);

    //fork a process to monitor system performance
    pid1 = fork();       /* 分叉进程 */
    if ( pid1 < 0 )  /* 在父进程中关闭客户端的连接 */
    {
        /* code */
        perror("fork process error");
        return -1;
    };

    if( pid1 == 0 )  /* 子进程中 */
    {     
        monitor_workload();
      
        return 0;
    };


    /* 建立一个流式套接字 */
    //UPD:SOCK_DGRAM   TCP:SOCK_STREAM
    serSock = socket(AF_INET, SOCK_STREAM, 0);
    if(serSock < 0){/* 出错 */
        perror("socket error");
        return -1;  
    };

    /* Disable the Nagle (TCP No Delay) algorithm */
    err = optimi_socket(serSock);
    if (err < 0) {
      perror("Couldn't optimize setting for serSock.)");
      return -1;
    };


    /* 设置服务器地址 */
    bzero(&server_addr, sizeof(server_addr));   /* 清0 */
    server_addr.sin_family = AF_INET;           /* 协议族 */
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);/* 本地地址 */
    server_addr.sin_port = htons(SERVERPORT);         /* 服务器端口 */

    /* 绑定地址结构到套接字描述符 */
    err = bind(serSock, (struct sockaddr*)&server_addr, sizeof(server_addr));
    if(err < 0){/* 出错 */
        perror("bind error");
        return -1;  
    }

    /* 设置侦听 */
    err = listen(serSock, BACKLOG);
    if(err < 0){/* 出错 */
        perror("listen error");
        return -1;  
    }



        unsigned int addrlen = sizeof(struct sockaddr);

    /* 主循环过程 */
    for(;;) 
    {
        /* 接收客户端连接 */
        cliSock = accept(serSock, (struct sockaddr*)&client_addr, &addrlen);
        if(cliSock < 0){     /* 出错 */
            continue;   /* 结束本次循环 */
        };

        err = optimi_socket(cliSock);
        if (err < 0) {
          perror("Couldn't optimize setting for cliSock.)");
          return -1;
        };


        /* 建立一个新的进程处理到来的连接 */
        pid2 = fork();       /* 分叉进程 */
        if( pid2 == 0 )  /* 子进程中 */
        {     
            close(serSock);      /* 在子进程中关闭服务器的侦听 */
            process_conn_server(cliSock);/* 处理连接 */
            return 0;
        }else if ( pid2 > 0 )  /* 在父进程中关闭客户端的连接 */
        {
            /* code */
            close(cliSock);  
        }else 
        {
             perror("fork process error");   
        }

    }



}