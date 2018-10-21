#include<linux/init.h>
#include<linux/module.h>
#include<linux/kernel.h>
#include<asm/uaccess.h>

#include<linux/cdev.h>
#include<linux/device.h>
#include<linux/fs.h>
#include<linux/errno.h>
#include<linux/types.h>
#include<linux/fcntl.h>
#include<linux/list.h>
#include<linux/slab.h>
#include<linux/sched.h>
#include<linux/spinlock.h>
#include<asm/delay.h>

#include "ku_ipc.h"

MODULE_LICENSE("Dual BSD/GPL");

#define DEV_NAME "hong_ioctl_dev"	//device name

struct str_st{
	char str[128];
	int len;
}STR_ST;

typedef struct _data {
	void *data;
	int ch;
}DATA;

typedef struct msg_que {
	struct list_head list;
	void *data;
	int data_size;
}MSG_QUE;

typedef struct que_duplex {
	int que1_size;
	int que1_vol;
	int que2_size;
	int que2_vol;
	MSG_QUE msg_que1;
	MSG_QUE msg_que2;
}QUE_DUPLEX;

typedef struct my_user {
	int u_id;
	int u_ch;
}USER;

typedef struct channel {
	struct list_head list;
	QUE_DUPLEX *que_duplex;	
	USER *u1, *u2;
	int ch_num;	//ch id
	int user_count;	//process count
	int _valid	//valid or not
}CHANNEL;

typedef	struct channel_list {
	CHANNEL channel;
	int ch_count;
}CHANNEL_LIST;

// in document


CHANNEL_LIST ch_list;
spinlock_t my_lock;

void mdelay(int msec){
	int i, j;
	for(j=0; j<msec; j++) {
		for(i=0; i<1000; i++) {
			udelay(1000);
		}
	}
}

int find_ch(int ch, CHANNEL* tmp_ch) {//in document
	list_for_each_entry(tmp_ch, &ch_list.channel.list, list) {	
		if(tmp_ch->ch_num == ch) {
			if(tmp_ch->_valid ==1) return 3;//(3)channel exist but disavailable connect
			else {return 2;}		//(2)channel exist, available connect
		}
	}
	if(ch_list.ch_count == MAX_CHANNEL) {return 4;}//(4)channel not exist, channel_count is max
	return 1;	//(1)channel not exist, available create
}

