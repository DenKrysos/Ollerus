#ifndef ABSINT_CTRL_H
#define ABSINT_CTRL_H

/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */

#include <sys/stat.h>
#include <arpa/inet.h>

#include "ollerus_globalsettings.h"
#include "absint_ctrl_msgs.h"



#define ABSINT_CTRL_THREADS_MAX 4096 //The number of allowed Threads for Abstract Interface Connections (i.e. How many AIs are allowed to connect and get managed)
#define ABSINT_CTRL_THREADS_ID_MAX (ABSINT_CTRL_THREADS_MAX+4) //The number of reserved pthread-id slots
#define ABSINT_CTRL_CLI_THREAD (ABSINT_CTRL_THREADS_MAX)
#define ABSINT_CTRL_SDN_CTRL_COM_THREAD (ABSINT_CTRL_CLI_THREAD+1)
#define ABSINT_CTRL_SDN_CTRL_COM_THREAD_LISTEN (ABSINT_CTRL_SDN_CTRL_COM_THREAD+1)
#define ABSINT_CTRL_MANAGER_THREAD (ABSINT_CTRL_SDN_CTRL_COM_THREAD_LISTEN+1)
//Use this at initialization of the Semaphore Array:
#define ABSINT_CTRL_SEM_MAX (ABSINT_CTRL_THREADS_MAX+4)
#define ABSINT_CTRL_SEM_INIT_LOOP (ABSINT_CTRL_THREADS_MAX)
//Use these as Indices in the Array, to address the specific Semaphores
#define ABSINT_CTRL_SEM_MANAGE_MSG_READ (ABSINT_CTRL_THREADS_MAX)
#define ABSINT_CTRL_SEM_MANAGE_MSG_WRITE (ABSINT_CTRL_SEM_MANAGE_MSG_READ+1)
#define ABSINT_CTRL_SEM_SDNSEND_MSG_READ (ABSINT_CTRL_SEM_MANAGE_MSG_WRITE+1)
#define ABSINT_CTRL_SEM_SDNSEND_MSG_WRITE (ABSINT_CTRL_SEM_SDNSEND_MSG_READ+1)
//#define ABSINT_CTRL_SEM_CHANSWITCH_FSM_START (ABSINT_CTRL_SEM_SDNSEND_MSG_WRITE+1)

#define ABSINT_CTRL_SEM_INIT_VAL 0






struct AICtrlConnectedAIIface{
	char iface[IFNAMSIZ];
	struct in_addr IP4;
	char adhocssid[33];
	msgfreq freq;
//	int bitrate;//Hm, would be nice. But how could one determine a Bitrate in an ad-hoc net? :-/
	uint16_t packerrrateTX;//Packet-Error-Rate in % * 10 (e.g. a Rate of 3.4% would be stored as 34)
};
struct AICtrlConnectedAIWNeighChan{
	unsigned int count;
	struct AICtrlConnectedAIWlanNeighbour *start;
	double trafficstat;
};
struct AICtrlConnectedAIWlanNeighbour{
	char mac[6];
	int8_t rssi;
};
struct AICtrlConnectedAI{
	uint64_t dpid;//The ID of the OVS on the AI-Network-Device. It's the numerical representation of the MAC-Adress of the Interface with IfIndex '0'
	int aip;//The Index of the AI-Partner of this AI. I.e. with which other AI this one is connected. It's the Index in the Threads-Array.
	int ifc;//Number of Interfaces of the connected AI
	struct AICtrlConnectedAIIface *iface;
	struct AICtrlConnectedAIWNeighChan wneigh24[13];//Array Index represents the WLAN Channel (2.4 GHz)
};
struct AI_ChanSwitch_Couple{
	int AI1_thread_index;
	int AI2_thread_index;
	msgswitchid instanceid;
	pthread_t FSM;
	int fsmsock;
	sem_t sem_sock;
};
/* The Socket Semaphore is initialized at the creation (i.e. malloc) of the actual strucute entity (-> At the Creation of the WLAN Topology)
 * Remember to destroy it, if the Topology changes, or when a AI disconnects.
 */
