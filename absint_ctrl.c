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






static char check_threads_index_presence(pthread_t *threads, int idx){
	if(threads[idx]==0){
		return 0;
	}else{
		return 1;
	}
}
static int get_first_free_threads_index(pthread_t *threads){
	int i;
	for(i=0;i<sizeof(threads);i++){
		if(threads[i]==0) {
			return i;
		}
	}
	return -1;
}
static int get_last_used_threads_index(pthread_t *threads){
	int i;
	for(i=sizeof(threads)-1;i>=0;i--){
		if(threads[i]!=0) {
			return i;
		}
	}
	return -1;
}





void *absint_ctrl_channel_switch_fsm (void* arg);



//TODO: When we send a msg to a AI, which contains a device: Check in our present Interfaces of this AI, if this dev even is present.
void *absint_ctrl_manager_thread (void* arg){
//Cancelstate of this Thread is obsolete anyway, because when this thread here gets canceled
//we are screwed in any case.
	printfc(green,"Manager-Thread: ");printf("Starting up.\n");
	pthread_t *all_threads = (((struct AICtrlManageThreadArgPassing *)arg)->all_threads_array);
	sem_t *sem_all = (((struct AICtrlManageThreadArgPassing *)arg)->sem_all);
	sem_t *sem_sendwait = (((struct AICtrlManageThreadArgPassing *)arg)->sem_sendwait);
	sem_t *semrd=&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_READ];
	sem_t *semwr=&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE];
	sem_t *sem_sdnsend_rd=&sem_all[ABSINT_CTRL_SEM_SDNSEND_MSG_READ];
	sem_t *sem_sdnsend_wr=&sem_all[ABSINT_CTRL_SEM_SDNSEND_MSG_WRITE];
	struct AICtrlInterThreadManageMsgHeader *manmsg=((struct AICtrlManageThreadArgPassing *)arg)->manmsg;
	struct AICtrlInterThreadSDNSendMsgHeader **sdnsend_msg=((struct AICtrlManageThreadArgPassing *)arg)->sdnsendmsg;
	struct AICtrlInterThreadSendMsgHeader ***sendmsg_all=((struct AICtrlManageThreadArgPassing *)arg)->sendmsg_all;
	struct AICtrlConnectedAI **AIConnections=((struct AICtrlManageThreadArgPassing *)arg)->AIConnections;
	//Use as Array. Entries with '0' do still not have an Inter-AI WLAN Connection assigned:
	//Array of Pointers: *UnsetablishedAIs[Number of Max Threads]
	struct AI_ChanSwitch_Couple **AICouples=((struct AICtrlManageThreadArgPassing *)arg)->AICouples;
	//The Pointer to my own sendmsg-header. This gets malloced
	#define sendmsghp ((*sendmsg_all)[manthreadindex])
	//The Pointer to the actual message
	#define sendmsgp ((char *)((*sendmsg_all)[manthreadindex]+sizeof(struct AICtrlInterThreadSendMsgHeader)))
	#define sdnmsghp (*sdnsend_msg)
	#define sdnmsgp ((char *)((*sdnsend_msg)+1))
	free(arg);
	int err,err2,i;
	#define manthreadindex (manmsg->idx)//The Index of the Thread(AI-Connection), which is handled at the current Loop-Run-Through
	#define manmsgsize (manmsg->msgsize)

