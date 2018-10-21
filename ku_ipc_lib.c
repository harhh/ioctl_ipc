#include<stdio.h> 
#include<sys/fcntl.h>
#include<sys/ioctl.h>
#include<string.h>
#include<stdlib.h>

#define TEST_IOCTL_NUM 'z'
#define IOCTL_START_NUM 0x80

//define cmd
#define IOCTL_MSG_RECEIVE       IOCTL_START_NUM+1	
#define IOCTL_MSG_SEND		IOCTL_START_NUM+2	
#define IOCTL_CH_OPEN		IOCTL_START_NUM+3	
#define IOCTL_CH_CLOSE		IOCTL_START_NUM+4
//define ioctl
#define MSG_SEND _IOWR(TEST_IOCTL_NUM, IOCTL_MSG_SEND, unsigned long *)
#define MSG_RECEIVE _IOWR(TEST_IOCTL_NUM, IOCTL_MSG_RECEIVE, unsigned long *)
#define CH_OPEN _IOWR(TEST_IOCTL_NUM, IOCTL_CH_OPEN, unsigned long *)
#define CH_CLOSE _IOWR(TEST_IOCTL_NUM, IOCTL_CH_CLOSE, unsigned long *)

#define DEV_NAME "/dev/hong_ioctl_dev" 
typedef struct _data {
	void *data;
	int ch;
}DATA;

int ku_ipc_open(int ch) {
	
	int dev, ret;
	DATA value;
	value.ch = ch;
	value.data ="ssss";
	dev = open(DEV_NAME, O_RDWR);	//ioctl : CH_OPEN
	if (dev < 0) {	
		printf("open device failed.!\n");
		return dev;					
	}
	else {
		ret = ioctl(dev,CH_OPEN, &value);
		if (ret < 0) {			
			printf("ioctl:CH_OPEN failed.!\n");
			return 0;
		}
		else {		
			return dev;
		}
	} 
}

int ku_ipc_close(int fd, int ch) {
	int ret;
	DATA value;
	value.ch = ch;
	ret = ioctl(fd, CH_CLOSE, &value);	//ioctl : CH_CLOSE
	//return 1 :success
	//return 0 : fail
	if (ret < 0) {
		printf("ioctl:CH_CLOSE failed.!\n");
		return 0;
	}
	else {	
		return ret;	
	} 

}

int ku_ipc_send(int fd, int ch, void *buf, int len) {
	int ret;
	DATA *value;
	value->ch = ch;
	value->data = buf;
	ret = ioctl(fd, MSG_SEND, value);	//ioctl : MSG_SEND
	
	if (ret < 0) {
		printf("ioctl:CH_CLOSE failed.!\n");		
		return -1;
	} 
	
	return len;								
	//return n : lengh
	//return 0 : no space
	//return -1 : error
}

int ku_ipc_recv(int fd, int ch, void*buf, int len) {
	int ret;
	DATA *value;
	printf("i.!\n");
	value->ch = ch;
	value->data = buf;

	ret = ioctl(fd, MSG_RECEIVE, value);	//ioctl : MSG_RECEICE
														
	if (ret < 0) {
		printf("ioctl:MSG_RECEIVE failed.!\n");
		return 0;
	} 

	return ret; 
	//return n : lengh
	//return -1 : error
				
}


void main(void) {

int i=0;
int ret=0;

	for(i;i<10;i++){
 	ret = ku_ipc_open(i);
	printf("open :dev=%d\n", ret);
	ret = ku_ipc_close(ret,i);	
	printf("close=%d\n", ret);
	} 

return;

}
	
