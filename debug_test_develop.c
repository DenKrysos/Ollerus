/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */

#define NO_ABSINT_CTRL_C_FUNCTIONS

#include "ollerus_globalsettings.h"
#include "ollerus.h"
#include "ollerus_base.h"
#include "absint.h"
#include "absint_ctrl.h"
#include "absint_ctrl_sdn_ctrl_com.h"

#include "head/ollerus_extern_functions.h"





static sem_t listen_wait;





static void *absint_ctrl_channel_switch_fsm_testing_socket (void* arg){
	int ls=((struct AICtrlFSMThreadArgPassing *)arg)->socket_fsmside;
	struct AI_ChanSwitch_Couple *AICouple=((struct AICtrlFSMThreadArgPassing *)arg)->AICouple;
	sem_t *sem_all=((struct AICtrlFSMThreadArgPassing *)arg)->sem_all;
	sem_t *sem_sendwait=((struct AICtrlFSMThreadArgPassing *)arg)->sem_sendwait;
	struct AICtrlInterThreadSendMsgHeader ***sendmsg_all=((struct AICtrlFSMThreadArgPassing *)arg)->sendmsg_all;
	free(arg);
	int s;
	int bytes_recvd;
	struct AICtrlFSMSocketMsgHeader recvh;
	char *msg;


	//TODO: This whole semaphore-destroy and _wait inside the listener threads; also at the manager->_AI_DISCONNECTED
		//To make it really sophisticated.
	//Last operations of the Thread (in order)
//	AICouple->FSM=0; //Every other Thread first checks for pthread_t!=0
	//				//After every sem_wait on fsm semaphores other Threads check for return val and errno
//	sem_destroy(&(AICouple->fsm_wakeup));
//	sem_destroy(&(AICouple->fsm_ranthrough));
	/* Ähm, ok slightly big change. Handle it completely other
	 * This Semaphore Gedöns would be to Heavyweight
	 */

	s = accept(ls, (struct sockaddr *)NULL, (socklen_t*)NULL);
	if (s<0) {
		perror("accept failed");
		return 1;
	}
	close(ls);

	sem_post(&listen_wait);

	bytes_recvd=recv(s,&recvh,sizeof(recvh),0);
		msg=malloc(recvh.msgsize);
		if(recvh.msgsize>0){
			bytes_recvd=recv(s,msg,recvh.msgsize,0);
			while(bytes_recvd<recvh.msgsize){
				bytes_recvd+=recv(s,msg+bytes_recvd,(recvh.msgsize)-bytes_recvd,0);
			}
		}
		printfc(green,"FSM-Thread");printf(": Received %d Bytes from Thread-Index %d\n",recvh.msgsize,recvh.thread_index);
		printf("Msg: ");printf("%s\n",msg);
		switch(recvh.type){
		case AIC_WLAN_SWITCH_FSM_AITC_READY_TO_SWITCH:
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_SWITCH_DONE_ACK:
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_READY_TIMEOUT:
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_DONE_TIMEOUT:
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_STAY_READY_ACK:
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_STAY_DONE_ACK:
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_R:
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_D:
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_FINISH_ACK:
			break;
		default:
			break;
		}

		recvh.thread_index;



	//Last operations of the Thread (in order)
	//Cut the Connection between the Inter-Thread Sockets

	//Close the two sockets.
		//Note: The Socket-Descriptor (also the integer, that holds it) for the FSM-Thread sided socket
		//was temporary. There everything gets lost, inklusive the variable
		//The holding Variable for the socket, that is used from the AI-Receive-Threads to pass
		//something to the FSM-Thread, stays allocated (inside the ChanSwitch_Couple struct)
		//But this 'int' becomes cleaned: It doesn't contain a valid socket-descriptor any longer.
	close(AICouple->fsmsock);
	AICouple->fsmsock=0;
	close(s);

	//Set the pthread-ID to '0', to tell that no Thread is running, that the Thread-Place is free
	AICouple->FSM=0;
	depr(endfsm)
	return NULL;
}