// Old Version
//struct AI_ChanSwitch_Couple{
//	int AI1_thread_index;
//	int AI2_thread_index;
//	pthread_t FSM;
//	sem_t fsm_wakeup;
//	sem_t fsm_ranthrough;
//	struct AICtrlInterThreadFSMMsgHeader fsmshmem;
//};
struct Wlanstat_Trafficstat_dist3{
	int chan1;
	int chan4;
	int chan7;
	int chan10;
	int chan13;
};
struct Wlanstat_Trafficstat_dist4{
	int chan1;
	int chan5;
	int chan9;
	int chan13;

};
struct Wlanstat_Trafficstat_dist5{
	int chan1;
	int chan6;
	int chan11;
};






#define CHANSWITCH_FSM_STATE_CTRL_START 0
#define CHANSWITCH_FSM_STATE_CTRL_AI1READY 1
#define CHANSWITCH_FSM_STATE_CTRL_AI2READY 2
#define CHANSWITCH_FSM_STATE_CTRL_ACCOMPLISHOVSSWITCH 3
#define CHANSWITCH_FSM_STATE_CTRL_ACCOMPLISHOVSSWITCH_DONE1 31
#define CHANSWITCH_FSM_STATE_CTRL_ACCOMPLISHOVSSWITCH_DONE2 32
#define CHANSWITCH_FSM_STATE_CTRL_WAITFORDONEACK 4
#define CHANSWITCH_FSM_STATE_CTRL_AI1DONE 5
#define CHANSWITCH_FSM_STATE_CTRL_AI2DONE 6
#define CHANSWITCH_FSM_STATE_CTRL_WAITFORFINISH 7
#define CHANSWITCH_FSM_STATE_CTRL_AI1FINISH 8
#define CHANSWITCH_FSM_STATE_CTRL_AI2FINISH 9
#define CHANSWITCH_FSM_STATE_CTRL_FINAL 10