static int hong_ioctl(struct file *file, unsigned int cmd, void *arg) {
	DATA *args = (DATA*)arg;//from app
	int ch = 0;
	int find_ch_result = 0;
	CHANNEL *tmp_ch = 0;	//for using list_for_each_entry
	CHANNEL *new_ch = 0;	//to create channel
	QUE_DUPLEX *que_du=0;	//to creat que_duplex
	MSG_QUE* tmp_msg = 0;	//to create msg_que	
	USER *u_tmp = 0;	//to create user
	int request_id = 0;	//pid
	int len = 0;		//sending message's size
	
	ch=args->ch;
	len = strlen(args->data);
	request_id = current->pid;
	find_ch_result = find_ch(ch, tmp_ch);	//var init

	printk("find result :%d", find_ch_result);//////
	printk("datasize:%d =", len*sizeof(*args->data));//////
	printk("data=%s", args->data);/////
	if(cmd <0) {
		printk("ioctl is not available");
	}

	switch(cmd) {

	case MSG_SEND :	//ioctl_send
		printk("0\n");
		if(find_ch_result ==1) {//do not have channel
			printk("not connected\n");			
			return -1;//not connected
		}
		printk("1\n");
		list_for_each_entry(tmp_ch, &ch_list.channel.list, list) {	
			printk("2\n");
			if(tmp_ch->ch_num == ch) {	//case: ch is exist
				printk("3\n");
				if(tmp_ch->u1->u_id == request_id) {	// case request == u1
					printk("case u1\n");
					if(tmp_ch->que_duplex->que1_size +1 > MAXMSG){printk("over maxmsg!\n");return 0;}
					if(tmp_ch->que_duplex->que1_vol+len > MAXVOL){printk("over maxvol!\n");return 0;}
					//exception processing
					tmp_msg = (MSG_QUE*)kmalloc(sizeof(MSG_QUE),GFP_KERNEL);
					tmp_msg->data = args->data;	
					tmp_msg->data_size = len*sizeof(*args->data);
					//creat msg
					mdelay(5);
					spin_lock(&my_lock);////critical session que
					list_add_tail(&tmp_msg, &tmp_ch->que_duplex->msg_que1.list);
					tmp_ch->que_duplex->que1_size+=1;
					tmp_ch->que_duplex->que1_vol +=tmp_msg->data_size;
					spin_unlock(&my_lock);////critical session que
					printk("success send(u1)");
					return len;
				}
				if(tmp_ch->u2->u_id == request_id){	//case request ==u2
					if(tmp_ch->que_duplex->que2_size +1 > MAXMSG){printk("over maxmsg!\n");return 0;}
					if(tmp_ch->que_duplex->que2_vol+len > MAXVOL){printk("over maxmsg!\n");return 0;}
					//exception processing
					tmp_msg = (MSG_QUE*)kmalloc(sizeof(MSG_QUE),GFP_KERNEL);
					tmp_msg->data = args->data;	
					tmp_msg->data_size = len*sizeof(*args->data);
					//creat msg
					mdelay(5);
					spin_lock(&my_lock);////critical session que
					list_add_tail(&args->data, &tmp_ch->que_duplex->msg_que2.list);
					tmp_ch->que_duplex->que2_size +=1;
					tmp_ch->que_duplex->que2_vol +=tmp_msg->data_size;
					spin_unlock(&my_lock);////critical session que
					printk("success send(u2)");
					return len;
				}
				else return -1;//not matching channel number
			}
		}
		break;

	case MSG_RECEIVE : //ioctl_receive	

		if(find_ch_result ==1) {//do not have channel
			return -1;//not connected
		}

		list_for_each_entry(tmp_ch, &ch_list.channel.list, list) {
			if(tmp_ch->ch_num == ch) {	//case: ch is exist
				if(tmp_ch->u1->u_id == request_id) {	// case request == u1
					if(list_empty(&(tmp_ch->que_duplex->msg_que2.list)))return 0;
					else {
						tmp_msg = tmp_ch->que_duplex->msg_que2.list.next;//first node for receive 
						if(tmp_msg->data_size <= len*sizeof(*args->data)) { //request bigger than node's data
							args->data =tmp_msg->data; //copy data
							mdelay(5);
							spin_lock(&my_lock);////critical session que
							list_del_init(tmp_msg);	//delete node from que
							tmp_ch->que_duplex->que2_size-1;
							tmp_ch->que_duplex->que2_vol -=len*sizeof(*args->data);
							spin_unlock(&my_lock);////critical session que
							kfree(tmp_msg);
							return len;
						}
						else {			//reqeust buf smaller than node's data
							//arg->data =tmp_msg->data; //copy data
							//arg->data_size = tmp_msg->data_size;
							//list_del(tmp_msg);
							//tmp_ch->que_duplex->que2_size-1;
							//tmp_ch->que_duplex->que2_vol - arg->data_size;
							//kfree(tmp_msg);
							//return len;
						}
					}
					return len;
				}

				if(tmp_ch->u2->u_id == request_id) {// case request == u2
					if(list_empty(&tmp_ch->que_duplex->msg_que1.list)) {return 0;}
					else {
						tmp_msg = tmp_ch->que_duplex->msg_que1.list.next;//first node for receive 
						if(tmp_msg->data_size <= len*sizeof(*args->data)) { //request bigger than node's data
							args->data =tmp_msg->data; //copy data
							mdelay(5);
							spin_lock(&my_lock);////critical session que
							list_del_init(tmp_msg);	//delete node from que
							tmp_ch->que_duplex->que1_size-1;
							tmp_ch->que_duplex->que1_vol - len*sizeof(*args->data);
							spin_unlock(&my_lock);////critical session que
							kfree(tmp_msg);
							return len;
						}
						else {			//reqeust buf smaller than node's data
							//arg->data =tmp_msg->data; //copy data
							//arg->data_size = tmp_msg->data_size;
							//list_del(tmp_msg);
							//tmp_ch->que_duplex->que2_size-1;
							//tmp_ch->que_duplex->que2_vol - arg->data_size;
							//kfree(tmp_msg);
							//return len;
						}
					}
				}
			}
		}	
		break;

	case CH_OPEN :	//ioctl_open
		printk("open\n");
		if(find_ch_result == 1) {//creat new channel
			new_ch = (CHANNEL*)kmalloc(sizeof(CHANNEL), GFP_KERNEL);//channel create
			que_du = (QUE_DUPLEX*)kmalloc(sizeof(QUE_DUPLEX), GFP_KERNEL);//que_du create
			u_tmp = (USER*)kmalloc(sizeof(USER), GFP_KERNEL);//user create
			new_ch->que_duplex= que_du;
			new_ch->ch_num = ch;
			new_ch->_valid = 0;	//channel init
			u_tmp->u_ch = ch;
			u_tmp->u_id = current->pid;	//user init
			//printk("--pid%d(curr):%d(u_id)\n",current->pid, u_tmp->u_id);
			new_ch->u1 = u_tmp;	//add user in channel
			new_ch->user_count = 1;
			list_add_tail(&(new_ch->list), &(ch_list.channel.list));//new_ch is added in ch_list
			ch_list.ch_count +=1;//total channel sum ++
			printk("create channel\n");//////////
			return ch;
		}		
		else if(find_ch_result ==2) { //connect to existing channel
			u_tmp = (USER*)kmalloc(sizeof(USER), GFP_KERNEL);
			u_tmp->u_ch = ch;
			u_tmp->u_id = current->pid;	//user create, init

			list_for_each_entry(tmp_ch, &ch_list.channel.list, list) {	
				if(tmp_ch->ch_num == ch) {
					tmp_ch->_valid = 1;//for detect to addtional connect_request
					tmp_ch->u2 = u_tmp;//add user in channel
					tmp_ch->user_count +=1;
					printk("connect~~~\n");///////		
					return ch;
				}
			}
		}
		else if(find_ch_result ==3) { //not valid ....
			printk("not valid");
			return 0;
		}
		else if(find_ch_result ==4) { //max channel...
			printk("max channel");
			return 0;
		}
		else printk("nonono!!!\n");//////////
		return -1;

	case CH_CLOSE :	//ioctl_close
		if(find_ch_result ==1) {//do not have channel
			printk("have not such channel num\n");
			return 0;//not connected
		}
		list_for_each_entry(tmp_ch, &ch_list.channel.list, list) {
			if(tmp_ch->ch_num == ch) {	//case: ch is exist
				if(tmp_ch->u1->u_id == request_id) {	// case request == u1
					printk("case u1\n");
					if(tmp_ch->user_count==1) {	//don't have partner
						list_del_init(&tmp_ch->list);//delete channel
						kfree(tmp_ch->u1);
						kfree(tmp_ch->que_duplex);
						kfree(tmp_ch);	//memory (user, channel, que)
						ch_list.ch_count -=1;//channel_count -1
						return 1;//close success
					} 
					else {//user_count==2
						kfree(tmp_ch->u1);//memory(user)
						tmp_ch->user_count -=1;//user_count -1
						return 1;//close success
					}
				}
				else if(tmp_ch->u2->u_id == request_id) {	// case request == u2
					printk("case u2\n");
					if(tmp_ch->user_count==1) {	//don't have partner
						list_del_init(&tmp_ch->list);//delete channel
						kfree(tmp_ch->que_duplex);
						kfree(tmp_ch->u2);
						kfree(tmp_ch);	//memory (user, channel, que)
						ch_list.ch_count -=1;//channel_count -1
						return 1;//close success
					} 
					else {//user_count==2
						kfree(tmp_ch->u2);//memory(user)
						tmp_ch->user_count -=1;//user_count -1
						return 1;//close success
					}
				}
				else {return 0;}//channel number is not valid
			}
		}
		break;

	}//switch
}//ioctl

struct file_operations hong_ioctl_fops = {
	.unlocked_ioctl = hong_ioctl,
};


static dev_t dev_num;
static struct cdev *cd_cdev;


static int __init ipc_init(void) {
	alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
	cd_cdev = cdev_alloc();
	cdev_init(cd_cdev, &hong_ioctl_fops);
	cdev_add(cd_cdev, dev_num, 1);
	
	INIT_LIST_HEAD(&ch_list.channel.list); //init channel_list'head
	ch_list.ch_count = 0;	//ch count init

	printk("ipc init");
	return 0;
}

static void __exit ipc_exit(void) {
	cdev_del(cd_cdev);
	unregister_chrdev_region(dev_num, 1);
	
	printk("ipc exit\n");
}

module_init(ipc_init);
module_exit(ipc_exit);
