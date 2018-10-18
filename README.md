# ioctl_ipc


-구조체 구조

데이터타입(실제로 응용과 커널에서 매개변수로 주고받는 data단위)
	typedef struct _data {
		void *data;
		int ch;
	}DATA;
  
메시지큐(리스트로연결되어있으며, 각각 사이즈와 해당내용(data)를 갖는다.)
	typedef struct msg_que {
		struct list_head list;
		void *data;
		int data_size;
	}MSG_QUE;
  
큐_duplex(duplex조건에 맞게 송신큐와 수신큐가있으며 큐마다의 사이즈와볼륨을갖고 있다.)
	typedef struct que_duplex {
		int que1_size;
		int que1_vol;
		int que2_size;
		int que2_vol;
		MSG_QUE msg_que1;
		MSG_QUE msg_que2;
	}QUE_DUPLEX;
  
User(커널에서 관리하기위한 user의 pid와 ch정보를 갖는다.)
	typedef struct my_user {
		int u_id;
		int u_ch;
	}USER;
  
Channel(리스트로연결되어있으며, 유저2명, 큐_duplex, 채널번호, 연결된 유저개수, valid값)
	typedef struct channel {
		struct list_head list;
		QUE_DUPLEX *que_duplex;	
		USER *u1, *u2;
		int ch_num;	//ch id
		int user_count;	//process count
		int _valid	//valid or not for connect(open)
	}CHANNEL;

채널리스트(채널리스트와 생성된 채널개수를관리한다.)
	typedef	struct channel_list {
		CHANNEL channel;
		int ch_count;
	}CHANNEL_LIST;