//	int msgsize=0;
	struct DPID_Collector DPIDs;
	memset(&DPIDs,0,sizeof(DPIDs));
	DPID_REFRESH
		
	while(1){

	sem_wait(semrd);
	
	if(FLAG_CHECK(manmsg->flags,FLAG_ABSINT_CTRL_INTER_MSG_CLI)){
		if(!check_threads_index_presence(all_threads,manmsg->idx)){
			printfc(yellow,"WARNING ");printfc(cyan,"AI-Ctrl: CLI ");printf("delivered Thread-Index, which isn't connected!\n\t--> Ignoring Command.\n");
			goto FreeSharedMemory;
		}
	}

	switch(manmsg->type){
		enum nl80211_band band;
		int chan;
		char *temp;
		char *readpnt;
		char *endpoint;
	case AIC_INTERN_AI_DISCONNECT:
		//Clean Datastructures for the client, like struct AICtrlConnectedAI
//		(*AIConnections)[manthreadindex].ifc=0;
		if((*AIConnections)[manthreadindex].iface){
			free((*AIConnections)[manthreadindex].iface);
//			(*AIConnections)[manthreadindex].iface=NULL;
		}
		for(err=0;err<13;err++){
			if(((*AIConnections)[manthreadindex].wneigh24)[err].start){
				free(((*AIConnections)[manthreadindex].wneigh24)[err].start);
//				((*AIConnections)[manthreadindex].wneigh24)[err].start=NULL;
			}
		}
		memset(&(*AIConnections)[manthreadindex],0,sizeof(struct AICtrlConnectedAI));
		if(sendmsghp){
			free(sendmsghp);
			sendmsghp=NULL;
		}
		if(AICouples[manthreadindex]){
			int tempDeleteIndex1,tempDeleteIndex2;
			tempDeleteIndex1=AICouples[manthreadindex]->AI1_thread_index;
			tempDeleteIndex2=AICouples[manthreadindex]->AI2_thread_index;
			if(AICouples[manthreadindex]->FSM)
				pthread_cancel(AICouples[manthreadindex]->FSM);
			errno=0;
			err=sem_getvalue(&(AICouples[manthreadindex]->sem_sock),&err2);
			if(errno!=EINVAL){
				sem_destroy(&(AICouples[manthreadindex]->sem_sock));
			}
			free(AICouples[manthreadindex]);
			AICouples[tempDeleteIndex1]=NULL;
			AICouples[tempDeleteIndex2]=NULL;
		}
		sem_post(&sem_all[manthreadindex]);
		break;
	case AIC_INTERN_PRINT_CONNECTIONS:
		err=print_active_ai_connections(all_threads,AIConnections,AICouples);
		break;
	case AIC_INTERN_PRINT_DPIDS:
		err=print_dpids(&DPIDs);
		break;
	case AIC_INTERN_GET_DPIDS:
		//Request the DPIDs from the SDN-Ctrl
		if(FLAG_CHECK(manmsg->flags,FLAG_ABSINT_CTRL_INTER_MSG_SDNCTRL)){
			if(DPIDs.count){
				DPIDs.count=0;
				free(DPIDs.dpid);
				DPIDs.dpid=NULL;
			}
			uint64_t tempdpid;
			temp=((char *)(manmsg->msg))+1;
			tempdpid=strtoll(temp, &endpoint, 10);
			if(temp==endpoint){
				//Then there was no number after the opening '['
				break;
			}else{
				DPIDs.count=1;
			}
			for(i=1;i<manmsgsize;i++){//Start with i=1 because the first character (the one with index '0') is always a '['
				if((manmsg->msg)[i]==',')
					(DPIDs.count)++;
			}
			DPIDs.dpid=malloc((DPIDs.count)*sizeof(*(DPIDs.dpid)));
			memset(DPIDs.dpid,0,(DPIDs.count)*sizeof(*(DPIDs.dpid)));
			i=0;
//			for(readpnt=(manmsg->msg);readpnt<((manmsg->msg)+manmsgsize);readpnt++){
//				if(((*readpnt)=='[')||((*readpnt)==' ')){
//					readpnt++;
//					(DPIDs.dpid)[i]=strtoll(readpnt, &endpoint, 10);
//					i++;
//				}
//			}
			for(readpnt=(manmsg->msg)+1;readpnt<((manmsg->msg)+manmsgsize);readpnt++){
				(DPIDs.dpid)[i]=strtoll(readpnt, &endpoint, 10);
				if(readpnt!=endpoint){
					readpnt=endpoint;
					i++;
					if(i>=(DPIDs.count))
						break;
				}
			}
			#ifdef DEVELOPMENT_MODE
			err=print_dpids(&DPIDs);
			#endif
		}else{
			sem_wait(sem_sdnsend_wr);
			sdnmsghp=malloc(sizeof(struct AICtrlInterThreadSDNSendMsgHeader));
			sdnmsghp->type=AIC_INTERN_GET_DPIDS;
			sdnmsghp->msgsize;
			sem_post(sem_sdnsend_rd);
		}
		break;
	case AIC_INTERN_DO_WLAN_TOPOLOGY_CONNECTIONS:
		;
		int unconnectedindex1,unconnectedindex2;
		err=establish_wlan_topology_connections(AICouples,all_threads,&unconnectedindex1,&unconnectedindex2);
		//if error is delivered, we do not have enough AIs connected -> Skip
		if(err)
			break;
		//And eeeeevery time i misuse the err-variable as a "real" integer, if it isn't needed as "error-code"
		//I know, it isn't thaaat nice, but spares memory...
		err=sizeof("AbsintAdhoc")+getDigitCountofInt(unconnectedindex1)+getDigitCountofInt(unconnectedindex2)+1;
		//+1 for the delimiting character between the indices, \0 is already in
		temp=malloc(err);
		snprintf(temp,err,"AbsintAdhoc%dU%d",unconnectedindex1,unconnectedindex2);
		temp[err]='\0';
		//OK, for now just do it easy. Start on any channel. If this Isnogood, the AIs will do a Channel-Switch anyway ;o)
		int initialchan;
		initialchan=1;
		msgfreq initialfreq=(msgfreq)ieee80211_channel_to_frequency(initialchan, NL80211_BAND_2GHZ);// NL80211_BAND_5GHZ
		sem_wait(&sem_sendwait[unconnectedindex1]);
		((*sendmsg_all)[unconnectedindex1])=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgfreq));
		((*sendmsg_all)[unconnectedindex1])->msgsize=sizeof(msgfreq);
		((*sendmsg_all)[unconnectedindex1])->type=AIC_CTAI_SET_ADHOC_FREQ;
		*((msgfreq*)((*sendmsg_all)[unconnectedindex1]+sizeof(struct AICtrlInterThreadSendMsgHeader)))=initialfreq;
		sem_post(&sem_all[unconnectedindex1]);
		//
		sem_wait(&sem_sendwait[unconnectedindex2]);
		((*sendmsg_all)[unconnectedindex2])=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgfreq));
		((*sendmsg_all)[unconnectedindex2])->msgsize=sizeof(msgfreq);
		((*sendmsg_all)[unconnectedindex2])->type=AIC_CTAI_SET_ADHOC_FREQ;
		*((msgfreq*)((*sendmsg_all)[unconnectedindex2]+sizeof(struct AICtrlInterThreadSendMsgHeader)))=initialfreq;
		sem_post(&sem_all[unconnectedindex2]);
		//
		sem_wait(&sem_sendwait[unconnectedindex1]);
		((*sendmsg_all)[unconnectedindex1])=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+err);
		((*sendmsg_all)[unconnectedindex1])->msgsize=err;
		((*sendmsg_all)[unconnectedindex1])->type=AIC_CTAI_SET_ADHOC_ESSID;
		memcpy(((char *)((*sendmsg_all)[unconnectedindex1]+sizeof(struct AICtrlInterThreadSendMsgHeader))),
				temp,
				err);
		sem_post(&sem_all[unconnectedindex1]);
		//
		sem_wait(&sem_sendwait[unconnectedindex2]);
		((*sendmsg_all)[unconnectedindex2])=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+err);
		((*sendmsg_all)[unconnectedindex2])->msgsize=err;
		((*sendmsg_all)[unconnectedindex2])->type=AIC_CTAI_SET_ADHOC_ESSID;
		memcpy(((char *)((*sendmsg_all)[unconnectedindex2]+sizeof(struct AICtrlInterThreadSendMsgHeader))),
				temp,
				err);
		sem_post(&sem_all[unconnectedindex2]);
		//
		sem_wait(&sem_sendwait[unconnectedindex1]);
		((*sendmsg_all)[unconnectedindex1])=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader));
		((*sendmsg_all)[unconnectedindex1])->msgsize=0;
		((*sendmsg_all)[unconnectedindex1])->type=AIC_CTAI_START_ADHOC;
		sem_post(&sem_all[unconnectedindex1]);
		//
		sem_wait(&sem_sendwait[unconnectedindex2]);
		((*sendmsg_all)[unconnectedindex2])=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader));
		((*sendmsg_all)[unconnectedindex2])->msgsize=0;
		((*sendmsg_all)[unconnectedindex2])->type=AIC_CTAI_START_ADHOC;
		sem_post(&sem_all[unconnectedindex2]);
		break;
	case AIC_INTERN_WLAN_SWITCH_FSM_START_CTRL_THREAD:
		if(!(AICouples[manthreadindex]->FSM)){
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
				fprintf(stderr, " unable to create left-side socket for the FSM-Thread\n");
				exit(1);
			}
			AICouples[manthreadindex]->fsmsock=socket(AF_INET,SOCK_STREAM,0);
			if (AICouples[manthreadindex]->fsmsock == -1) {
				ANSICOLORSET(ANSI_COLOR_GREEN);
				fprintf(stderr,"-->AI-Ctrl-Manager:");
				ANSICOLORRESET;
				fprintf(stderr, " unable to create FSM-Thread-sided socket\n");
				exit(1);
			}
			if (bind(sock_fsmside,(struct sockaddr *)&myaddr_in,sizeof(struct sockaddr_in))==-1) {
				ANSICOLORSET(ANSI_COLOR_GREEN);
				fprintf(stderr,"-->AI-Ctrl-Manager:");
				ANSICOLORRESET;
				fprintf(stderr, " unable to bind FSM-Thread-sided address\n");
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
			getsockname(sock_fsmside,(struct sockaddr *)&myaddr_in2,(socklen_t *)(&err));
			peeraddr_in.sin_family = AF_INET;
			peeraddr_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			peeraddr_in.sin_port = myaddr_in2.sin_port;
			{/*Scope*/
				struct AICtrlFSMThreadArgPassing *pthreadArgPass;

				pthreadArgPass = malloc(sizeof(struct AICtrlFSMThreadArgPassing));
				pthreadArgPass->newfreq=*((msgfreq *)(manmsg->msg));
				pthreadArgPass->socket_fsmside=sock_fsmside;
				pthreadArgPass->sem_all=sem_all;
				pthreadArgPass->sem_sendwait=sem_sendwait;
				pthreadArgPass->sendmsg_all=sendmsg_all;
				pthreadArgPass->AICouple=(AICouples[manthreadindex]);

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
				if( (err=pthread_create(&(AICouples[manthreadindex]->FSM), &tattr, absint_ctrl_channel_switch_fsm, (void*)pthreadArgPass)) < 0) {
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
			if (connect(AICouples[manthreadindex]->fsmsock,(struct sockaddr *)&peeraddr_in,sizeof(struct sockaddr_in))==-1) {
					#ifdef DEBUG
				printfc(green,"FSM-Thread-Start");printf(": Unsuccessful Inter-Thread Socket connect.\n\t\tTrying again\n");
					#endif
				goto ThreadNotReadyAlready;
			}
		}
		break;
	case AIC_CTAI_START_ADHOC:
		sem_wait(&sem_sendwait[manthreadindex]);
		sendmsghp=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+manmsgsize);
		sendmsghp->msgsize=manmsgsize;
		sendmsghp->type=AIC_CTAI_START_ADHOC;
		memcpy(sendmsgp,manmsg->msg,manmsgsize);
		sem_post(&sem_all[manthreadindex]);
		break;
	case AIC_CTAI_STOP_ADHOC:
		sem_wait(&sem_sendwait[manthreadindex]);
		sendmsghp=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+manmsgsize);
		sendmsghp->msgsize=manmsgsize;
		sendmsghp->type=AIC_CTAI_STOP_ADHOC;
		memcpy(sendmsgp,manmsg->msg,manmsgsize);
		sem_post(&sem_all[manthreadindex]);
		break;
	case AIC_CTAI_SET_ADHOC_ESSID:
		sem_wait(&sem_sendwait[manthreadindex]);
		sendmsghp=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+manmsgsize);
		sendmsghp->msgsize=manmsgsize;
		sendmsghp->type=AIC_CTAI_SET_ADHOC_ESSID;
		memcpy(sendmsgp,manmsg->msg,manmsgsize);
		sem_post(&sem_all[manthreadindex]);
		break;
	case AIC_CTAI_SET_ADHOC_CHAN:
		sem_wait(&sem_sendwait[manthreadindex]);
						/*
						if((sendmsghp)&&(FLAG_CHECK(sendmsghp->flags,FLAG_ABSINT_CTRL_INTER_MSG_NEW))){
							//Then here is a still not outread send msg
							//Shouldn't be able to occur, through semaphore-synchronization
						}
						*/
		sendmsghp=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+manmsgsize-sizeof(msgchan)+sizeof(msgfreq));
		sendmsghp->msgsize=manmsgsize-sizeof(msgchan)+sizeof(msgfreq);
		//FLAG_SET(sendmsghp->flags,FLAG_ABSINT_CTRL_INTER_MSG_NEW);
		sendmsghp->type=AIC_CTAI_SET_ADHOC_FREQ;
		band = *(msgchan*)(manmsg->msg) <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
		//Fill in the message
		*(msgfreq*)sendmsgp= (msgfreq)ieee80211_channel_to_frequency(*(msgchan*)(manmsg->msg), band);
		memcpy((msgfreq*)sendmsgp+1,((msgfreq*)(manmsg->msg))+1,manmsgsize-sizeof(msgfreq));
		sem_post(&sem_all[manthreadindex]);
		break;
	case AIC_CTAI_SET_ADHOC_FREQ:
		sem_wait(&sem_sendwait[manthreadindex]);
		sendmsghp=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+manmsgsize);
		sendmsghp->msgsize=sizeof(msgfreq);
		sendmsghp->type=AIC_CTAI_SET_ADHOC_FREQ;
		*(msgfreq*)sendmsgp= *(msgfreq*)(manmsg->msg);
		sem_post(&sem_all[manthreadindex]);
		break;
	case AIC_CTAI_INQUIRE_IFACE:
		sem_wait(&sem_sendwait[manthreadindex]);
		sendmsghp=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+manmsgsize);
		sendmsghp->msgsize=manmsgsize;
		sendmsghp->type=AIC_CTAI_INQUIRE_IFACE;
		sem_post(&sem_all[manthreadindex]);
		break;
	case AIC_CTAI_INQUIRE_RSSI:
		sem_wait(&sem_sendwait[manthreadindex]);
		sendmsghp=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+manmsgsize-sizeof(msgchan)+sizeof(msgfreq));
		sendmsghp->msgsize=manmsgsize-sizeof(msgchan)+sizeof(msgfreq);
		sendmsghp->type=AIC_CTAI_INQUIRE_RSSI;
		band = *(msgchan*)(manmsg->msg) <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
		*(msgfreq*)sendmsgp= (msgfreq)ieee80211_channel_to_frequency(*(msgchan*)(manmsg->msg), band);
		memcpy(((msgfreq*)sendmsgp)+1,((msgchan*)(manmsg->msg))+1,manmsgsize-sizeof(msgchan));
		sem_post(&sem_all[manthreadindex]);
		break;
	case AIC_AITC_IFACE:
	#define ifmsgh
	#define ifmsgp (struct AICtrlConnectedAIIface *)(manmsg->msg)
	#define ifmsgcount (manmsgsize/sizeof(struct AICtrlConnectedAIIface))

		if((*AIConnections)[manthreadindex].iface){
			free((*AIConnections)[manthreadindex].iface);
			(*AIConnections)[manthreadindex].iface=NULL;
		}

		(*AIConnections)[manthreadindex].ifc=ifmsgcount;
		(*AIConnections)[manthreadindex].iface=malloc(manmsgsize);
//		for(err=0;err<(*AIConnections)[manthreadindex].ifc;err++){
//
//		}
		memcpy((*AIConnections)[manthreadindex].iface,manmsg->msg,manmsgsize);
//		depr(11);
//		char testdev[128];
//		inet_ntop(AF_INET,&(((*AIConnections)[manthreadindex].iface)[0].IP4),testdev,100);
//		depr(1,"count %d | iface %s | %s",((*AIConnections)[manthreadindex].ifc),(((*AIConnections)[manthreadindex].iface)[0].iface),testdev);

	#undef ifmsgh
	#undef ifmsgp
	#undef ifmsgcount
		#ifdef DEVELOPMENT_MODE
		print_active_ai_connections(all_threads,AIConnections,NULL);
		#endif
		break;
	case AIC_AITC_WLANSTAT_RSSI:
	#define strmsghp ((struct AICtrlMsgRSSIHeader *)(manmsg->msg))
	#define strmsgp ((struct WifiPackageParseData *)(strmsghp+1))
	#define strmsgcount (strmsghp->count)
		;
		chan=ieee80211_frequency_to_channel(strmsghp->freq)-1;
		if(((*AIConnections)[manthreadindex].wneigh24)[chan].start){
			free(((*AIConnections)[manthreadindex].wneigh24)[chan].start);
			((*AIConnections)[manthreadindex].wneigh24)[chan].start=NULL;
		}
		((*AIConnections)[manthreadindex].wneigh24)[chan].count=strmsgcount;
		((*AIConnections)[manthreadindex].wneigh24)[chan].start=malloc(manmsgsize-sizeof(struct AICtrlMsgRSSIHeader));
		memcpy(((*AIConnections)[manthreadindex].wneigh24)[chan].start,strmsgp,manmsgsize-sizeof(struct AICtrlMsgRSSIHeader));

	#undef strmsghp
	#undef strmsgp
	#undef strmsgcount
		#ifdef DEVELOPMENT_MODE