#define ABSINT_READ_CONFIG_FILE_CTRL printf("\tReading in the config-file...\n"); \
		umask(0); \
    	struct in_addr dnsserverip4; \
    	/*uintptr_t sdnctrldns;*/ \
    	char *sdnctrldns; \
    	sdnctrldns=NULL; \
    	int sdnctrlport; \
    	int aictrlport; \
    	struct in_addr controlleraddress; \
        /* Maybe Open a Scope for Variables, which won't be needed forever */ \
		/* Remember to close the Scope somewhere after the Macro-Usage */ \
		/* If there aren't any Variables like this just close it immediately after the Macro Usage */ \
		/* or delete the Line with the opening Bracket ( { \ ) */ \
		\
        /* Open a Scope for the Config-related Variables (Read-Buffer, File Pointer...) */ \
        /* Therewith they get deallocated after the File-Readout */ \
    	{ \
		char cfg_file[strlen(ProgPath)+strlen(ABSINT_CFG_FILE_PATH_PREFIX)+strlen(ABSINT_CFG_FILE_NAME)+strlen(ABSINT_CFG_FILE_PATH_SUFFIX)+1]; \
		memcpy(cfg_file,ProgPath,strlen(ProgPath)); \
		memcpy(cfg_file+strlen(ProgPath),ABSINT_CFG_FILE_PATH_PREFIX,strlen(ABSINT_CFG_FILE_PATH_PREFIX)); \
		memcpy(cfg_file+strlen(ProgPath)+strlen(ABSINT_CFG_FILE_PATH_PREFIX),ABSINT_CFG_FILE_NAME,strlen(ABSINT_CFG_FILE_NAME)); \
		memcpy(cfg_file+strlen(ProgPath)+strlen(ABSINT_CFG_FILE_PATH_PREFIX)+strlen(ABSINT_CFG_FILE_NAME),ABSINT_CFG_FILE_PATH_SUFFIX,strlen(ABSINT_CFG_FILE_PATH_SUFFIX)); \
		cfg_file[sizeof(cfg_file)-1]='\0'; \
	    FILE *cfgf; \
	    cfgf = fopen(cfg_file, "r"); \
	    if (!cfgf) { \
			char cfg_file_dir[strlen(ProgPath)+strlen(ABSINT_CFG_FILE_PATH_PREFIX)+1]; \
	    	switch(errno) { \
	    		case EACCES: \
	    			printf("\tERROR: Couldn't open cfg-File!\n\t\tReason:Permission denied!\n"); \
	    		break; \
	    		case ENOENT: \
	    			printf("\tERROR: Cfg-File doesn't exist!\n"); \
	    			printf("\tCreating new one:\n\t\twhile setting every Entry to auto...\n"); \
					/*NOT Exclude the preceding '/' from the folder-path*/ \
					memcpy(cfg_file_dir,ProgPath,strlen(ProgPath)); \
					memcpy(cfg_file_dir+strlen(ProgPath),ABSINT_CFG_FILE_PATH_PREFIX,strlen(ABSINT_CFG_FILE_PATH_PREFIX)); \
					cfg_file_dir[sizeof(cfg_file_dir)-1]='\0'; \
					/*Check the complete Path and eventually create the folders*/ \
					CREATE_COMPLETE_FOLDER_PATH(cfg_file_dir); \
	    		    cfgf = fopen(cfg_file, "w+"); \
	    		    if(!cfgf){ \
	    		    	printf("\t\tERROR: Couldn't create new cfg-file!\n"); \
	    		    	return FILE_ERR_PERMISSION_DENIED; \
	    		    } \
    		    	printf("\t\tSetting up config-file with:\n"); \
					printf("\t\t\t#AIConnectMode=auto\n"); \
					fprintf(cfgf, "\n#AIConnectMode=auto"); \
	    			fprintf(cfgf, "\n*INFO: The AIs have to communicate with each other. This Value describes how they set up the connection"); \
	    			fprintf(cfgf, "\n*INFO: Allowed Values:"); \
	    			fprintf(cfgf, "\n*INFO: server: This AI starts a server, to which other AIs can connect."); \
	    			fprintf(cfgf, "\n*INFO: client: This AI assumes that one AI on the other Side of the physical Connection runs as a Server and trys to connect to it."); \
	    			fprintf(cfgf, "\n*INFO: auto: Automatic Detection and maybe setting up of a server or connecting to an existing one."); \
	    			printf("\t\t\t#DNSServerIP4=172.10.0.100\n"); \
	    			fprintf(cfgf, "\n#DNSServerIP4=172.10.0.100"); \
	    			printf("\t\t\t#SDNControllerIPorDNS=dns\n"); \
	    			fprintf(cfgf, "\n#SDNControllerIPorDNS=dns"); \
	    			fprintf(cfgf, "\n*INFO: dns: AI takes the String at SDNControllerAddrDNS and does a DNS Lookup"); \
	    			fprintf(cfgf, "\n*INFO: ip4: AI uses directly the IPv4 Address from SDNControllerAddrIP4"); \
	    			printf("\t\t\t#SDNControllerAddrDNS=sdncontroller.cocos.de\n"); \
					fprintf(cfgf, "\n#SDNControllerAddrDNS=sdncontroller.cocos.de"); \
					printf("\t\t\t#SDNControllerAddrIP4=192.168.1.1\n"); \
					fprintf(cfgf, "\n#SDNControllerAddrIP4=192.168.1.1"); \
					inet_pton(AF_INET,"192.168.1.1",&controlleraddress); \
					printf("\t\t\t#SDNControllerPort=%d\n",ABSINT_SDN_CTRL_STD_PORT); \
					fprintf(cfgf, "\n#SDNControllerPort=%d",ABSINT_SDN_CTRL_STD_PORT); \
					sdnctrlport=ABSINT_SDN_CTRL_STD_PORT; \
					printf("\t\t\t#AIControllerPort=%d\n",ABSINT_AI_CTRL_STD_PORT); \
					fprintf(cfgf, "\n#AIControllerPort=%d",ABSINT_AI_CTRL_STD_PORT); \
					aictrlport=ABSINT_AI_CTRL_STD_PORT; \
	    			fprintf(cfgf, "\n*INFO: AI-Ctrl is autonomous Program, that runs on the same Machine as the SDN-Ctrl"); \
	    			fprintf(cfgf, "\n*INFO: The AI-Ctrl communicates on this Machine with the SDN-Ctrl"); \
	    			fprintf(cfgf, "\n*INFO: Every AI communicates over the Network with the AI-Ctrl"); \
	    			fprintf(cfgf, "\n*INFO: So the AI-Ctrl Port is used between every AI and the AI-Ctrl"); \
	    			fprintf(cfgf, "\n*INFO: And the SDN-Ctrl Port is used only from the AI-Ctrl; between the AI-Ctrl and the SDN-Ctrl"); \
					printf("\t\t\t#WLANChanTrafficMonS=0\n"); \
					fprintf(cfgf, "\n#WLANChanTrafficMonS=0"); \
					printf("\t\t\t#WLANChanTrafficMonNS=500000000\n"); \
					fprintf(cfgf, "\n#WLANChanTrafficMonNS=500000000"); \
					printf("\t\t\t#AIPartnerCount=1\n"); \
					fprintf(cfgf, "\n#AIPartnerCount=1"); \
					printf("\t\t\t#AIPartnerAddrSrc=auto\n"); \
					fprintf(cfgf, "\n#AIPartnerAddrSrc=auto"); \
					printf("\t\t\t#AIPartnerAddrIP4=10.0.1.20\n"); \
					fprintf(cfgf, "\n#AIPartnerAddrIP4=10.0.1.20"); \
					printf("\t\t\t#AIPartnerPort=%d\n",ABSINT_AI_PARTNER_STD_PORT); \
					fprintf(cfgf, "\n#AIPartnerPort=%d",ABSINT_AI_PARTNER_STD_PORT); \
					fprintf(cfgf, "\n"); \
	    	    	fclose(cfgf); \
	    			printf("\t\t...created cfg-file successfully!\n"); \
	    			printfc(red,"\tProgram is now exiting!\n"); \
	    			printfc(red,"\t\t-->"); \
	    			printfc(yellow," -->\n"); \
	    			printfc(yellow,"\tPlease configure the cfg-file and start again!\n"); \
	    			printf("Exit.\n"); \
	    			exit(0); \
	    		break; \
				default: \
					fprintf(stderr, "Ups, seems like we've encountered a case, which isn't caught yet :o("); \
					return MAIN_ERR_FUNC_INCOMPLETE; \
				break; \
	    	} \
	    } else { \
	    	char buf[128]; \
	    	memset(buf,0,sizeof(buf)); \
	    	while(1) { \
	    		int loopcnt; \
	    		int readval; \
				if ((readval = getc(cfgf)) == EOF) { \
				 printf("\t...Read completely through config-file.\n"); \
				 break; \
				} \
				buf[0]=(char)readval; \
				if (buf[0] == '#') { \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '=') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
				} \
				if(strcmp(buf,"SDNControllerAddrIP4")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					inet_pton(AF_INET,buf,&controlleraddress); \
				} else if(strcmp(buf,"SDNControllerPort")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					if(strcmp(buf,"auto")==0){ \
					}else{ \
						char *endpoint; \
						sdnctrlport = strtol(buf, &endpoint, 10); \
						if (*endpoint) { \
							printf("ERROR: Unsupported Format in cfg-File at:\n\tSDNControllerPort\n"); \
							return MAIN_ERR_BAD_CMDLINE; \
						} \
					} \
				} else if(strcmp(buf,"AIControllerPort")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					if(strcmp(buf,"auto")==0){ \
					}else{ \
						char *endpoint; \
						aictrlport = strtol(buf, &endpoint, 10); \
						if (*endpoint) { \
							printf("ERROR: Unsupported Format in cfg-File at:\n\tAIControllerPort\n"); \
							return MAIN_ERR_BAD_CMDLINE; \
						} \
					} \
				} else if(strcmp(buf,"DNSServerIP4")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					inet_pton(AF_INET,buf,&dnsserverip4); \
				} else if(strcmp(buf,"SDNControllerIPorDNS")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					if(strcmp(buf,"dns")==0){ \
						/*sdnctrldns=malloc(256);*/ \
						sdnctrldns=(void *)1; \
					} \
					/*Maybe add more here in the future to differ IP4 and IP6*/ \
				} else if(strcmp(buf,"SDNControllerAddrDNS")==0){ \
					/*For this Method it is mighty, mighty important, that #SDNControllerIPorDNS comes before #SDNControllerAddrDNS in the cfg-File*/ \
					/* Or it screws up */ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					if(sdnctrldns!=NULL){ \
						sdnctrldns=malloc(loopcnt+1); \
						memcpy(sdnctrldns,buf,loopcnt+1); \
					} \
				} \
	    	} \
	    	if (fclose(cfgf)) { \
	        	printf("NOTICE: Config-File couldn't be closed successfully!"); \
	        } else { \
	        	printf("...config-file read successfully!\n"); \
	        } \
	    } \
		/* Close the Scope for the Config-related Variables */ \
		}




