#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/select.h>

#include "list.h"

#define MAXFD 3	

pthread_mutex_t  mutex;
sem_t sem;

struct node *head = NULL;

int create_sockfd();
void * pthread_fun( void *arg);
void create_threads();
void send_file(int c, char *s);
void recvf_ile(int c,char *s);


void sig_fun(int sig)
{
	printf("sig = %d\n ",sig);
}
int main()
{
	signal(SIGPIPE,sig_fun);
	pthread_mutex_init(&mutex,NULL);
	sem_init(&sem,0,0);
	
	list_init(&head);

	int sockfd = create_sockfd();

	create_threads();
	while(1)
	{
		struct sockaddr_in caddr;
		int len = sizeof(caddr);

		int c = accept(sockfd,(struct sockaddr*)&caddr,&len);
		if( c < 0)
		{
			continue;
		}
		
		pthread_mutex_lock(&mutex);
		list_add(head,c);
		pthread_mutex_unlock(&mutex);

		printf("accept c=%d\n",c);
     		
		sem_post(&sem);//11	//v
	
	}
}

void send_file(int c, char *s)
{
	int fd = open(s,O_RDONLY);
	if(fd == -1)
	{
		send(c,"err",3,0);
		return;
	}
	struct stat st;
	lstat(s,&st);
	char buff[128] = {"ok#"};
	sprintf(buff+3,"%d",st.st_size);
	send(c,buff,strlen(buff),0);
		
	char cli_stat[32] = {0};
	if(recv(c,cli_stat,31,0)<=0)//对方关闭
	{
		return;
	}
	if(strncmp(cli_stat,"ok",2)!=0)//回复的信息不对
	{
		return;
	}
	char recvbuff[256] = {0};
	int n = -1;
	
	while((n = recv(c,recvbuff,256,0))>0)
	{
		if(write(fd,recvbuff,n)<=0)
		{
			perror("send error");
			return;
		}
	} 
	printf("send file finish\n");
	return;
	
	/////////////////////////////////////////
/* 	fd_set fdsetr,fdsetw;
	
	while(1)
	{
		FD_ZERO(&fdsetr);
		FD_ZERO(&fdsetw);
		
		FD_SET(c,fdsetr);
		FD_SET(c,fdsetw);
		
		n = select(c+1,fdsetr,fdsetw,NULL,NULL);//永久阻塞
		if(n < 0)
		{
			
		}
		else if( n == 0)
		{
			printf("time out\n");
		}
		else
		{
			if(FD_ISSET(c,&fdsetr))
			{
				char buff[128] = {0};
				if ( recv(c,buff,127,0) == 0)
				{
					printf("client over\n");
					break;
				}
			}
			if(FD_ISSET(c,&fdsetw))
			{
				int num = read(c,sendbuff,256);
				if(num == 0)
				{
						printf("send file finish\n");
						break;
				}
				send(c,sendbuff,num,0);
			
			}
			
		}
	} */
	
}

void create_threads()
{
	pthread_t id[MAXFD];
	int i=0;
	for(; i<MAXFD;i++)
	{
		pthread_create(&id[i],NULL,pthread_fun,NULL);
	}
}

void recv_file(int c,char *s)
{
	char buff[32] = {0};
	recv(c,buff,31,0);
	if(strcmp(buff,"err") == 0)
	{
		return;
	}
	send(c,"ok",2,0);

	int fd = open(s,O_WRONLY|O_CREAT,0600);
	if(fd == -1)
	{
		send(c,"err",3,0);
		return;
	}
	
	send(c,"ok",2,0);
	
	char recvbuff[1024] = {"ok#"};
	
	int fsize = 0;
	recv(c,recvbuff,256,0);
	sscanf(recvbuff+3,"%d",&fsize);
	
	
	int cursize = 0;
	int n = -1;
	
	while(1)
	{
		n = recv(c,recvbuff,1024,0);
		cursize+=n;
		if(n <= 0)//对方可能关闭
		{
			break;
		}
		write(fd,recvbuff,n);
		if(cursize == fsize)
		{
			printf("file recv success\n");
			send(c,"fileok",6,0);
			break;
		}
	}
	
	close(fd);
}
void *pthread_fun(void *arg)
{
	printf("pthread run\n");
	
	while(1)
	{
		sem_wait(&sem);//p
		pthread_mutex_lock(&mutex);
		int c = list_get(head);
		pthread_mutex_unlock(&mutex);
		printf("c=%d\n",c);
		while(1)
		{
			char buff[128] = {0};
		  
			int n = recv(c,buff,127,0);
			if(n == 0)
			{
				printf("ont client over\n");
				close(c);
				break;
			}

			char *myargv[10] = {0};
			char *sp = NULL;
			char *s = strtok_r(buff," ",&sp);
			if(s == NULL)
			{
				send(c,"ok#",3,0);
				continue;
			}
			
			int i = 0;
			while(s != NULL)
			{
				myargv[i++] = s;
				s = strtok_r(NULL," ",&sp);
			}

			if(strcmp(myargv[0],"get") == 0)
			{
				send_file(c,myargv[1]);
			}
			else if(strcmp(myargv[0],"put") == 0)
			{
				recv_file(c,myargv[1]);
				continue;
			}
			else
			{
			//ls ,cp
				int pipefd[2];
				pipe(pipefd);

				pid_t pid = fork();
				assert(pid != -1);

				if(pid == 0)
				{
					close(pipefd[0]);
					dup2(pipefd[1],1);
					dup2(pipefd[1],2);

					execvp(myargv[0],myargv);
					perror("execvp error");
					exit(0);
				}
				
				close(pipefd[1]);
				wait(NULL);
				char readbuff[1024] = {"ok#"};
				read(pipefd[0],readbuff+3,1000);
				send(c,readbuff,strlen(readbuff),0);
			}
		}
	}
}


int create_sockfd()
{
	int sockfd = socket(AF_INET,SOCK_STREAM,0);
	assert(sockfd != -1);

	struct sockaddr_in saddr;
	memset(&saddr,0,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(6000);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int res = bind(sockfd,(struct sockaddr *)&saddr,sizeof(saddr));
	assert(res != -1);

	listen(sockfd,5);

	return sockfd;
}