static void *absint_ctrl_each_connected_thread_listen_testing_socket (void* arg) {
	const int thread_index = (((struct AICtrlThreadListenArgPassing *)arg)->thread_index);
	int s = ((struct AICtrlThreadListenArgPassing *)arg)->solid_s;
	int *bytes_recvd = ((struct AICtrlThreadListenArgPassing *)arg)->bytes_recvd;
	sem_t *sem_all = ((struct AICtrlThreadListenArgPassing *)arg)->sem_all;
	sem_t *sem=&sem_all[thread_index];
	struct AICtrlInterThreadManageMsgHeader *manmsg=((struct AICtrlThreadListenArgPassing *)arg)->manmsg;
	struct AI_ChanSwitch_Couple *AICouple=((struct AICtrlThreadListenArgPassing *)arg)->AICouple;
	free(arg);
	int err;
	int pthreadoldcancelstate;
	int bytes_recvd_local;
	struct AICtrlMsgHeader recvh;
	char *fsmmsg;

	recvh.type=AIC_WLAN_SWITCH_FSM_AITC_READY_TO_SWITCH;
	char testmsg[]="test";
	recvh.msgsize=sizeof(testmsg);

			fsmmsg=malloc(sizeof(struct AICtrlFSMSocketMsgHeader)+recvh.msgsize);
			((struct AICtrlFSMSocketMsgHeader *)fsmmsg)->msgsize=recvh.msgsize;
			((struct AICtrlFSMSocketMsgHeader *)fsmmsg)->thread_index=thread_index;
			((struct AICtrlFSMSocketMsgHeader *)fsmmsg)->type=recvh.type;
			if(recvh.msgsize>0){
				memcpy(fsmmsg+sizeof(struct AICtrlFSMSocketMsgHeader),testmsg,sizeof(testmsg));
			}
			sem_wait(&(AICouple->sem_sock));
			int bytes_sent,fsmmsglen;
			fsmmsglen = sizeof(struct AICtrlFSMSocketMsgHeader)+(((struct AICtrlFSMSocketMsgHeader *)fsmmsg)->msgsize);

			sem_wait(&listen_wait);

			SendFSMAgain:
			bytes_sent = send(AICouple->fsmsock, fsmmsg, fsmmsglen, 0);
			switch(bytes_sent) {
			case -1:
				switch(errno){
				case EINVAL:
				case ENOTSOCK:
					printfc(red,"Error");printf(": Socket not existent.\n");
					goto SkipFSMMsg;
					break;
				default:
					break;
				}
				goto SkipFSMMsg;
				break;
			default:
				if(bytes_sent==fsmmsglen) {
					//Everything sent, everything done. Just fine and we are finished.
				} else if(bytes_sent<fsmmsglen) {
					//Not everything sent, but the beginning. Go on and send the rest.
					fsmmsg=fsmmsg+bytes_sent;
					fsmmsglen=fsmmsglen-bytes_sent;
					goto SendFSMAgain;
				}
				break;
			}
//			err = senddetermined(AICouple->fsmsock,fsmmsg,sizeof(struct AICtrlFSMSocketMsgHeader)+(((struct AICtrlFSMSocketMsgHeader *)fsmmsg)->msgsize));
			SkipFSMMsg:
			sem_post(&(AICouple->sem_sock));
			free(fsmmsg);

	err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
//	sem_wait(&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE]);
//	manmsg->msgsize=0;
//	manmsg->idx=thread_index;
//	manmsg->type=AIC_INTERN_AI_DISCONNECT;
//	manmsg->msg=NULL;

//	*bytes_recvd = bytes_recvd_local;
//	sem_post(sem);
	err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);

	depr(endlisten)
	return err;
}




int test_socket_comm_to_chan_switch_fsm(){
	int err;
	pthread_t thread1, thread2;
	thread1=0;


	struct AI_ChanSwitch_Couple **AICouple;
	AICouple=malloc(sizeof(struct AI_ChanSwitch_Couple *));
	*AICouple=malloc(sizeof(struct AI_ChanSwitch_Couple));
	memset(*AICouple,0,sizeof(struct AI_ChanSwitch_Couple));


	(*AICouple)->AI1_thread_index=0;
	(*AICouple)->AI2_thread_index=1;
	sem_init(&((*AICouple)->sem_sock),0,1);

	sem_init(&listen_wait,0,-1);


	struct AICtrlThreadListenArgPassing *pthreadArgPass;

	pthreadArgPass = malloc(sizeof(struct AICtrlThreadListenArgPassing));
//	pthreadArgPass->solid_s = s;
//	pthreadArgPass->bytes_recvd = &bytes_recvd;
//	pthreadArgPass->sem_all = sem_all;
	pthreadArgPass->thread_index=42;
//	pthreadArgPass->manmsg=((struct AICtrlThreadArgPassing *)arg)->manmsg;
	pthreadArgPass->AICouple=*AICouple;


	{
	pthread_attr_t tattr;
	/*initialized with default attributes*/
	if((err=pthread_attr_init(&tattr)) < 0) {
		perror("could not create thread-attribute");
		return MAIN_ERR_STD;
	}
	if((err=pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)) < 0){
		perror("could not modify thread-attribute");
		if( (err=pthread_attr_destroy(&tattr)) != 0 ) {
			perror("could not destroy thread-attribute");
			return MAIN_ERR_STD;
		}
		return MAIN_ERR_STD;
	}
//	depr(thread1,"%d",thread1)
	/*secure detached-state for the threads over attributed creation*/
	if( (err=pthread_create(&thread1, &tattr, absint_ctrl_each_connected_thread_listen_testing_socket, (void*)pthreadArgPass)) < 0) {
		perror("could not create thread");
		if( (err=pthread_attr_destroy(&tattr)) != 0 ) {
			perror("could not destroy thread-attribute");
			return MAIN_ERR_STD;
		}
		return MAIN_ERR_STD;
	}
	if( (err=pthread_attr_destroy(&tattr)) != 0 ) {
		perror("could not destroy thread-attribute");
		return MAIN_ERR_STD;
	}
	}