#define ABSINT_CTRL_INET_SOCKET_SERVER_SETUP \
		int s; /* connected socket descriptor */ \
		int ls; /* listen socket descriptor */ \
		 \
		struct sockaddr_in myaddr_in; \
		struct sockaddr_in peeraddr_in; \
		 \
		int addrlen; \
		 \
		printf("\nGetting ready to listen for incoming connections...\n"); \
		/* clear out address structures */ \
		memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in)); \
		memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in)); \
		/* Set up address structure for the listen socket. */ \
		printf("\tsockaddr_in struct for port listening...\n"); \
		myaddr_in.sin_family = AF_INET; \
		printf("\t...Socket Family to: %d\n", myaddr_in.sin_family); \
		printf("\t\t-To Check->Should be: %d (AF_INET)\n", AF_INET); \
		myaddr_in.sin_addr.s_addr = INADDR_ANY; \
		printf("\t...Listening to wildcard-address: %d\n", myaddr_in.sin_addr.s_addr); \
		 \
		myaddr_in.sin_port = aictrlport; \
		printf("\t...Port number: %d\n", myaddr_in.sin_port); \
		 \
		printf("\tCreating listening socket...\n"); \
		ls = socket (AF_INET, SOCK_STREAM, 0); \
		if (ls == -1) { \
			perror(*args); \
			fprintf(stderr, "%s: unable to create socket\n" , *args); \
			exit(1); \
		} \
		printf("\t...Socket created!\n"); \
		/* Bind the listen address to the socket. */ \
		printf("\tBinding listen Address to socket...\n"); \
		if (bind(ls, &myaddr_in, sizeof(struct sockaddr_in)) == -1) { \
			perror(args[0]); \
			fprintf(stderr, "%s: unable to bind address\n", args[0]); \
			exit(1); \
		} \
		printf("\t...bound!\n"); \
		printf("\tInitiating listening on socket...\n"); \
		if (listen(ls, 5) == -1) { \
			perror(args[0]); \
			fprintf(stderr, "%s: unable to listen on socket\n",args[0]); \
			exit(1); \
		} \
		printf("...Initiation complete!\n"); \
		printf("\nNOTE: Maximum possible Threads are predefined: %d\n",ABSINT_CTRL_THREADS_MAX); \
		printf("\t(Which determines the max number of parallel connected Abstract Interfaces.)\n"); \
		 \
	    addrlen = sizeof(struct sockaddr_in); \
	    \
		printf("\nWaiting for incoming Connections...\n");