//		depr(1,"rssi %d",(((*AIConnections)[manthreadindex].wneigh24)[chan].start)[2].rssi);
		print_rssi_of_ai_connections(-1,all_threads,AIConnections);
		#endif
		break;
	case AIC_AITC_WLANSTAT_TRAFFICSTAT:
	#define strmsgp ((struct AICtrlMsgTrafficstat *)(manmsg->msg))
		;
		chan=ieee80211_frequency_to_channel(strmsgp->freq)-1;
		((*AIConnections)[manthreadindex].wneigh24)[chan].trafficstat=strmsgp->trafficstat;

	#undef strmsgp
		#ifdef DEVELOPMENT_MODE
//		depr(1,"rssi %d",(((*AIConnections)[manthreadindex].wneigh24)[chan].start)[2].rssi);
		print_rssi_of_ai_connections(-1,all_threads,AIConnections);
		#endif
		break;
	case AIC_AITC_MAC:
	case AIC_AITC_DPID:
	#define msgp ((uint64_t *)(manmsg->msg))
	#define msgcount (manmsgsize/sizeof(uint64_t))
		;
//		register uint64_t mac;
		uint64_t mac;
		for(i=0;i<msgcount;i++){
			mac=*(msgp+i);
			#ifdef DEVELOPMENT_MODE
			printMAC((unsigned char *)(&mac),6);puts("");
			printf("Dezimal: %llu\n",mac);
			#endif
			if(dpid_is_present(mac,&DPIDs)){
				(*AIConnections)[manthreadindex].dpid=mac;
				goto DPIDFound;
			}
		}
		DPID_REFRESH
		//If not present, just let the AI disconnect
		//The DPIDs are refreshed right now. If the AI is sure, that he has
		//an OVS running and this is properly working with DPID and all
		//than this maybe just wasn't still recognized by the AI-Ctrl and the
		//AI should try to reconnect.
		printfc(yellow,"\tINFO: I don't know the DPID for this AI, so i can't handel it anyway. Let it disconnect.\n");
		printfc(red,"IMPORTANT!: ");printf("Dirty testing. If you read this: Make the sending below this codeline working again!.\n");
		goto SkipDPIDDisconnect;
		sem_wait(&sem_sendwait[manthreadindex]);
		sendmsghp=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader));
		sendmsghp->msgsize=0;
		sendmsghp->type=AIC_INTERN_LET_AI_DISCONNECT;
		sem_post(&sem_all[manthreadindex]);
		SkipDPIDDisconnect:
		DPIDFound:
		;

				/*Complete different make over
					register uint64_t mac;
					err=0;
					AgainAfterRefreshedDPIDs:
					err2=(int)(DPIDs.count);
					if(err>2){
						//TODO: No DPID for this AI known. Do some Handling
						goto SkipRefresh;
					}
					for(i=0;i<(int)(*strmsgcount);i++){
						mac=*(((uint64_t *)(strmsgp))+err);
						if(dpid_is_present(mac,&DPIDs)){
							(*AIConnections)[manthreadindex].dpid=mac;
							goto SkipRefresh;
						}
					}
					DPID_REFRESH_AND_WAIT
					if(err2!=(DPIDs.count)){
						err++;
					}
					goto AgainAfterRefreshedDPIDs;
					SkipRefresh:
					;
				*/

	#undef msgcount
	#undef msgp
		break;
	default:
		break;
	}
	
	FreeSharedMemory:
	if(manmsg->msg){
		free(manmsg->msg);
		(manmsg->msg)=NULL;
	}
	memset(manmsg,0,sizeof(*manmsg));
	//FLAG_UNSET(manmsg->flags,FLAG_ABSINT_CTRL_INTER_MSG_NEW);
	sem_post(semwr);
	}
	
	#undef sendmsghp
	#undef sendmsgp
	#undef sdnmsghp
	#undef sdnmsgp
	#undef manmsgsize
	#undef threadindex_local
	#undef msgsize_local
}