//	depr(thread1,"%d",thread1)



	if(!((*AICouple)->FSM)){
		//Start Thread
		//First create "Reception Socket" for the FSM-Thread: The Socket-Descriptor that the Thread gets
		//(Something like the "right-side" of the connection between two sockets. The "left-side" is the one used by the
		//Receive-Threads for the AI-Connections)
		//Then create the Thread (pass the "temporary" sock-descriptor to the Thread)
		//Finally connect the two sockets
		int sock_fsmside;
		struct sockaddr_in myaddr_in;
		struct sockaddr_in myaddr_in2;
		struct sockaddr_in peeraddr_in;
		memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset ((char *)&myaddr_in2, 0, sizeof(struct sockaddr_in));
		memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in));
		myaddr_in.sin_family = AF_INET;
		myaddr_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		myaddr_in.sin_port = htons(0);
		sock_fsmside=socket(AF_INET,SOCK_STREAM,0);
		if (sock_fsmside == -1) {
			ANSICOLORSET(ANSI_COLOR_GREEN);
			fprintf(stderr,"-->AI-Ctrl-Manager:");
			ANSICOLORRESET;
			fprintf(stderr, " unable to create left-side socket for the FSM-Thread. errno: %d\n",errno);
			exit(1);
		}
		(*AICouple)->fsmsock=socket(AF_INET,SOCK_STREAM,0);
		if ((*AICouple)->fsmsock == -1) {
			ANSICOLORSET(ANSI_COLOR_GREEN);
			fprintf(stderr,"-->AI-Ctrl-Manager:");
			ANSICOLORRESET;
			fprintf(stderr, " unable to create FSM-Thread-sided socket\n");
			exit(1);
		}
		if (bind(sock_fsmside, &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
			ANSICOLORSET(ANSI_COLOR_GREEN);
			fprintf(stderr,"-->AI-Ctrl-Manager:");
			ANSICOLORRESET;
			fprintf(stderr, " unable to bind FSM-Thread-sided address. errno: %d\n",errno);
			exit(1);
		}
		if (listen(sock_fsmside, 5) == -1) {
			ANSICOLORSET(ANSI_COLOR_GREEN);
			fprintf(stderr,"-->AI-Ctrl-Manager:");
			ANSICOLORRESET;
			fprintf(stderr, " unable to listen on FSM-Thread-sided socket\n");
			exit(1);
		}
		err=sizeof(struct sockaddr_in);
		getsockname(sock_fsmside,&myaddr_in2,(socklen_t *)(&err));
		peeraddr_in.sin_family = AF_INET;
		peeraddr_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		peeraddr_in.sin_port = myaddr_in2.sin_port;
		{/*Scope*/
			struct AICtrlFSMThreadArgPassing *pthreadArgPass;

			pthreadArgPass = malloc(sizeof(struct AICtrlFSMThreadArgPassing));
			pthreadArgPass->socket_fsmside=sock_fsmside;
//			pthreadArgPass->sem_all=sem_all;
//			pthreadArgPass->sem_sendwait=sem_sendwait;
//			pthreadArgPass->sendmsg_all=sendmsg_all;
			pthreadArgPass->AICouple=*AICouple;

			pthread_attr_t tattr;
			/*initialized with default attributes*/
			if((err=pthread_attr_init(&tattr)) < 0) {
				perror("could not create thread-attribute");
				exit(MAIN_ERR_STD);
			}
			if((err=pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)) < 0){
				perror("could not modify thread-attribute");
				if( (err=pthread_attr_destroy(&tattr)) != 0 ) {
					perror("could not destroy thread-attribute");
//						exit(MAIN_ERR_STD);
				}
//					exit(MAIN_ERR_STD);
			}
			/*secure detached-state for the threads over attributed creation*/
			if( (err=pthread_create(&((*AICouple)->FSM), &tattr, absint_ctrl_channel_switch_fsm_testing_socket, (void*)pthreadArgPass)) < 0) {
				perror("could not create thread");
				if( (err=pthread_attr_destroy(&tattr)) != 0 ) {
					perror("could not destroy thread-attribute");
					exit(MAIN_ERR_STD);
				}
				exit(MAIN_ERR_STD);
			}
			if( (err=pthread_attr_destroy(&tattr)) != 0 ) {
				perror("could not destroy thread-attribute");
//					exit(MAIN_ERR_STD);
			}
			/*Thread created*/
		}
		ThreadNotReadyAlready:
		if (connect((*AICouple)->fsmsock, &peeraddr_in, sizeof(struct sockaddr_in)) ==-1) {
				#ifdef DEBUG
			printfc(green,"FSM-Thread-Start");printf(": Unsuccessful Inter-Thread Socket connect.\n\t\tTrying again\n");
				#endif
			goto ThreadNotReadyAlready;
		}
		sem_post(&listen_wait);
	}

	struct timespec waitfor;
	waitfor.tv_sec=1;
	waitfor.tv_nsec=0;
	do {
		err = nanosleep(&waitfor, &waitfor);
	} while (err<0);

	pthread_join(thread1,NULL);
	pthread_join(((*AICouple)->FSM),NULL);

	return err;
}




#undef NO_ABSINT_CTRL_C_FUNCTIONS