#define ABSINT_CTRL_THREAD_START_MANAGER { \
	struct AICtrlManageThreadArgPassing *managerArgPass; \
	 \
	managerArgPass = malloc(sizeof(struct AICtrlManageThreadArgPassing)); \
	managerArgPass->all_threads_array=threads_aictrl; \
	managerArgPass->sem_all=sem_aictrl; \
	managerArgPass->sem_sendwait = sem_sendwait; \
	managerArgPass->manmsg=&InterThreadManagerMsg; \
	managerArgPass->sdnsendmsg=&InterThreadSDNSendMsg; \
	managerArgPass->sendmsg_all=&sendmsg_all; \
	managerArgPass->AIConnections=&AIConnections; \
	managerArgPass->AICouples=AICouples; \
	 \
	pthread_attr_t tattr; \
	if((err=pthread_attr_init(&tattr)) < 0) { \
		perror("could not create thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
	if((err=pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)) < 0){ \
		perror("could not modify thread-attribute"); \
		if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_create(&threads_aictrl[ABSINT_CTRL_MANAGER_THREAD], &tattr, absint_ctrl_manager_thread, (void*)managerArgPass)) < 0) { \
		perror("could not create thread"); \
		if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
		perror("could not destroy thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
}




#define ABSINT_CTRL_SDN_CONTROLLER_COM_START_THREAD { \
		struct AICtrlSDNCtrlComThreadArgPassing *SDNComArgPass; \
		 \
		SDNComArgPass = malloc(sizeof(struct AICtrlSDNCtrlComThreadArgPassing)); \
		SDNComArgPass->all_threads_array=threads_aictrl; \
		SDNComArgPass->sem_all=sem_aictrl; \
		SDNComArgPass->sem_sendwait = sem_sendwait; \
		SDNComArgPass->manmsg=&InterThreadManagerMsg; \
		SDNComArgPass->sdnsendmsg=&InterThreadSDNSendMsg; \
		SDNComArgPass->sendmsg_all=&sendmsg_all; \
		 \
		pthread_attr_t tattr; \
		if((err=pthread_attr_init(&tattr)) < 0) { \
			perror("could not create thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		if((err=pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)) < 0){ \
			perror("could not modify thread-attribute"); \
			if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
				perror("could not destroy thread-attribute"); \
				return MAIN_ERR_STD; \
			} \
			return MAIN_ERR_STD; \
		} \
		if( (err=pthread_create(&threads_aictrl[ABSINT_CTRL_SDN_CTRL_COM_THREAD], &tattr, absint_ctrl_sdn_ctrl_com_thread, (void*)SDNComArgPass)) < 0) { \
			perror("could not create thread"); \
			if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
				perror("could not destroy thread-attribute"); \
				return MAIN_ERR_STD; \
			} \
			return MAIN_ERR_STD; \
		} \
		if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
}