void *absint_ctrl_channel_switch_fsm (void* arg){
		#ifdef DEVELOPMENT_MODE
			#define Print_Channel_Switch_State \
			printfc(blue,"\tState"); \
			switch(state){ \
			case CHANSWITCH_FSM_STATE_CTRL_START: \
				printf(": Start\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_AI1READY: \
				printf(": AI1-Ready\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_AI2READY: \
				printf(": AI2-Ready\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_ACCOMPLISHOVSSWITCH: \
				printf(": Accomplish-OVS-Switch\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_ACCOMPLISHOVSSWITCH_DONE1: \
				printf(": Accomplish-OVS-Switch-Done1\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_ACCOMPLISHOVSSWITCH_DONE2: \
				printf(": Accomplish-OVS-Switch-Done2\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_WAITFORDONEACK: \
				printf(": Wait-for-Done-ACK\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_AI1DONE: \
				printf(": AI1-Done\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_AI2DONE: \
				printf(": AI2-Done\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_WAITFORFINISH: \
				printf(": Wait-for-Finish\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_AI1FINISH: \
				printf(": AI1-Finish\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_AI2FINISH: \
				printf(": AI2-Finish\n"); \
				break; \
			case CHANSWITCH_FSM_STATE_CTRL_FINAL: \
				printf(": Final\n"); \
				break; \
			default: \
			break; \
			}
		#else
			#define Print_Channel_Switch_State
		#endif

	int err;
	int ls=((struct AICtrlFSMThreadArgPassing *)arg)->socket_fsmside;
	struct AI_ChanSwitch_Couple *AICouple=((struct AICtrlFSMThreadArgPassing *)arg)->AICouple;
	sem_t *sem_all=((struct AICtrlFSMThreadArgPassing *)arg)->sem_all;
	sem_t *sem_sendwait=((struct AICtrlFSMThreadArgPassing *)arg)->sem_sendwait;
	struct AICtrlInterThreadSendMsgHeader ***sendmsg_all=((struct AICtrlFSMThreadArgPassing *)arg)->sendmsg_all;
	msgfreq newfreq = ((struct AICtrlFSMThreadArgPassing *)arg)->newfreq;
	free(arg);
	int s;
	int bytes_recvd;
	struct AICtrlFSMSocketMsgHeader recvh;
	char *msg, *tmp;

	double time_begin,time_end,time_diff;

	char state=0;
	int AI1_Dev_Idx, AI2_Dev_Idx;

	//The Pointer to my own sendmsg-header. This gets malloced
	#define sendmsghp(IDX) ((*sendmsg_all)[IDX])
	//The Pointer to the actual message
	#define sendmsgp(IDX) ((char *)((*sendmsg_all)[IDX]+sizeof(struct AICtrlInterThreadSendMsgHeader)))
	#define FSMMSGID (*((msgswitchid *)(msg)))
	#define PAFTERID (msg+sizeof(msgswitchid))


	//NOTTODO: This whole semaphore-destroy and _wait inside the listener threads; also at the manager->_AI_DISCONNECTED
		//To make it really sophisticated.
	//Last operations of the Thread (in order)
//	AICouple->FSM=0; //Every other Thread first checks for pthread_t!=0
	//				//After every sem_wait on fsm semaphores other Threads check for return val and errno
//	sem_destroy(&(AICouple->fsm_wakeup));
//	sem_destroy(&(AICouple->fsm_ranthrough));
	/* Ähm, ok slightly big change. Handle it completely otherwise
	 * This Semaphore Gedöns would be to Heavyweight
	 */

	s = accept(ls, (struct sockaddr *)NULL, (socklen_t*)NULL);
	if (s<0) {
		perror("accept failed");
		return 1;
	}
	close(ls);

	(AICouple->instanceid)++;
	//First send the START Msg to the AIs
	err=sizeof("AbsintAdhocUF")+getDigitCountofInt(AICouple->AI1_thread_index)+getDigitCountofInt(AICouple->AI2_thread_index)+4;
	// +4 For the concatenated Frequency
	tmp=malloc(err);
	snprintf(tmp,err,"AbsintAdhoc%dU%dF%d",AICouple->AI1_thread_index,AICouple->AI2_thread_index,(int)newfreq);
	err--;//We don't send the \0 together, see below...
	tmp[err]='\0';
	sem_wait(&sem_sendwait[AICouple->AI1_thread_index]);
	sendmsghp(AICouple->AI1_thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid)+sizeof(msgfreq)+err);
	(sendmsghp(AICouple->AI1_thread_index))->msgsize=sizeof(msgswitchid)+sizeof(msgfreq)+err;
	(sendmsghp(AICouple->AI1_thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_START;
	*((msgfreq*)((msgswitchid *)(sendmsgp(AICouple->AI1_thread_index))+1))=newfreq;
	*((msgswitchid *)(sendmsgp(AICouple->AI1_thread_index)))=AICouple->instanceid;
	memcpy(sendmsgp(AICouple->AI1_thread_index)+sizeof(msgswitchid)+sizeof(msgfreq),tmp,err);
	sem_post(&sem_all[AICouple->AI1_thread_index]);

	sem_wait(&sem_sendwait[AICouple->AI2_thread_index]);
	sendmsghp(AICouple->AI2_thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid)+sizeof(msgfreq)+err);
	(sendmsghp(AICouple->AI2_thread_index))->msgsize=sizeof(msgswitchid)+sizeof(msgfreq)+err;
	(sendmsghp(AICouple->AI2_thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_START;
	*((msgfreq*)((msgswitchid *)(sendmsgp(AICouple->AI2_thread_index))+1))=newfreq;
	*((msgswitchid *)(sendmsgp(AICouple->AI2_thread_index)))=AICouple->instanceid;
	memcpy(sendmsgp(AICouple->AI2_thread_index)+sizeof(msgswitchid)+sizeof(msgfreq),tmp,err);
	sem_post(&sem_all[AICouple->AI2_thread_index]);
	free(tmp);

	state=CHANSWITCH_FSM_STATE_CTRL_START;

	Print_Channel_Switch_State;

	//Then go into this typical Input->Operation, State-Transition Loop-Mode

	msg=NULL;
	time_begin=getRealTime();
	while((bytes_recvd=recv(s,&recvh,sizeof(recvh),0))>0){
		if(recvh.msgsize>0){
			msg=malloc(recvh.msgsize);
			bytes_recvd=recv(s,msg,recvh.msgsize,0);
			while(bytes_recvd<recvh.msgsize){
				bytes_recvd+=recv(s,msg+bytes_recvd,(recvh.msgsize)-bytes_recvd,0);
			}
		}
		switch(recvh.type){
		case AIC_WLAN_SWITCH_FSM_AITC_READY_TO_SWITCH:
			if((AICouple->instanceid)!=FSMMSGID){
				break;
			}
			if((recvh.thread_index)==(AICouple->AI1_thread_index)){
				AI1_Dev_Idx=*((unsigned int *)(msg+sizeof(msgswitchid)));
				switch(state){
				case CHANSWITCH_FSM_STATE_CTRL_START:
					state=CHANSWITCH_FSM_STATE_CTRL_AI1READY;
//						#ifdef DEVELOPMENT_MODE
//						printfc(gray,"\tState");printf(": AI1-Ready\n");
//						#endif
					break;
				case CHANSWITCH_FSM_STATE_CTRL_AI2READY:
					//Start the Stuff with the SDN-Ctrl
					state=CHANSWITCH_FSM_STATE_CTRL_ACCOMPLISHOVSSWITCH;
						#ifdef DEVELOPMENT_MODE
						printfc(gray,"\tState");printf(": Accomplish-OVS-Switch\n");
						#endif
					//TODO: Temporary
						goto TemporaryStateTransition;
					break;
				default:
					goto LeaveMsgSwitch;
					break;
				}
			}else if((recvh.thread_index)==(AICouple->AI2_thread_index)){
				AI2_Dev_Idx=*((unsigned int *)(msg+sizeof(msgswitchid)));
				switch(state){
				case CHANSWITCH_FSM_STATE_CTRL_START:
					state=CHANSWITCH_FSM_STATE_CTRL_AI2READY;
//						#ifdef DEVELOPMENT_MODE
//						printfc(gray,"\tState");printf(": AI2-Ready\n");
//						#endif
					break;
				case CHANSWITCH_FSM_STATE_CTRL_AI1READY:
					//Start the Stuff with the SDN-Ctrl
					state=CHANSWITCH_FSM_STATE_CTRL_ACCOMPLISHOVSSWITCH;
						#ifdef DEVELOPMENT_MODE
						printfc(gray,"\tState");printf(": Accomplish-OVS-Switch\n");
						#endif
					//TODO: Temporary
						TemporaryStateTransition:
						state=CHANSWITCH_FSM_STATE_CTRL_WAITFORDONEACK;
//							#ifdef DEVELOPMENT_MODE
//							printfc(gray,"\tState");printf(": Wait-for-Done-ACK\n");
//							#endif
						sem_wait(&sem_sendwait[AICouple->AI1_thread_index]);
						sendmsghp(AICouple->AI1_thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid));
						(sendmsghp(AICouple->AI1_thread_index))->msgsize=sizeof(msgswitchid);
						(sendmsghp(AICouple->AI1_thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_SWITCH_DONE;
						*((msgswitchid *)(sendmsgp(AICouple->AI1_thread_index)))=AICouple->instanceid;
						sem_post(&sem_all[AICouple->AI1_thread_index]);
						//////////////////////////
						sem_wait(&sem_sendwait[AICouple->AI2_thread_index]);
						sendmsghp(AICouple->AI2_thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid));
						(sendmsghp(AICouple->AI2_thread_index))->msgsize=sizeof(msgswitchid);
						(sendmsghp(AICouple->AI2_thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_SWITCH_DONE;
						*((msgswitchid *)(sendmsgp(AICouple->AI2_thread_index)))=AICouple->instanceid;
						sem_post(&sem_all[AICouple->AI2_thread_index]);
					break;
				default:
					goto LeaveMsgSwitch;
					break;
				}
			}else{
				//Has to be an error. Hm, how to handle this in an elegant way?
			}
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_SWITCH_DONE_ACK:
			if((AICouple->instanceid)!=FSMMSGID){
				break;
			}
			if((recvh.thread_index)==(AICouple->AI1_thread_index)){
				switch(state){
				case CHANSWITCH_FSM_STATE_CTRL_WAITFORDONEACK:
					state=CHANSWITCH_FSM_STATE_CTRL_AI1DONE;
//						#ifdef DEVELOPMENT_MODE
//						printfc(gray,"\tState");printf(": AI1-Done\n");
//						#endif
					break;
				case CHANSWITCH_FSM_STATE_CTRL_AI2DONE:
					goto SendFinishMsg;
					break;
				default:
					goto LeaveMsgSwitch;
					break;
				}
			}else if((recvh.thread_index)==(AICouple->AI2_thread_index)){
				switch(state){
				case CHANSWITCH_FSM_STATE_CTRL_WAITFORDONEACK:
					state=CHANSWITCH_FSM_STATE_CTRL_AI2DONE;
//						#ifdef DEVELOPMENT_MODE
//						printfc(gray,"\tState");printf(": AI2-Done\n");
//						#endif
					break;
				case CHANSWITCH_FSM_STATE_CTRL_AI1DONE:
					SendFinishMsg:
					sem_wait(&sem_sendwait[AICouple->AI1_thread_index]);
					sendmsghp(AICouple->AI1_thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid));
					(sendmsghp(AICouple->AI1_thread_index))->msgsize=sizeof(msgswitchid);
					(sendmsghp(AICouple->AI1_thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_FINISH;
					*((msgswitchid *)(sendmsgp(AICouple->AI1_thread_index)))=AICouple->instanceid;
					sem_post(&sem_all[AICouple->AI1_thread_index]);
					/////////////////////////////////
					sem_wait(&sem_sendwait[AICouple->AI2_thread_index]);
					sendmsghp(AICouple->AI2_thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid));
					(sendmsghp(AICouple->AI2_thread_index))->msgsize=sizeof(msgswitchid);
					(sendmsghp(AICouple->AI2_thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_FINISH;
					*((msgswitchid *)(sendmsgp(AICouple->AI2_thread_index)))=AICouple->instanceid;
					sem_post(&sem_all[AICouple->AI2_thread_index]);
					state=CHANSWITCH_FSM_STATE_CTRL_WAITFORFINISH;
//						#ifdef DEVELOPMENT_MODE
//						printfc(gray,"\tState");printf(": Wait-for-Finish\n");
//						#endif
					break;
				default:
					goto LeaveMsgSwitch;
					break;
				}
			}
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_READY_TIMEOUT:
			#ifdef DEVELOPMENT_MODE
			printf("\tReady ");printfc(blue,"Timeout.\t");
			#endif
			if((AICouple->instanceid)!=FSMMSGID){
				break;
			}
			switch(state){
			case CHANSWITCH_FSM_STATE_CTRL_AI1READY:
				if((recvh.thread_index)!=(AICouple->AI1_thread_index))
					goto LeaveMsgSwitch;
				sem_wait(&sem_sendwait[AICouple->AI1_thread_index]);
				sendmsghp(AICouple->AI1_thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid));
				(sendmsghp(AICouple->AI1_thread_index))->msgsize=sizeof(msgswitchid);
				(sendmsghp(AICouple->AI1_thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_STAY_READY;
				*((msgswitchid *)(sendmsgp(AICouple->AI1_thread_index)))=AICouple->instanceid;
				sem_post(&sem_all[AICouple->AI1_thread_index]);
				break;
			case CHANSWITCH_FSM_STATE_CTRL_AI2READY:
				if((recvh.thread_index)!=(AICouple->AI2_thread_index))
					goto LeaveMsgSwitch;
				sem_wait(&sem_sendwait[AICouple->AI2_thread_index]);
				sendmsghp(AICouple->AI2_thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid));
				(sendmsghp(AICouple->AI2_thread_index))->msgsize=sizeof(msgswitchid);
				(sendmsghp(AICouple->AI2_thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_STAY_READY;
				*((msgswitchid *)(sendmsgp(AICouple->AI2_thread_index)))=AICouple->instanceid;
				sem_post(&sem_all[AICouple->AI2_thread_index]);
				break;
			case CHANSWITCH_FSM_STATE_CTRL_ACCOMPLISHOVSSWITCH:
			case CHANSWITCH_FSM_STATE_CTRL_ACCOMPLISHOVSSWITCH_DONE1:
			case CHANSWITCH_FSM_STATE_CTRL_ACCOMPLISHOVSSWITCH_DONE2:
				sem_wait(&sem_sendwait[recvh.thread_index]);
				sendmsghp(recvh.thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid));
				(sendmsghp(recvh.thread_index))->msgsize=sizeof(msgswitchid);
				(sendmsghp(recvh.thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_STAY_READY;
				*((msgswitchid *)(sendmsgp(recvh.thread_index)))=AICouple->instanceid;
				sem_post(&sem_all[recvh.thread_index]);
				break;
			}
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_DONE_TIMEOUT:
			#ifdef DEVELOPMENT_MODE
			printf("\tDone ");printfc(blue,"Timeout.\t");
			#endif
			if((AICouple->instanceid)!=FSMMSGID){
				break;
			}
			switch(state){
			case CHANSWITCH_FSM_STATE_CTRL_AI1DONE:
				if((recvh.thread_index)!=(AICouple->AI1_thread_index))
					goto LeaveMsgSwitch;
				sem_wait(&sem_sendwait[AICouple->AI1_thread_index]);
				sendmsghp(AICouple->AI1_thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid));
				(sendmsghp(AICouple->AI1_thread_index))->msgsize=sizeof(msgswitchid);
				(sendmsghp(AICouple->AI1_thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_STAY_DONE;
				*((msgswitchid *)(sendmsgp(AICouple->AI1_thread_index)))=AICouple->instanceid;
				sem_post(&sem_all[AICouple->AI1_thread_index]);
				break;
			case CHANSWITCH_FSM_STATE_CTRL_AI2DONE:
				if((recvh.thread_index)!=(AICouple->AI2_thread_index))
					goto LeaveMsgSwitch;
				sem_wait(&sem_sendwait[AICouple->AI2_thread_index]);
				sendmsghp(AICouple->AI2_thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid));
				(sendmsghp(AICouple->AI2_thread_index))->msgsize=sizeof(msgswitchid);
				(sendmsghp(AICouple->AI2_thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_STAY_DONE;
				*((msgswitchid *)(sendmsgp(AICouple->AI2_thread_index)))=AICouple->instanceid;
				sem_post(&sem_all[AICouple->AI2_thread_index]);
				break;
			case CHANSWITCH_FSM_STATE_CTRL_WAITFORFINISH:
				sem_wait(&sem_sendwait[recvh.thread_index]);
				sendmsghp(recvh.thread_index)=malloc(sizeof(struct AICtrlInterThreadSendMsgHeader)+sizeof(msgswitchid));
				(sendmsghp(recvh.thread_index))->msgsize=sizeof(msgswitchid);
				(sendmsghp(recvh.thread_index))->type=AIC_WLAN_SWITCH_FSM_CTAI_STAY_DONE;
				*((msgswitchid *)(sendmsgp(recvh.thread_index)))=AICouple->instanceid;
				sem_post(&sem_all[recvh.thread_index]);
				break;
			}
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_STAY_READY_ACK:
			if((AICouple->instanceid)!=FSMMSGID){
				break;
			}
			//Ähm... Ehhh... wayne... just ignore such msgs -.- The loose of such msgs, combinated with a timeout, would be worth to think about it.
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_STAY_DONE_ACK:
			if((AICouple->instanceid)!=FSMMSGID){
				break;
			}
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_R:
			if((AICouple->instanceid)!=FSMMSGID){
				break;
			}
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_D:
			if((AICouple->instanceid)!=FSMMSGID){
				break;
			}
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_FINISH_ACK:
			if((AICouple->instanceid)!=FSMMSGID){
				break;
			}
			if((recvh.thread_index)==(AICouple->AI1_thread_index)){
				switch(state){
				case CHANSWITCH_FSM_STATE_CTRL_WAITFORFINISH:
					state=CHANSWITCH_FSM_STATE_CTRL_AI1FINISH;
//						#ifdef DEVELOPMENT_MODE
//						printfc(gray,"\tState");printf(": AI1-Finish\n");
//						#endif
					break;
				case CHANSWITCH_FSM_STATE_CTRL_AI2FINISH:
					goto FinishFSM;
					break;
				default:
					goto LeaveMsgSwitch;
					break;
				}
			}else if((recvh.thread_index)==(AICouple->AI2_thread_index)){
				switch(state){
				case CHANSWITCH_FSM_STATE_CTRL_WAITFORFINISH:
					state=CHANSWITCH_FSM_STATE_CTRL_AI2FINISH;
//						#ifdef DEVELOPMENT_MODE
//						printfc(gray,"\tState");printf(": AI2-Finish\n");
//						#endif
					break;
				case CHANSWITCH_FSM_STATE_CTRL_AI1FINISH:
					goto FinishFSM;
					break;
				default:
					goto LeaveMsgSwitch;
					break;
				}
			}
			break;
		case AIC_WLAN_SWITCH_FSM_CTSDN_:
			break;
		case AIC_WLAN_SWITCH_FSM_SDNTC_:
			break;
		default:
			printfc(cyan,"Chan-Switch-FSM");printfc(red,"ERROR");printf(": Gone into default-case. Shouldn't occur...\n");
			break;
		}
		LeaveMsgSwitch:

		if(msg){
			free(msg);
			msg=NULL;
		}
		Print_Channel_Switch_State;
	}


	FinishFSM:
	time_end=getRealTime();
		#ifdef DEVELOPMENT_MODE
		printfc(gray,"\tChannel Switch");printf(" successful!\n");
		#endif
	time_diff=time_end-time_begin;
	printfc(gray,"\tElapsed Time: %f (Start-Time: %f | End-Time: %f\n",time_diff,time_begin,time_end);

	if(msg){
		free(msg);
		msg=NULL;
	}


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

	//The Pointer to my own sendmsg-header. This gets malloced
	#undef sendmsghp
	//The Pointer to the actual message
	#undef sendmsgp
	#undef FSMMSGID
	#undef PAFTERID

	//Set the pthread-ID to '0', to tell that no Thread is running, that the Thread-Place is free
	AICouple->FSM=0;
	return NULL;
}


void *absint_ctrl_sdn_ctrl_com_thread_listen(void* arg){
	int s = (((struct AICtrlSDNCtrlComThreadListenArgPassing *)arg)->solid_s);
	sem_t *sem_all = (((struct AICtrlSDNCtrlComThreadListenArgPassing *)arg)->sem_all);
	sem_t *sem_sendwait = (((struct AICtrlSDNCtrlComThreadListenArgPassing *)arg)->sem_sendwait);
	sem_t *semrd=&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_READ];
	sem_t *semwr=&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE];
	struct AICtrlInterThreadManageMsgHeader *manmsg=((struct AICtrlSDNCtrlComThreadListenArgPassing *)arg)->manmsg;
	struct AICtrlInterThreadSendMsgHeader ***sendmsg_all=((struct AICtrlSDNCtrlComThreadListenArgPassing *)arg)->sendmsg_all;
	free(arg);
	int err,i;
	int bytes_recvd_local;
	unsigned int bufsiz,msgsize;
	char *recv_buf;
	char *bufptr;
	recv_buf=malloc(256);

	{
	RecvLoopStart:
	recv_buf=realloc(recv_buf,256);
	memset(recv_buf,0,256);
			if((bytes_recvd_local = recv(s , recv_buf , 256 , MSG_PEEK)) <= 0 ){
				//Some "Error" at the Socket, like a Connection Close
				//Some Day handle it ;o)
			}

		char *endpoint;
		uintptr_t msgend;

		msgend=(unsigned int)bytes_recvd_local;
		msgend+=(uintptr_t)recv_buf;
		bufptr=strstr(recv_buf,"Content-Length:");
		bufptr+=16;
		msgsize=strtol(bufptr, &endpoint, 10);
//		for(bufptr=strstr(endpoint,"\r\n\r\n");*bufptr!='\n';bufptr++){}
//		for(bufptr;(((*bufptr)=='\n')||((*bufptr)=='\r'));bufptr++){}
		for(bufptr=endpoint;1;bufptr++){
			if((*bufptr)=='\r'){
				if((*(bufptr+1))=='\n'){
					if((*(bufptr+2))=='\r'){
						if((*(bufptr+3))=='\n'){
							bufptr+=4;
							break;
						}
					}
				}
			}
			if(((uintptr_t)bufptr) >= (uintptr_t)msgend){
				printfc(red,"WARNING:");printf(" Received malformed HTTP-Header from SDN-Ctrl.\n\tCouldn't decode it. Hence aborting this Message:\n\tClearing Socket completely...\n");
				while(recv(s , recv_buf , 256 , MSG_DONTWAIT) > 0 ){}
				goto RecvLoopStart;
			}
		}
		bufsiz=((uintptr_t)bufptr-(uintptr_t)recv_buf)+msgsize+1;//Size for one more, than is there in the socket (Null-terminating)
		recv_buf=realloc(recv_buf,bufsiz);
		bytes_recvd_local = recv(s , recv_buf , bufsiz-1 , 0);//Read one less, than the buffer size. Id est, read exactly the message size.
		recv_buf[bufsiz-1]='\0';//Now set the Null-terminating character on to the last slot.
		for(bufptr=endpoint;1;bufptr++){
			if((*bufptr)=='\r'){
				if((*(bufptr+1))=='\n'){
					if((*(bufptr+2))=='\r'){
						if((*(bufptr+3))=='\n'){
							bufptr+=4;
							break;
						}
					}
				}
			}
			if(((uintptr_t)bufptr) >= (uintptr_t)msgend){
				printfc(red,"WARNING:");printf(" Received malformed HTTP-Header from SDN-Ctrl.\n\tCouldn't decode it. Hence aborting this Message:\n\tClearing Socket completely...\n");
				while(recv(s , recv_buf , 256 , MSG_DONTWAIT) > 0 ){}
				goto RecvLoopStart;
			}
		}

		//Now we do have the whole msg and nothing more
		//	and bufptr pointing to the Beginning of the Payload
		//Next extract the content-type
		//		misuse the endpoint for the sake of memory save
		char tempbuf[128];
		endpoint=strstr(recv_buf,"Content-Type:");
		endpoint+=14;
		for(i=0;1;i++){
			if((tempbuf[i]=endpoint[i])==';')
				break;
		}
		tempbuf[i]='\0';

		if (strcmp(tempbuf, "application/json") == 0){
			sem_wait(semwr);
			manmsg->msgsize=msgsize+1;
			manmsg->idx=ABSINT_CTRL_SDN_CTRL_COM_THREAD_LISTEN;
			manmsg->type=AIC_INTERN_GET_DPIDS;
			manmsg->msg=malloc(msgsize+1);
			manmsg->flags=FLAG_ABSINT_CTRL_INTER_MSG_SDNCTRL;
			memcpy(manmsg->msg,bufptr,msgsize);
			(manmsg->msg)[msgsize]='\0';
		}else{
			//No valid msg received
			goto RecvLoopStart;
		}


		/*
		printfc(red,"||");
		printfc(yellow,"------");
		printfc(green,">>>");
		printf("Answer from the SDN-Controller");
		printfc(green,"<<<");
		printfc(yellow,"------");
		printfc(red,"||");
		printf("\n|%s|",bufptr);
		puts("");
		printfc(red,"||");
		printfc(yellow,"------");
		printfc(green,">>>");
		printf("Answer End");
		printfc(green,"<<<");
		printfc(yellow,"------");
		printfc(red,"||");
		puts("");
		*/



		sem_post(semrd);

	}
	goto RecvLoopStart;

	return NULL;
}


void *absint_ctrl_sdn_ctrl_com_thread(void* arg){
	pthread_t *all_threads = (((struct AICtrlSDNCtrlComThreadArgPassing *)arg)->all_threads_array);
	sem_t *sem_all = (((struct AICtrlSDNCtrlComThreadArgPassing *)arg)->sem_all);
	sem_t *sem_sendwait = (((struct AICtrlSDNCtrlComThreadArgPassing *)arg)->sem_sendwait);
	sem_t *sem_sdnsend_rd=&sem_all[ABSINT_CTRL_SEM_SDNSEND_MSG_READ];
	sem_t *sem_sdnsend_wr=&sem_all[ABSINT_CTRL_SEM_SDNSEND_MSG_WRITE];
	struct AICtrlInterThreadManageMsgHeader *manmsg=((struct AICtrlSDNCtrlComThreadArgPassing *)arg)->manmsg;
	struct AICtrlInterThreadSDNSendMsgHeader **sdnsend_msg=((struct AICtrlSDNCtrlComThreadArgPassing *)arg)->sdnsendmsg;
	struct AICtrlInterThreadSendMsgHeader ***sendmsg_all=((struct AICtrlSDNCtrlComThreadArgPassing *)arg)->sendmsg_all;
	#define sdnmsghp (*sdnsend_msg)
	#define sdnmsgp ((char *)((*sdnsend_msg)+1))
	free(arg);
	int err,msgsize;
	char *msg;

	CREATE_PROGRAM_PATH(*args);
	ABSINT_READ_CONFIG_FILE_CTRL;

	ABSINT_CTRL_INET_SOCKET_CONTROLLER_COM_DNS_LOOKUP;
	ABSINT_CTRL_INET_SOCKET_SDN_CONTROLLER_COM_CONNECT;

	ABSINT_CTRL_SDN_CONTROLLER_COM_LISTEN_START_THREAD;

	while(1){

	sem_wait(sem_sdnsend_rd);


	switch(sdnmsghp->type){
	case AIC_INTERN_GET_DPIDS:
		err = senddetermined(s,SDNCtrlGetSwitchesHTTPHead,sizeof(SDNCtrlGetSwitchesHTTPHead)-1);
		goto FreeSharedMemory;
		break;
	case AIC_INTERN_DUMP:
	default:
	//Generic Send. Direct Passthrough of gotten msg. Has to be prepared completely properly.
		;GenericSend:
		break;
	}

	err = senddetermined(s,msg,msgsize);
	free(msg);

	FreeSharedMemory:
	free(sdnmsghp);
	sdnmsghp=NULL;
	//FLAG_UNSET(manmsg->flags,FLAG_ABSINT_CTRL_INTER_MSG_NEW);
	sem_post(sem_sdnsend_wr);

	}

	#undef sdnmsghp
	#undef sdnmsgp
	return NULL;
}

void *absint_ctrl_each_connected_thread_listen (void* arg) {
	const int thread_index = (((struct AICtrlThreadListenArgPassing *)arg)->thread_index);
	int s = ((struct AICtrlThreadListenArgPassing *)arg)->solid_s;
	int *bytes_recvd = ((struct AICtrlThreadListenArgPassing *)arg)->bytes_recvd;
	sem_t *sem_all = ((struct AICtrlThreadListenArgPassing *)arg)->sem_all;
	sem_t *sem=&sem_all[thread_index];
	struct AICtrlInterThreadManageMsgHeader *manmsg=((struct AICtrlThreadListenArgPassing *)arg)->manmsg;
	struct AI_ChanSwitch_Couple **AICouple=((struct AICtrlThreadListenArgPassing *)arg)->AICouple;
	free(arg);
	int err;
	int pthreadoldcancelstate;
	int bytes_recvd_local;
	struct AICtrlMsgHeader recvh;
	char *msg;
	char *fsmmsg;
	
	/* You could design this dynamic, to support multiple channel distances
	 * Deliver the Data with special Msg-Types for each distance and multiplex them.
	 * Then use the right statistic or adjust the holder.
	 */
	double wlanstat_trafficstat[WLAN_TRAFFICSTAT_CHAN_NUM];
	/* Use div as Hash-Function to compress the channel-indices to the array-range
	 * I.e.: Channel 1 -> Index 0, Channel 5 -> Index 1 ...
	 */

	while( (bytes_recvd_local = recv(s , &recvh , sizeof(recvh) , 0)) > 0 ){
		if((recvh.type!=AIC_AITC_WLANSTAT_RSSI)&&
			(recvh.type!=AIC_AITC_WLANSTAT_TRAFFICSTAT)&&
			(recvh.type!=AIC_AITC_WLANSTAT_COMPLETE)){
			printf(ANSI_COLOR_RED);
			printf("-->Message from #%d: ",thread_index);
			printf(ANSI_COLOR_RESET);
			printf("%d Bytes (+%d Bytes Header). Type: ",recvh.msgsize,bytes_recvd_local);
			PRINT_AICTRL_MSGTYPE(recvh.type);puts("");
		}

							//InterThreadMessageNotReadAlready:

		/* Intercept the Channel-Switch-FSM messages and send them to the FSM-Thread
		 * Otherwise: To the Manager-Thread
		 */
		/* After every Reception of a channel-switch-FSM msg this gets send through the shared socket
		 * to the common FSM Thread -->
		 * After this send check the returned err=send() for Existence and if, then
		 * check errno for
		 * EINVAL - Invalid argument passed.
		 * 		and
		 * ENOTSOCK - The argument sockfd is not a socket.
		 * Then there was a Error in the whole Architecture: The AI sent a message for
		 * the channel-switch-FSM, although this isn't active (or maybe just the socket messed up...)
		 * Normally, "No Socket" means "No FSM Thread" means "No Channel-Switch Procedure active".
		 * This would be an error from the AI: So we ignore this msg.
		 * Note that a FSM-msg properly only can occur after the FSM on the Controller was started.
		 * Because the Controller initiates the Channel-Switch and the Start of FSMs on the AIs.
		 * This occurs not until the FSM on the Controller (and his socket) runs.
		 */
		/* You could check beforehand - before sending into socket, i.e. directly after reception
		 * of the network msg - if the FSM-Thread and/or if the socket are/is present.
		 * And if not, then immediately skip everything for this msg.
		 * But i expect, that such errors won't occur very often, if any. So there would be many
		 * checks, would consume CPU-Time without effect. So i omit this beforehand checks and
		 * just for security do the afterwards check.
		 */
		/* Sending of msg to the FSM-Thread like
		 * 1. Prepare msg (mostly simple forwarding)
		 * 2. sem_wait(socket)
		 * 3. send msg into socket
		 * 4. check error (see above)
		 * 5. sem_post(socket)
		 */
		/* The sockets and the connections are already prepared
		 * The manager prepares every socket stuff at the FSM-Thread-Creation Point
		 * The FSM-Thread cleans all sockets and connections at Termination
		 */
		switch(recvh.type){
		case AIC_AITC_WLANSTAT_TRAFFICSTAT:
			//Also intercept this.//
			msg=malloc(recvh.msgsize);
			if(recvh.msgsize>0){
				bytes_recvd_local=recv(s,msg,recvh.msgsize,0);
	//			depr(rec3,"bytes rec %d | msgsize %d | rest %d",bytes_recvd_local,recvh.msgsize,recv(s,temp,128,MSG_DONTWAIT|MSG_PEEK));
				while(bytes_recvd_local<recvh.msgsize){
					bytes_recvd_local+=recv(s,(msg)+bytes_recvd_local,(recvh.msgsize)-bytes_recvd_local,0);
	//				depr(rec4,"bytes rec %d | msgsize %d | rest %d",bytes_recvd_local,recvh.msgsize,recv(s,temp,128,MSG_DONTWAIT|MSG_PEEK));
				}
			}
			#ifdef DEBUG
//			printfc(gray,"Received WLAN-Stats: Current Freq: %d | Stat from %d: %f",
//						((struct AICtrlMsgTrafficstat *)msg)->currentfreq,
//						((struct AICtrlMsgTrafficstat *)msg)->freq,
//						((struct AICtrlMsgTrafficstat *)msg)->trafficstat);
//			puts("");
			#endif
			//Refresh my local statistics.
			int statchan;
			statchan=ieee80211_frequency_to_channel((((struct AICtrlMsgTrafficstat *)msg)->freq));
			wlanstat_trafficstat[statchan / WLAN_TRAFFICSTAT_CHAN_DIST]=
					((struct AICtrlMsgTrafficstat *)msg)->trafficstat;
			//Analyse newest statistics.
			int newfreq;
			err=analyse_wlanstatistics(&newfreq,
					((struct AICtrlMsgTrafficstat *)msg)->currentfreq,
					wlanstat_trafficstat,
					sizeof(wlanstat_trafficstat));
			//IF Channel-Switch should start: Tell Manager to do so.
			switch(err){
			case 0://Channel Switch
				err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
				sem_wait(&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE]);
				manmsg->msgsize=sizeof(msgfreq);
				manmsg->idx=thread_index;
				manmsg->type=AIC_INTERN_WLAN_SWITCH_FSM_START_CTRL_THREAD;
				manmsg->msg=malloc(sizeof(msgfreq));
				*(msgfreq *)(manmsg->msg)=(msgfreq)newfreq;
				sem_post(&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_READ]);
				err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);
				break;
			case 1://No Channel Switch: Current Channel is good enough
			case 2://No Channel Switch: Current Channel is the best channel
			case 3://No Channel Switch: No other Channel is sufficiently better
			default:
				break;
			}
			free(msg);
			msg=NULL;
			break;
		case AIC_WLAN_SWITCH_FSM_AITC_READY_TO_SWITCH:
		case AIC_WLAN_SWITCH_FSM_AITC_SWITCH_DONE_ACK:
		case AIC_WLAN_SWITCH_FSM_AITC_READY_TIMEOUT:
		case AIC_WLAN_SWITCH_FSM_AITC_DONE_TIMEOUT:
		case AIC_WLAN_SWITCH_FSM_AITC_STAY_READY_ACK:
		case AIC_WLAN_SWITCH_FSM_AITC_STAY_DONE_ACK:
		case AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_R:
		case AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_D:
		case AIC_WLAN_SWITCH_FSM_AITC_FINISH_ACK:
			fsmmsg=malloc(sizeof(struct AICtrlFSMSocketMsgHeader)+recvh.msgsize);
			((struct AICtrlFSMSocketMsgHeader *)fsmmsg)->msgsize=recvh.msgsize;
			((struct AICtrlFSMSocketMsgHeader *)fsmmsg)->thread_index=thread_index;
			((struct AICtrlFSMSocketMsgHeader *)fsmmsg)->type=recvh.type;
			if(recvh.msgsize>0){
				bytes_recvd_local=recv(s,fsmmsg+sizeof(struct AICtrlFSMSocketMsgHeader),recvh.msgsize,0);
				while(bytes_recvd_local<recvh.msgsize){
					bytes_recvd_local+=recv(s,(fsmmsg+sizeof(struct AICtrlFSMSocketMsgHeader))+bytes_recvd_local,(recvh.msgsize)-bytes_recvd_local,0);
				}
			}
			sem_wait(&((*AICouple)->sem_sock));
			int bytes_sent,fsmmsglen;
			fsmmsglen = sizeof(struct AICtrlFSMSocketMsgHeader)+(((struct AICtrlFSMSocketMsgHeader *)fsmmsg)->msgsize);
			SendFSMAgain:
			bytes_sent = send((*AICouple)->fsmsock, fsmmsg, fsmmsglen, 0);
			switch(bytes_sent) {
			case -1:
				switch(errno){
				case EINVAL:
				case ENOTSOCK:
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
			sem_post(&((*AICouple)->sem_sock));
			free(fsmmsg);
			break;
		default:
			err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
			sem_wait(&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE]);
								/*
								if(FLAG_CHECK(manmsg->flags,FLAG_ABSINT_CTRL_INTER_MSG_NEW)){//Shouldn't be able to occur, through semaphore-synchronization
									sem_post(&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE]);
									err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);
									goto InterThreadMessageNotReadAlready;
								}
								*/
			manmsg->msgsize=recvh.msgsize;
			manmsg->idx=thread_index;
			manmsg->type=recvh.type;
			manmsg->msg=malloc(manmsg->msgsize);
			//FLAG_SET(manmsg->flags,FLAG_ABSINT_CTRL_INTER_MSG_NEW);
//			char temp[128];
	//		depr(rec2,"bytes rec %d | msgsize %d | rest %d",bytes_recvd_local,recvh.msgsize,recv(s,temp,128,MSG_DONTWAIT|MSG_PEEK));
			if(recvh.msgsize>0){
				bytes_recvd_local=recv(s,manmsg->msg,manmsg->msgsize,0);
	//			depr(rec3,"bytes rec %d | msgsize %d | rest %d",bytes_recvd_local,recvh.msgsize,recv(s,temp,128,MSG_DONTWAIT|MSG_PEEK));
				while(bytes_recvd_local<recvh.msgsize){
					bytes_recvd_local+=recv(s,(manmsg->msg)+bytes_recvd_local,(manmsg->msgsize)-bytes_recvd_local,0);
	//				depr(rec4,"bytes rec %d | msgsize %d | rest %d",bytes_recvd_local,recvh.msgsize,recv(s,temp,128,MSG_DONTWAIT|MSG_PEEK));
				}
			}
			sem_post(&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_READ]);
			err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);
	//		depr(rec5,"bytes rec %d | msgsize %d | rest %d",bytes_recvd_local,recvh.msgsize,recv(s,temp,128,MSG_DONTWAIT|MSG_PEEK));
			break;
		}
    }

	printf("\tThread #%d - Connection: Socket closed with: %d | ERRNO: %d.\n", thread_index,bytes_recvd_local,errno);

	err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
	sem_wait(&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE]);
	manmsg->msgsize=0;
	manmsg->idx=thread_index;
	manmsg->type=AIC_INTERN_AI_DISCONNECT;
	manmsg->msg=NULL;

	*bytes_recvd = bytes_recvd_local;
	sem_post(sem);
	err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);


	return err;
}

void *absint_ctrl_each_connected_thread (void* arg) {
	const int thread_index = (((struct AICtrlThreadArgPassing *)arg)->thread_index);
	printf("\t#%d Connection: Starting up.\n", thread_index);
	struct AICtrlInterThreadManageMsgHeader *manmsg=((struct AICtrlThreadArgPassing *)arg)->manmsg;
	sem_t *sem_all = (((struct AICtrlThreadArgPassing *)arg)->sem_all);
	sem_t *sem_sendwait = (((struct AICtrlThreadArgPassing *)arg)->sem_sendwait);
	//sem_t *sem=&sem_all[thread_index];
	//sem_t *semsendwait=&sem_sendwait[thread_index];
	struct AI_ChanSwitch_Couple **AICouple=((struct AICtrlThreadArgPassing *)arg)->AICouple;
	int s = ((struct AICtrlThreadArgPassing *)arg)->solid_s;
	pthread_t *threads_aictrl= (((struct AICtrlThreadArgPassing *)arg)->all_threads_array);
	struct AICtrlInterThreadSendMsgHeader ***sendmsg_all=((struct AICtrlThreadArgPassing *)arg)->sendmsg_all;
	//Pointer to my semaphore
	#define sem &sem_all[thread_index]
	//Pointer to my sendwait semaphore
	#define semsendwait &sem_sendwait[thread_index]
	//The Pointer to my own sendmsg-header. This gets malloced
	#define sendmsghp (*sendmsg_all)[thread_index]
	//The Pointer to the actual message
	#define sendmsgp ((char *)((*sendmsg_all)[thread_index]+sizeof(struct AICtrlInterThreadSendMsgHeader)))
	#define msgh (*((struct AICtrlMsgHeader *)msg))
	#define msgp ((char *)(msg+sizeof(struct AICtrlMsgHeader)))
//	struct AICtrlInterThreadManageMsgHeader *manmsg=((struct AICtrlThreadArgPassing *)arg)->manmsg;
	int err;
	//NOTE: The free(arg) is included in the Thread-Starting Macro

	int pthreadoldcancelstate;
	int bytes_recvd;bytes_recvd=1;

	char *msg;

	//Every Server Thread needs an own additional Thread - a listening Thread
	//The Server Thread itself does all the checking and sending stuff
	//The listening Thread blocks at recv messages from the port. Especially listens
	//for a '0' to see if the client closed the connection.
	pthread_t absintctrl_each_listen;
	ABSINT_CTRL_START_COM_LISTEN_THREAD;

    while(1) {
    	sem_wait(sem);
    	//First check if the connection is still standing.
    	//The listening thread changes the bytes_recvd value to zero
    	//if the connection got closed from the client
		if(bytes_recvd == 0) {
			printf("\tThread #%d - Connection: Client disconnected\n", thread_index);
			fflush(stdout);
			goto ConnectionClosed;
		} else if(bytes_recvd == -1) {
			fprintf(stderr, "\tThread #%d - Connection: recv failed\n", thread_index);
			goto ConnectionClosed;
		}
		//To remember:
		//The recv function delivers a Zero if other Site closed the connection
		
//		sendmsghp;
//		sendmsgp;
		switch(sendmsghp->type){
		case AIC_CTAI_START_ADHOC:
		case AIC_CTAI_STOP_ADHOC:
		case AIC_CTAI_INQUIRE_IFACE:
		case AIC_CTAI_INQUIRE_RSSI:
			goto GenericSend;
			break;
		case AIC_CTAI_SET_ADHOC_FREQ:
			msg=malloc(sizeof(struct AICtrlMsgHeader)+(sendmsghp->msgsize));
			msgh.msgsize=(sendmsghp->msgsize);//sizeof(int)+devlength;
			msgh.type=AIC_CTAI_SET_ADHOC_FREQ;
//			*((int *)msgp)=*(int*)(sendmsgp);
			memcpy(msgp,sendmsgp,sendmsghp->msgsize);
			break;
		case AIC_INTERN_LET_AI_DISCONNECT:
			goto KillListenAndCloseConnection;
			break;
		default:
//			printfc(yellow,"\tWARNING: ");printf("Thread #%d - Message Sending of undefined Type: Type %d\n",thread_index,sendmsghp->type);
//			printf("\t--> Hence trying generic Sending. (Direct Passthrough)\n");
			GenericSend:
			msg=malloc(sizeof(struct AICtrlMsgHeader)+(sendmsghp->msgsize));
			msgh.msgsize=(sendmsghp->msgsize);
			msgh.type=sendmsghp->type;
			memcpy(msgp,sendmsgp,sendmsghp->msgsize);
//			continue;
			break;
		}

		printfc(green,"\tThread #%d",thread_index);printf(" - Sending Msg | Type: ");PRINT_AICTRL_MSGTYPE(sendmsghp->type);
		printf("\n");
//		printf(ANSI_COLOR_GREEN);
//		printf("\t  -->Thread #%d", thread_index);
//		printf(ANSI_COLOR_RESET);
//		printf(" - Sending (first Value) : ");
//		#ifdef DEBUG
//			char testdev[IFNAMSIZ];
//		#endif
//		switch(sendmsghp->type){
//		case AIC_CTAI_START_ADHOC:
//			printf("\n");
//			#ifdef DEBUG
//				memset(testdev,0,sizeof(testdev));
//				memcpy(testdev,msgp,(sizeof(testdev)<(msgh.msgsize))?sizeof(testdev):(msgh.msgsize));
//				printf("\t--> Sending Device %s\n",testdev);
//			#endif
//			break;
//		case AIC_CTAI_STOP_ADHOC:
//			printf("\n");
//			#ifdef DEBUG
//				memset(testdev,0,sizeof(testdev));
//				memcpy(testdev,msgp,(sizeof(testdev)<(msgh.msgsize))?sizeof(testdev):(msgh.msgsize));
//				printf("\t--> Sending Device %s\n",testdev);
//			#endif
//			break;
//		case AIC_CTAI_SET_ADHOC_FREQ:
//			printf("%d", *((msgfreq *)msgp));
//			printf("\n");
//			#ifdef DEBUG
//				memset(testdev,0,sizeof(testdev));
//				memcpy(testdev,(char*)((msgfreq*)(msgp)+1),(sizeof(testdev)<((msgh.msgsize)-sizeof(msgfreq)))?sizeof(testdev):((msgh.msgsize)-sizeof(msgfreq)));
//				printf("\t--> Sending Device %s\n",testdev);
//			#endif
//			break;
//		case AIC_CTAI_INQUIRE_RSSI:
//			printf("%d", *((msgfreq *)msgp));
//			printf("\n");
//			#ifdef DEBUG
//				memset(testdev,0,sizeof(testdev));
//				memcpy(testdev,(char*)((msgfreq*)(msgp)+1),(sizeof(testdev)<((msgh.msgsize)-sizeof(msgfreq)))?sizeof(testdev):((msgh.msgsize)-sizeof(msgfreq)));
//				printf("\t--> Sending Device %s\n",testdev);
//			#endif
//			break;
//		default:
//			printf("\n");
//			#ifdef DEBUG
//				memset(testdev,0,sizeof(testdev));
//				memcpy(testdev,msgp,(sizeof(testdev)<(msgh.msgsize))?sizeof(testdev):(msgh.msgsize));
//				printf("\t--> Sending Device %s\n",testdev);
//			#endif
//			break;
//		}
		err = senddetermined(s,msg,sizeof(struct AICtrlMsgHeader)+(sendmsghp->msgsize));

										//Wait a bit before next check
								//		struct timespec remainingdelay;
								//		remainingdelay.tv_sec = 10;
								//		remainingdelay.tv_nsec = 0;
								//		do {
								//			err = nanosleep(&remainingdelay, &remainingdelay);
								//		} while (err<0);
								
		free(sendmsghp);
		sendmsghp=NULL;
		free(msg);
		sem_post(semsendwait);
    }
    KillListenAndCloseConnection:
	pthread_cancel(absintctrl_each_listen);
	if(sendmsghp)
		free(sendmsghp);
	sendmsghp=NULL;
	sem_post(semsendwait);
	sem_wait(&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE]);
	manmsg->msgsize=0;
	manmsg->idx=thread_index;
	manmsg->type=AIC_INTERN_AI_DISCONNECT;
	manmsg->msg=NULL;
    ConnectionClosed:
	#undef sendmsghp
	#undef sendmsgp
	#undef semsendwait
	#undef msgh
	#undef msgp
	close(s);
	sem_post(&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_READ]);
	sem_wait(sem);
	#undef sem


	printf("\tThread #%d - Connection: Terminating.\n", thread_index);
	printf("\tReady for new Connection on Thread Place #%d. Awaiting...\n", thread_index);
	threads_aictrl[thread_index]=0;

	return err;
}


int absint_controller(int argc, char **argstart,struct nl80211_state *sktctr,struct CommandContainer *cmd){
	int err;err=0;

	int i;

	printf("\nStarting Abstract Interface Controller...\n");
	printf("...to manage connecting Abstract Interfaces.\n");
	puts("");


	CREATE_PROGRAM_PATH(*args);
	ABSINT_READ_CONFIG_FILE_CTRL;

	if(argc>0){
		if (strcmp(*(argstart), "debug") == 0){
			int err;
			if (argc<=1){
				printf("\nAbsInt Ctrl: To few Arguments on \"debug\"\n");
				return MAIN_ERR_BAD_CMDLINE;
			} else {
				puts("");
				printfc(yellow,"AbsInt Ctrl: ");
				printfc(red,"Debug Command called\n");
				puts("");
				argc--;
				argstart++;
				if (strcmp(*(argstart), "testsdnctrlcom") == 0){
					err=absint_ctrl_debug_testsdnctrlcomm();
					return err;
				} else if (strcmp(*(argstart), "dpidrefresh") == 0){
					return MAIN_ERR_NONE;
				} else if (strcmp(*(argstart), "temp") == 0){
					//some temporary tested function
					err=test_socket_comm_to_chan_switch_fsm();
					return MAIN_ERR_NONE;
				} else {
					printf("Invalid Command after \"absint controller debug\"\n");
					return MAIN_ERR_BAD_CMDLINE;
				}
			}
		}
	}

	pthread_t threads_aictrl[ABSINT_CTRL_THREADS_ID_MAX]; /*to store the references to active connections*/
	/*Don't forget the Mutex-handling on this thing, if the necessity comes in place for you!*/
	memset(threads_aictrl, 0, sizeof(threads_aictrl));
	/* As long as an Entry is '0', he isn't coupled yet.
	 * When assigned it points to a Couple struct
	 */
	struct AI_ChanSwitch_Couple *AICouples[ABSINT_CTRL_THREADS_MAX];
	memset(AICouples,0,sizeof(AICouples));

	printf("Initialize Semaphores...");
	sem_t sem_aictrl[ABSINT_CTRL_SEM_MAX];
//	printf(" (# %d)\n",sizeof(sem_aictrl)/sizeof(sem_t));
//	for(i=0;i<(sizeof(sem_aictrl)/sizeof(sem_t));i++)
	printf(" (# %d)\n",ABSINT_CTRL_SEM_MAX);
	for(i=0;i<ABSINT_CTRL_SEM_INIT_LOOP;i++)
		sem_init(&sem_aictrl[i],0,ABSINT_CTRL_SEM_INIT_VAL);
	//The Semaphores for the SendWait-Synchronization between the Manager and the Sending-Threads
	sem_init(&sem_aictrl[ABSINT_CTRL_SEM_MANAGE_MSG_READ],0,0);
	sem_init(&sem_aictrl[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE],0,1);
	sem_init(&sem_aictrl[ABSINT_CTRL_SEM_SDNSEND_MSG_READ],0,0);
	sem_init(&sem_aictrl[ABSINT_CTRL_SEM_SDNSEND_MSG_WRITE],0,1);
	//This Semaphore is used for Mutex at the Start of a Channel-Switch-FSM-Thread
	//Some Mutex at this Point it necessary between the two grouped AI-Threads
	//Like this here, using only one Semaphore for ALL FSM Starts, there can only on
	//FSM-Thread be started at a time. This isn't perfect for parallelized Runtime.
	//But otherwise we would something like "more Semaphores at every time" or a concept
	//for "Semaphore Creation and Destroying". This would have other negative issues to
	//RAM-Space and/or Runtime and so would also not be 'perfect'. Another Point is, that
	//probably there won't be that many concurrent FSM-Starts and the corresponding critical sections
	//are some short. Due to this all i this decided to choose this Kind of sequential
	//coupling at FSM-Thread Start.
	//Ähm, no, command back. No Semaphore needed. The sequential coupling is secured over
	//the manager thread.
//	sem_init(&sem_aictrl[ABSINT_CTRL_SEM_CHANSWITCH_FSM_START],0,1);
	sem_t sem_sendwait[ABSINT_CTRL_THREADS_MAX+1];//+1 for the CLI-Thread
	for(i=0;i<ABSINT_CTRL_THREADS_MAX+1;i++)
		sem_init(&sem_sendwait[i],0,1);

	struct AICtrlInterThreadManageMsgHeader InterThreadManagerMsg;
	struct AICtrlInterThreadSDNSendMsgHeader *InterThreadSDNSendMsg;
	struct AICtrlInterThreadSendMsgHeader **sendmsg_all=malloc(sizeof(void *));
	struct AICtrlConnectedAI *AIConnections=malloc(sizeof(struct AICtrlConnectedAI));
	ABSINT_CTRL_THREAD_START_MANAGER;

	ABSINT_CTRL_THREAD_START_CLI;

	//TODO: Uncomment
	ABSINT_CTRL_SDN_CONTROLLER_COM_START_THREAD;

	ABSINT_CTRL_INET_SOCKET_SERVER_SETUP;

	struct AICtrlThreadArgPassing *pthreadArgPass;
	int firstfreethreadindex;
    while( s=accept(ls,(struct sockaddr *)&peeraddr_in,(socklen_t*)&addrlen) ){
        if (s<0) {
            perror("accept failed");
            return 1;
        }
        puts("");
		puts("Incoming Connection accepted");

		//Now, first refresh our known DPIDs
		#define sem_sdnsend_rd (&sem_aictrl[ABSINT_CTRL_SEM_SDNSEND_MSG_READ])
		#define sem_sdnsend_wr (&sem_aictrl[ABSINT_CTRL_SEM_SDNSEND_MSG_WRITE])
		#define sdnmsghp (InterThreadSDNSendMsg)
		DPID_REFRESH
		#undef sem_sdnsend_rd
		#undef sem_sdnsend_wr
		#undef sdnmsghp

		//Following is a malloc. Necessary for passing the needed arguments to the newly
		//created threads. (Every thread needs his own arg passing mem space, so that this
		//isn't modified before the thread read it out... (vgl. Mutex)
		//So remember to free this space inside the thread, when the args are read out
		pthreadArgPass = malloc(sizeof(struct AICtrlThreadArgPassing));
		pthreadArgPass->solid_s = s;

		if((firstfreethreadindex=get_first_free_threads_index(threads_aictrl)) == -1) {
			//TODO: Real usable handling of this...
			//Insert something here like a single message to tell the client to try again later
			//or just a connection refuse and for this case the whole handling inside the client.
			printf("PROBLEM: No free Thread-Index present!\n");
			continue;
		}
		pthreadArgPass->sem_all = sem_aictrl;
		pthreadArgPass->sem_sendwait = sem_sendwait;
		pthreadArgPass->thread_index = firstfreethreadindex;
		pthreadArgPass->all_threads_array = threads_aictrl;
		pthreadArgPass->manmsg=&InterThreadManagerMsg;
		pthreadArgPass->sendmsg_all=&sendmsg_all;
		pthreadArgPass->AICouple=&AICouples[firstfreethreadindex];
		sem_getvalue(&sem_aictrl[firstfreethreadindex],&err);
		if(err!=ABSINT_CTRL_SEM_INIT_VAL){
			sem_destroy(&sem_aictrl[firstfreethreadindex]);
			sem_init(&sem_aictrl[firstfreethreadindex],0,ABSINT_CTRL_SEM_INIT_VAL);
			err=0;
		}
		sem_getvalue(&sem_sendwait[firstfreethreadindex],&err);
		if(err!=1){
			sem_destroy(&sem_sendwait[firstfreethreadindex]);
			sem_init(&sem_sendwait[firstfreethreadindex],0,1);
			err=0;
		}
		
		err=get_last_used_threads_index(threads_aictrl);
		if(firstfreethreadindex>err){
			sendmsg_all=realloc(sendmsg_all,(firstfreethreadindex+1)*sizeof(void *));
			AIConnections=realloc(AIConnections,(firstfreethreadindex+1)*sizeof(struct AICtrlConnectedAI));
		}else {
			sendmsg_all=realloc(sendmsg_all,(err+1)*sizeof(void *));
			AIConnections=realloc(AIConnections,(err+1)*sizeof(struct AICtrlConnectedAI));
		}
		((sendmsg_all)[firstfreethreadindex])=NULL;
//		AIConnections[firstfreethreadindex].ifc=0;
//		AIConnections[firstfreethreadindex].iface=NULL;
//		memset(AIConnections[firstfreethreadindex].wneigh24,0,sizeof(AIConnections[firstfreethreadindex].wneigh24));
		memset(&AIConnections[firstfreethreadindex],0,sizeof(struct AICtrlConnectedAI));
		
		printf("Create thread with Index: %d\n", firstfreethreadindex);

		pthread_attr_t tattr;
		// initialized with default attributes
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
		//Not any semaphore, synchronization, mutex needed for now
		//refer to the big comment-block above
		//secure detached-state for the threads over attributed creation
		if( (err=pthread_create(&threads_aictrl[firstfreethreadindex], &tattr, absint_ctrl_each_connected_thread, (void*)pthreadArgPass)) < 0) {
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

		//Let the Manager do the WLAN-Topology-Inter-AI-Connection-Stuff
		sem_wait(&sem_aictrl[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE]);
		InterThreadManagerMsg.msgsize=0;
		InterThreadManagerMsg.idx=0;//Doesn't really care here...
		InterThreadManagerMsg.type=AIC_INTERN_DO_WLAN_TOPOLOGY_CONNECTIONS;
		InterThreadManagerMsg.msg=NULL;
    	sem_post(&sem_aictrl[ABSINT_CTRL_SEM_MANAGE_MSG_READ]);


    }
    if (s<0) {
        perror("accept failed");
        return 1;
    }

	pthread_exit(NULL);


	return err;
}



#undef NO_ABSINT_CTRL_C_FUNCTIONS
