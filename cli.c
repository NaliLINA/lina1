#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void  down_file(int c, char *s);
void  up_file(int c, char *s);

int main()
{
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    assert(sockfd != -1);

    struct sockaddr_in saddr,caddr;
    memset(&saddr,0,sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(6000);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int res = connect(sockfd,(struct sockaddr*)&saddr,sizeof(saddr));
    assert(res != -1);
   
    while(1)
    {
		char buff[128] = {0};
		write(1,"Con>>",5);
		fgets(buff,128,stdin);

		if(strcmp(buff,"\n") == 0)
		{
			continue;
		}
		buff[strlen(buff)-1] = 0;
		if(strncmp(buff,"end",3)==0)
		{ 
			break;
		}
		else if(strncmp(buff,"get",3) == 0)
		{
			down_file(sockfd,buff);
		} 
		else if(strncmp(buff,"put",3) == 0)
		{
			up_file(sockfd,buff);
			continue;
		}
		else
		{
			send(sockfd,buff,strlen(buff),0);
			char recvbuff[1024] = {0};
			recv(sockfd,recvbuff,1023,0);
			if(strncmp(recvbuff,"ok#",3) != 0)
			{
				printf("cmd data error\n");
				break;
			}	
			printf("%s\n",recvbuff+3);
		
		}
	} 
	close(sockfd);		
}

void up_file(int c, char *s)
{
	send(c,s,strlen(s),0);
	
	char buff[128] = {0};
	strcpy(buff,s);
	
	char *p = strtok(buff," ");
	p = strtok(NULL," ");
	
	if(p == NULL)//没输文件名
	{
		return;
	}
	
	printf("p=%s\n",p);
	int fd = open(p,O_RDONLY);
	

	send(c,"ok",2,0);
	
	char ser_stat[32] = {0};
	
	recv(c,ser_stat,31,0);
	if(strncmp(ser_stat,"ok",2)!=0)//回复的信息不对
	{
		return;
	}
	
	recv(c,ser_stat,31,0);
	if(strncmp(ser_stat,"err",3)==0)
	{
		return;
	}
	
	
	struct stat st;
	lstat(p,&st);
	
	char buffsize[128] = {"ok#"};
	sprintf(buffsize+3,"%d",st.st_size);
	send(c,buffsize,strlen(buffsize),0);
	
	int fsize = (int ) st.st_size;
	
	char sendbuff[1024] = {"ok#"};
	int n = 0;
	int cur_size = 0;
	float f = 0.0;
	
	printf("\033[?25l");
	while(1)
	{
		n = read(fd,sendbuff,1024);
		if(n <= 0)
		{
			break;
		}
		send(c,sendbuff,n,0);
		cur_size +=n;
		
		f = cur_size *100.0/fsize;
		printf("downfile:%.1f\r",f);
		fflush(stdout);
		if(cur_size >= fsize)
		{
			char rs[128] = {0};
			recv(c,rs,1024,0);
			
			if(strcmp(rs,"fileok") == 0)
			{
				printf("file up success\n");
				break;
			}
			else
			{
				printf("error\n");
				break;
			}
		}
	}	
	close(fd);
	
	return;
}

void down_file(int c, char *s)
{
	
	char buff[128] = {0};
	strcpy(buff,s);
	
	char *p = strtok(buff," ");
	p = strtok(NULL," ");
	
	if(p == NULL)//没输文件名
	{
		return;
	}
	
	send(c,s,strlen(s),0);
	
	char ser_status[32] = {0};
	if(recv(c,ser_status,31,0) <= 0) //ser出错关闭
	{
		return;
	}
	
	if(strncmp(ser_status,"ok#",3) != 0 )
	{
		printf("file not exists\n");
		return;
	}
	
	
	int fsize = 0;//ok#234 只要234
	sscanf(ser_status+3,"%d",&fsize);
	printf("down file size:%d\n",fsize);
	int fd = open(p,O_WRONLY|O_CREAT,0600);
	if(fd == -1)
	{
		send(c,"err",3,0);
	}
	send(c,"ok",2,0);
	char recvbuff[256] = {0};
	int n = 0;
	int cur_size = 0;
	float f = 0.0;
	
	printf("\033[?25l");
	while(1)
	{
		n = recv(c,recvbuff,256,0);
		if(n <= 0)//对方可能关闭
		{
			break;
		}
		write(fd,recvbuff,n);
		cur_size +=n;
		f = cur_size *100.0/fsize;
		printf("downfile:%.1f\r",f);
		fflush(stdout);
		if(cur_size == fsize)
		{
			printf("file up ok\n");
			break;
		}
	}
	printf("\033[?25h\n");
	close(fd);
}