#define ABSINT_CTRL_SDN_CONTROLLER_COM_LISTEN_START_THREAD { \
		struct AICtrlSDNCtrlComThreadListenArgPassing *SDNComArgPass; \
		 \
		SDNComArgPass = malloc(sizeof(struct AICtrlSDNCtrlComThreadListenArgPassing)); \
		SDNComArgPass->solid_s = s; \
		SDNComArgPass->sem_all=sem_all; \
		SDNComArgPass->sem_sendwait = sem_sendwait; \
		SDNComArgPass->manmsg=manmsg; \
		SDNComArgPass->sendmsg_all=sendmsg_all; \
		 \
		pthread_attr_t tattr; \
		if((err=pthread_attr_init(&tattr)) < 0) { \
			perror("could not create thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		if((err=pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)) < 0){ \
			perror("could not modify thread-attribute"); \
			if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
				perror("could not destroy thread-attribute"); \
				return MAIN_ERR_STD; \
			} \
			return MAIN_ERR_STD; \
		} \
		if( (err=pthread_create(&all_threads[ABSINT_CTRL_SDN_CTRL_COM_THREAD_LISTEN], &tattr, absint_ctrl_sdn_ctrl_com_thread_listen, (void*)SDNComArgPass)) < 0) { \
			perror("could not create thread"); \
			if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
				perror("could not destroy thread-attribute"); \
				return MAIN_ERR_STD; \
			} \
			return MAIN_ERR_STD; \
		} \
		if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
}




#define ABSINT_CTRL_START_COM_LISTEN_THREAD {/*Scope*/ \
struct AICtrlThreadListenArgPassing *pthreadArgPass; \
\
pthreadArgPass = malloc(sizeof(struct AICtrlThreadListenArgPassing)); \
pthreadArgPass->solid_s = s; \
pthreadArgPass->bytes_recvd = &bytes_recvd; \
pthreadArgPass->sem_all = sem_all; \
pthreadArgPass->thread_index=thread_index; \
pthreadArgPass->manmsg=((struct AICtrlThreadArgPassing *)arg)->manmsg; \
pthreadArgPass->AICouple=AICouple; \
/*Free the arguments passing memory space*/ \
free(arg); \
\
pthread_attr_t tattr; \
/*initialized with default attributes*/ \
if((err=pthread_attr_init(&tattr)) < 0) { \
	perror("could not create thread-attribute"); \
	return MAIN_ERR_STD; \
} \
if((err=pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)) < 0){ \
	perror("could not modify thread-attribute"); \
	if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
		perror("could not destroy thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
	return MAIN_ERR_STD; \
} \
/*secure detached-state for the threads over attributed creation*/ \
if( (err=pthread_create(&absintctrl_each_listen, &tattr, absint_ctrl_each_connected_thread_listen, (void*)pthreadArgPass)) < 0) { \
	perror("could not create thread"); \
	if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
		perror("could not destroy thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
	return MAIN_ERR_STD; \
} \
if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
	perror("could not destroy thread-attribute"); \
	return MAIN_ERR_STD; \
} \
/*Thread created*/ \
}



void *absint_ctrl_cli (void* arg);

#define ABSINT_CTRL_THREAD_START_CLI { \
	struct AICtrlManageThreadArgPassing *cliArgPass; \
	 \
	cliArgPass = malloc(sizeof(struct AICtrlManageThreadArgPassing)); \
	cliArgPass->sem_all=sem_aictrl; \
	cliArgPass->manmsg=&InterThreadManagerMsg; \
	 \
	pthread_attr_t tattr; \
	if((err=pthread_attr_init(&tattr)) < 0) { \
		perror("could not create thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
	if((err=pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)) < 0){ \
		perror("could not modify thread-attribute"); \
		if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_create(&threads_aictrl[ABSINT_CTRL_CLI_THREAD], &tattr, absint_ctrl_cli, (void*)cliArgPass)) < 0) { \
		perror("could not create thread"); \
		if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
		perror("could not destroy thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
}






#define PRINT_AICTRL_MSGTYPE(type) switch(type){ \
	case AIC_AITC_IFACE: \
	printf("AIC_AITC_IFACE"); \
		break; \
	case AIC_CTAI_START_ADHOC: \
	printf("AIC_CTAI_START_ADHOC"); \
		break; \
	case AIC_CTAI_STOP_ADHOC: \
	printf("AIC_CTAI_STOP_ADHOC"); \
		break; \
	case AIC_CTAI_SET_ADHOC_ESSID: \
	printf("AIC_CTAI_SET_ADHOC_ESSID"); \
		break; \
	case AIC_CTAI_SET_ADHOC_FREQ: \
	printf("AIC_CTAI_SET_ADHOC_FREQ"); \
		break; \
	case AIC_CTAI_SET_ADHOC_CHAN: \
	printf("AIC_CTAI_SET_ADHOC_CHAN"); \
		break; \
	case AIC_CTAI_SET_ADHOC_ESSID_FREQ: \
	printf("AIC_CTAI_SET_ADHOC_ESSID_FREQ"); \
		break; \
	case AIC_INTERN_PRINT_CONNECTIONS: \
	printf("AIC_INTERN_PRINT_CONNECTIONS"); \
		break; \
	case AIC_AITC_WLANSTAT_RSSI: \
	printf("AIC_AITC_WLANSTAT_RSSI"); \
		break; \
	case AIC_AITC_WLANSTAT_COMPLETE: \
	printf("AIC_AITC_WLANSTAT_COMPLETE"); \
		break; \
	case AIC_AITC_BITRATE: \
	printf("AIC_AITC_BITRATE"); \
		break; \
	case AIC_CTAI_SET_ADHOC_SECUR_PW: \
	printf("AIC_CTAI_SET_ADHOC_SECUR_PW"); \
		break; \
	case AIC_CTAI_SET_IFACE_IP4: \
	printf("AIC_CTAI_SET_IFACE_IP4"); \
		break; \
	case AIC_CTAI_INQUIRE_IFACE: \
	printf("AIC_CTAI_INQUIRE_IFACE"); \
		break; \
	case AIC_CTAI_INQUIRE_RSSI: \
	printf("AIC_CTAI_INQUIRE_RSSI"); \
		break; \
	case AIC_CTAI_MONITOR_RSSI: \
	printf("AIC_CTAI_MONITOR_RSSI"); \
		break; \
	case AIC_INTERN_AI_DISCONNECT: \
	printf("AIC_INTERN_AI_DISCONNECT"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_CTAI_START: \
	printf("AIC_WLAN_SWITCH_FSM_CTAI_START"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_CTAI_SWITCH_DONE: \
	printf("AIC_WLAN_SWITCH_FSM_CTAI_SWITCH_DONE"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_CTAI_FINISH: \
	printf("AIC_WLAN_SWITCH_FSM_CTAI_FINISH"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_CTAI_STAY_READY: \
	printf("AIC_WLAN_SWITCH_FSM_CTAI_STAY_READY"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_CTAI_STAY_DONE: \
	printf("AIC_WLAN_SWITCH_FSM_CTAI_STAY_DONE"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_AITC_READY_TO_SWITCH: \
	printf("AIC_WLAN_SWITCH_FSM_AITC_READY_TO_SWITCH"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_AITC_SWITCH_DONE_ACK: \
	printf("AIC_WLAN_SWITCH_FSM_AITC_SWITCH_DONE_ACK"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_AITC_READY_TIMEOUT: \
	printf("AIC_WLAN_SWITCH_FSM_AITC_READY_TIMEOUT"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_AITC_DONE_TIMEOUT: \
	printf("AIC_WLAN_SWITCH_FSM_AITC_DONE_TIMEOUT"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_AITC_STAY_READY_ACK: \
	printf("AIC_WLAN_SWITCH_FSM_AITC_STAY_READY_ACK"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_AITC_STAY_DONE_ACK: \
	printf("AIC_WLAN_SWITCH_FSM_AITC_STAY_DONE_ACK"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_R: \
	printf("AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_R"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_D: \
	printf("AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_D"); \
		break; \
	case AIC_WLAN_SWITCH_FSM_AITC_FINISH_ACK: \
	printf("AIC_WLAN_SWITCH_FSM_AITC_FINISH_ACK"); \
		break; \
	default: \
		break; \
	}










#endif /* ABSINT_CTRL_H */
