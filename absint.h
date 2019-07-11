#ifndef ABSINT_H
#define ABSINT_H

/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */

#include <sys/stat.h>
#include <resolv.h>
#include <arpa/inet.h>

#include "ollerus_globalsettings.h"
//#include "ollerus.h"
#include "debug.h"
#include "nl80211.h"
#include "remainder.h"

#include "absint_netf.h"
#include "absint_ctrl_msgs.h"
#include "absint_ctrl.h"
#include "wlan_sniff.h"

//#include "head/ollerus_extern_functions.h" //Stands inside every src at it's own





#define ABSINT_CFG_FILE_NAME "absint"
#define ABSINT_CFG_FILE_PATH_PREFIX ""
#define ABSINT_CFG_FILE_PATH_SUFFIX ".cfg"
#define ABSINT_ADHOC_CFG_FILE_NAME "absint-wpa_supplicant-adhoc_"
#define ABSINT_ADHOC_CFG_FILE_PATH_PREFIX "wlan/"
#define ABSINT_ADHOC_CFG_FILE_PATH_SUFFIX ".conf"
#define ABSINT_AI_CTRL_STD_PORT 24292
#define ABSINT_SDN_CTRL_STD_PORT 8080
#define ABSINT_AI_PARTNER_STD_PORT 24291
#define RECVBUFFSIZECONTROLLER 1024
#define RECVBUFFSIZEOTHERAI 1024
#define INTER_AI_MSG_MAX_SIZE 1024

#define ABSINT_SWITCH_MODE_TIMEOUT_S 10
#define ABSINT_SWITCH_MODE_TIMEOUT_NS 0



#define WLAN_TRAFFICSTAT_CHAN_DIST ((int)4)//The distance of the used channels
#define WLAN_TRAFFICSTAT_CHAN_NUM ((((int)13)/((int)WLAN_TRAFFICSTAT_CHAN_DIST))+((int)1))


#define SIGKILL_WLANMON SIGUSR1



enum aiconnectclientserver {
	INTER_AI_CLSERV_AUTO,
	INTER_AI_CLSERV_SERVER,
	INTER_AI_CLSERV_CLIENT,
	INTER_AI_CLSERV_CFG
};
enum aipartneraddrsrc {
	INTER_AI_ADDR_SRC_AUTO,
	INTER_AI_ADDR_SRC_CFG,
	INTER_AI_ADDR_SRC_CMDLINE
};

struct aiconnectmode {
	enum aiconnectclientserver clientOrServer;
	enum aipartneraddrsrc AIPartnerAddrSrc;
};

struct absintComThreadArgPassing {
	int destport;
	struct in_addr destIP4;
	struct ComThrShMem *shmem;//ShMem Pointer
	sem_t *sem_mainsynch;//Sem1 Pointer
	sem_t *sem_send;
	pthread_t *threadindex;//Pointer to own ThreadIndex
	struct AbsintInterfaces *ifcollect;
	struct AbsintInterfacesWLAN *wifcollect;
};

struct absintListenThreadArgPassing {
	struct ComThrShMem *shmem;//ShMem Pointer
	sem_t *sem_mainsynch;//Sem1 Pointer
	sem_t *sem_send;
	pthread_t *threadindex;//Pointer to own ThreadIndex
	struct AbsintInterfaces *ifcollect;
	int socket;
	unsigned int *bytes_recvd;
};

struct absintWLANMonThreadArgPassing {
	struct ComThrShMem *shmem;//ShMem Pointer
	sem_t *sem_mainsynch;//Sem1 Pointer
	sem_t *sem_send;
	pthread_t *threadindex;//Pointer to own ThreadIndex
	char dev[IFNAMSIZ];
	double WLANChanTrafficMonTime;
	char ChannelDistance;
};

struct absintStationMonThreadArgPassing {
	struct ComThrShMem *shmem;//ShMem Pointer
	sem_t *sem_mainsynch;//Sem1 Pointer
	sem_t *sem_send;
	pthread_t *threadindex;//Pointer to own ThreadIndex
	struct AbsintInterfaces *ifcollect;
};

struct absintTimeoutThreadArgPassing {
	struct ComThrShMem *shmem;//ShMem Pointer
	sem_t *sem_mainsynch;//Sem1 Pointer
//	sem_t *sem_send;
//	pthread_t *threadindex;//Pointer to own ThreadIndex
	struct timespec timeoutduration;
	int timeoutID;//ID
	enum AICtrlMsgType timeoutType;
};

struct absintSendThreadArgPassing {
	int *socket;//Pointer to established Socket
	pthread_t *threadindex;//Pointer to own ThreadIndex
	struct AbsintInterfaces *ifcollect;
};

struct DNeigh{
	char dev[IFNAMSIZ];//The Port over which the Neighbour can be reached
	struct in_addr IP4;
	char mac[6];//Only store the "numbers" of the Hardware-Address. Look Comment after struct
	__u16 state;
};
/* MAC / Hardware-Address:
 * One Binary Byte is 8 Bit. One Hexadecimal Sign needs exactly 4 Bit. This means one "char" gives us exactly
 * two Hexadecimal Digits. A MAC Address has six Groups of Two-Digit-Hexadecimals. Here we store these
 * Values in numerical Form, without the intermediate ":"
 */
struct DiscoveredNeighbours{
	int neighc;//The Quantity of all found neighbours
	struct DNeigh **dnstart;//Like common: Points to an Area, where Pointers are concatenated. These Pointers point to the DiscoveredNeighbour structs. One for each neighbour.
};
struct AbsintInterfaces {
	unsigned int ifc;//The Number of all available Interfaces
	char **ifacestart;
	char **ifmacstart;
	char **ifaddrstart;
	struct DiscoveredNeighbours neighbours;
};
struct AbsintInterfacesWLAN {//This stores the Indices of the Devices, found in a "struct AbsintInterfaces", which are WLAN
	unsigned int wlanc;//Number of WLAN-Interfaces
	unsigned int *wlanidx;//For Indices will be a malloc done. Readout like an array
};

#define AI_WLAN_SWITCH_DEV_NAMEP_OLD ai_wlan_dev_namep[0]
#define AI_WLAN_SWITCH_DEV_NAMEP_NEW ai_wlan_dev_namep[1]
#define AI_WLAN_SWITCH_DEV_IDX_OLD ai_wlan_dev_idx[0]
#define AI_WLAN_SWITCH_DEV_IDX_NEW ai_wlan_dev_idx[1]
#define AI_WLAN_SWITCH_DEV_EXCHANGE { \
	char *triangleswap; \
	triangleswap = AI_WLAN_SWITCH_DEV_NAMEP_OLD; \
	AI_WLAN_SWITCH_DEV_NAMEP_OLD = AI_WLAN_SWITCH_DEV_NAMEP_NEW; \
	AI_WLAN_SWITCH_DEV_NAMEP_NEW = triangleswap; \
	AI_WLAN_SWITCH_DEV_IDX_NEW=if_nametoindex(AI_WLAN_SWITCH_DEV_NAMEP_NEW); \
	AI_WLAN_SWITCH_DEV_IDX_OLD=if_nametoindex(AI_WLAN_SWITCH_DEV_NAMEP_OLD); \
}

struct ComThrShMem {//The Shared Memory for the Message Passing between the Communication Threads
	char *ShMem;
	sem_t sem_shmem;//Sem2
	int msgsize;
	enum AICtrlMsgType type;
	char flags;
};
//The Flags:
#define FLAG_ABSINT_COMTHR_SHMEM_NEW 0x01//If Set: Message is new and Main hasn't read out yet
#define FLAG_ABSINT_COMTHR_SHMEM_CHAN_SWITCH_MODE 0x02//If Set: We are in Channel Switching Mode. Look below
//#define FLAG_ABSINT_COMTHR_SHMEM_ 0x04
#define FLAG_ABSINT_COMTHR_SHMEM_SEND 0x08//If Set: Its a Message to be send (Then the two SRC_AI | SRC_CTRL Flags are used to signal which is the destination)
//#define FLAG_ABSINT_COMTHR_SHMEM_ 0x10
//#define FLAG_ABSINT_COMTHR_SHMEM_ 0x20
#define FLAG_ABSINT_COMTHR_SHMEM_SRC_AI 0x40//IF set: The message comes from another AI
#define FLAG_ABSINT_COMTHR_SHMEM_SRC_CTRL 0x80//If set: The message comes from the SDN Controller




struct SwitchFSM {
	char state;//Obiously, the current state of the FSM of the current Channel-Switch-Mode
	uint32_t instanceid;//The ID of the current Channel-Switch-Mode. Starts at Controller with '1'
};
/* The State Encoding of the Channel Switch FSM
*/
#define CHANSWITCH_FSM_STATE_NONE 0//Not in Channel Switch Mode
#define CHANSWITCH_FSM_STATE_ENTER 1//The Message to trigger the Mode just came from the Controller. We entered the Channel Switch Mode
#define CHANSWITCH_FSM_STATE_READY_TO_SWITCH 2//AI sent out, that it is ready. Now it waits for the "Done" Msg from the Ctrl
#define CHANSWITCH_FSM_STATE_SWITCH_DONE 3//The Switch (for me) should be done. Waiting to ensure, that my Partner got it to.
#define CHANSWITCH_FSM_STATE_READY_TIMEOUT 4//Timeout First Stage. Now second Timeout let me Fallback or Ctrl holds me ready
#define CHANSWITCH_FSM_STATE_DONE_TIMEOUT 5//Timeout First Stage. Now second Timeout let me Fallback or Ctrl holds me ready
#define CHANSWITCH_FSM_STATE_FINISH 6//Alrighty. Everything done. Now leave the Mode. 

#define CHANSWITCH_FSM_STATE_DONE_AND_LEAVE 99//Hm, would tell something like: Everything done, we are leaving the Mode. Not really needed, don't have to use it





/* You could use the AI Partner Address - in case of Working as Server from the Inter AI Communication Point of View -
 * to restrict incoming Connection Requests
 * See the Wildcard Comment on the Server Setup Macro
 */
/* IMPORTANT: First expand the Macro "CREATE_PROGRAM_PATH(*args)", before using this Macro
 * Because this here needs the Program Path
 */
/*
 * umask(0); is nearly the opposite of umask(ACCESSPERMS);
 * Used here like this, because umask specifies which permission flags get cleared at file/folder creation
 * umask(mask) specifies the mask which used at creation with something like mkdir("dir",creatmask)
 * 		in form of (creatmask & ~mask)
 */
#define ABSINT_READ_CONFIG_FILE printf("\tReading in the config-file...\n"); \
		umask(0); \
    	int aipc=1; \
    	int aiport; \
    	struct in_addr dnsserverip4; \
    	/*uintptr_t sdnctrldns;*/ \
    	char *sdnctrldns; \
    	sdnctrldns=NULL; \
    	int aictrlport; \
    	int sdnctrlport; \
    	struct in_addr controlleraddress; \
    	struct timespec WLANChanTrafficMonTime; \
    	char WLANChanTrafficMonChanDistance; \
        /* Maybe Open a Scope for Variables, which won't be needed forever */ \
		/* Remember to close the Scope somewhere after the Macro-Usage */ \
		/* If there aren't any Variables like this just close it immediately after the Macro Usage */ \
		/* or delete the Line with the opening Bracket ( { \ ) */ \
    	struct aiconnectmode aiconnectmodecfg={ \
			.clientOrServer=INTER_AI_CLSERV_AUTO, \
			.AIPartnerAddrSrc=INTER_AI_ADDR_SRC_AUTO \
		}; \
    	struct in_addr aipaddresscfg; \
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
	    			fprintf(cfgf, "\n*INFO: Setup should be as following:"); \
	    			fprintf(cfgf, "\n*INFO: AI-Ctrl is autonomous Program, that runs on the same Machine as the SDN-Ctrl"); \
	    			fprintf(cfgf, "\n*INFO: The AI-Ctrl communicates on this Machine with the SDN-Ctrl"); \
	    			fprintf(cfgf, "\n*INFO: Every AI communicates over the Network with the AI-Ctrl"); \
	    			fprintf(cfgf, "\n*INFO: So the AI-Ctrl Port is used between every AI and the AI-Ctrl"); \
	    			fprintf(cfgf, "\n*INFO: And the SDN-Ctrl Port is used only from the AI-Ctrl; between the AI-Ctrl and the SDN-Ctrl"); \
					printf("\t\t\t#WLANChanTrafficMonS=0\n"); \
					fprintf(cfgf, "\n#WLANChanTrafficMonS=0"); \
					printf("\t\t\t#WLANChanTrafficMonNS=500000000\n"); \
					fprintf(cfgf, "\n#WLANChanTrafficMonNS=500000000"); \
					WLANChanTrafficMonTime.tv_sec=0; \
					WLANChanTrafficMonTime.tv_nsec=500000000; \
					printf("\t\t\t#WLANChanTrafficMonChanDist=5\n"); \
					fprintf(cfgf, "\n#WLANChanTrafficMonChanDist=5"); \
					WLANChanTrafficMonChanDistance=5; \
					printf("\t\t\t#AIPartnerCount=1\n"); \
					fprintf(cfgf, "\n#AIPartnerCount=1"); \
					printf("\t\t\t#AIPartnerAddrSrc=auto\n"); \
					fprintf(cfgf, "\n#AIPartnerAddrSrc=auto"); \
					printf("\t\t\t#AIPartnerAddrIP4=10.0.1.20\n"); \
					fprintf(cfgf, "\n#AIPartnerAddrIP4=10.0.1.20"); \
					inet_pton(AF_INET,"10.0.1.20",&aipaddresscfg); \
					printf("\t\t\t#AIPartnerPort=%d\n",ABSINT_AI_PARTNER_STD_PORT); \
					fprintf(cfgf, "\n#AIPartnerPort=%d",ABSINT_AI_PARTNER_STD_PORT); \
					aiport=ABSINT_SDN_CTRL_STD_PORT; \
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
				if(strcmp(buf,"AIConnectMode")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					if(strcmp(buf,"client")==0){ \
						aiconnectmodecfg.clientOrServer=INTER_AI_CLSERV_CLIENT; \
					} else \
					if(strcmp(buf,"server")==0){ \
						aiconnectmodecfg.clientOrServer=INTER_AI_CLSERV_SERVER; \
					} else \
					if(strcmp(buf,"auto")==0){ \
						aiconnectmodecfg.clientOrServer=INTER_AI_CLSERV_AUTO; \
					} else { \
						printf("\tERROR: cfg-file damaged! Invalid Entry at AIConnectMode!\n"); \
					} \
				} else if(strcmp(buf,"SDNControllerAddrIP4")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					inet_pton(AF_INET,buf,&controlleraddress); \
				} else if(strcmp(buf,"AIPartnerCount")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					char *endpoint; \
					aipc = strtol(buf, &endpoint, 10); \
					if (*endpoint) { \
						printf("ERROR: Unsupported Format in cfg-File at:\n\tAIPartnerCount\n"); \
						return MAIN_ERR_BAD_CMDLINE; \
					} \
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
				} else if(strcmp(buf,"WLANChanTrafficMonS")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					char *endpoint; \
					WLANChanTrafficMonTime.tv_sec = strtol(buf, &endpoint, 10); \
					if (*endpoint) { \
						printf("ERROR: Unsupported Format in cfg-File at:\n\tWLANChanTrafficMonS\n"); \
						return MAIN_ERR_BAD_CMDLINE; \
					} \
				} else if(strcmp(buf,"WLANChanTrafficMonNS")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					char *endpoint; \
					WLANChanTrafficMonTime.tv_nsec = strtol(buf, &endpoint, 10); \
					if (*endpoint) { \
						printf("ERROR: Unsupported Format in cfg-File at:\n\tWLANChanTrafficMonNS\n"); \
						return MAIN_ERR_BAD_CMDLINE; \
					} \
				} else if(strcmp(buf,"WLANChanTrafficMonChanDist")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					char *endpoint; \
					WLANChanTrafficMonChanDistance = strtol(buf, &endpoint, 10); \
					if (*endpoint) { \
						printf("ERROR: Unsupported Format in cfg-File at:\n\tWLANChanTrafficMonS\n"); \
						return MAIN_ERR_BAD_CMDLINE; \
					} \
				} else if(strcmp(buf,"AIPartnerPort")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					if(strcmp(buf,"auto")==0){ \
					}else{ \
						char *endpoint; \
						aiport = strtol(buf, &endpoint, 10); \
						if (*endpoint) { \
							printf("ERROR: Unsupported Format in cfg-File at:\n\tAIPartnerCount\n"); \
							return MAIN_ERR_BAD_CMDLINE; \
						} \
					} \
				} else if(strcmp(buf,"AIPartnerAddrSrc")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					if(strcmp(buf,"auto")==0){ \
						aiconnectmodecfg.AIPartnerAddrSrc=INTER_AI_ADDR_SRC_AUTO; \
					}else if(strcmp(buf,"cfg")==0){ \
						aiconnectmodecfg.AIPartnerAddrSrc=INTER_AI_ADDR_SRC_CFG; \
					}else if(strcmp(buf,"cmdline")==0){ \
						aiconnectmodecfg.AIPartnerAddrSrc=INTER_AI_ADDR_SRC_CMDLINE; \
					}else{ \
						printf("ERROR: Unsupported Format in cfg-File at:\n\tAIPartnerAddrSrc\n"); \
						return MAIN_ERR_BAD_CMDLINE; \
					} \
				} else if(strcmp(buf,"AIPartnerAddrIP4")==0){ \
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){ \
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) { \
							buf[loopcnt]='\0'; \
							break; \
						} \
					} \
					inet_pton(AF_INET,buf,&aipaddresscfg); \
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
	    	printf("\t...Read in Connection Mode to other AIs: %s\n", (aiconnectmodecfg.clientOrServer==INTER_AI_CLSERV_CLIENT)?"client":((aiconnectmodecfg.clientOrServer==INTER_AI_CLSERV_SERVER)?"server":((aiconnectmodecfg.clientOrServer==INTER_AI_CLSERV_AUTO)?"auto":"Ehm, something gone wrong... so use: AUTO"))); \
	    	printf("\t\tRemember: Commandline has higher Priority (if given)\n"); \
			char ControllerIP4[16]; \
			inet_ntop(AF_INET,&controlleraddress,ControllerIP4,sizeof(ControllerIP4)); \
			printf("\t...Read Address (IP4) of SDN Controller: %s\n",ControllerIP4); \
	    	printf("\t...Read Number of connected other AIs: %d\n",aipc); \
	        if (fclose(cfgf)) { \
	        	printf("NOTICE: Config-File couldn't be closed successfully!"); \
	        } else { \
	        	printf("...config-file read successfully!\n"); \
	        } \
	    } \
		/* Close the Scope for the Config-related Variables */ \
		}






/*
 * To make the process of applying changes to the conf file easier:
 * Append Whitespaces to the lines of ssid and frequency that are enough to catch coming signs (SSIDs have a max number of 32 characters)
 * so we do not have to handle complicated mmap functions for file changes like copy, new file creation and such stuff
 * Nowadays 4 characters for the frequency should be enough (suffices for 2.4GHz and 5GHz) For HF Communication we should think
 * about spending more white spaces or do this bad mmap file expanding stuff.
 */
/* IMPORTANT: First expand the Macro "CREATE_PROGRAM_PATH", before using this Macro
 * Because this here needs the Program Path
 */
#define WPA_SUPPLICANT_CONF_SETUP(DEV) printf("   Checking the wpa_supplicant.conf... for Device \"%s\"\n\t...This is used for the Ad-hoc Setup\n\t...and the Detection Stuff\n",DEV); \
	umask(0); \
	char wpasup_file[strlen(ProgPath)+strlen(ABSINT_ADHOC_CFG_FILE_PATH_PREFIX)+strlen(ABSINT_ADHOC_CFG_FILE_NAME)+strlen(DEV)+strlen(ABSINT_ADHOC_CFG_FILE_PATH_SUFFIX)+1]; \
	memcpy(wpasup_file,ProgPath,strlen(ProgPath)); \
	memcpy(wpasup_file+strlen(ProgPath),ABSINT_ADHOC_CFG_FILE_PATH_PREFIX,strlen(ABSINT_ADHOC_CFG_FILE_PATH_PREFIX)); \
	memcpy(wpasup_file+strlen(ProgPath)+strlen(ABSINT_ADHOC_CFG_FILE_PATH_PREFIX),ABSINT_ADHOC_CFG_FILE_NAME,strlen(ABSINT_ADHOC_CFG_FILE_NAME)); \
	memcpy(wpasup_file+strlen(ProgPath)+strlen(ABSINT_ADHOC_CFG_FILE_PATH_PREFIX)+strlen(ABSINT_ADHOC_CFG_FILE_NAME),DEV,strlen(DEV)); \
	memcpy(wpasup_file+strlen(ProgPath)+strlen(ABSINT_ADHOC_CFG_FILE_PATH_PREFIX)+strlen(ABSINT_ADHOC_CFG_FILE_NAME)+strlen(DEV),ABSINT_ADHOC_CFG_FILE_PATH_SUFFIX,strlen(ABSINT_ADHOC_CFG_FILE_PATH_SUFFIX)); \
	wpasup_file[sizeof(wpasup_file)-1]='\0'; \
    /* Open a Scope for the Config-related Variables (Read-Buffer, File Pointer...) */ \
    /* Therewith they get deallocated after the File-Readout */ \
	{ \
	FILE *wpasupf; \
	wpasupf = fopen(wpasup_file, "r"); \
	if (!wpasupf) { \
		char wpasup_dir[strlen(ProgPath)+strlen(ABSINT_ADHOC_CFG_FILE_PATH_PREFIX)+1]; \
		switch(errno) { \
			case EACCES: \
				printf("\tERROR: Couldn't open wpa_supplicant.conf-File!\n\t\tReason:Permission denied!\n"); \
			break; \
			case ENOENT: \
				printf("\tWARNING: wpa_supplicant.conf-File doesn't exist!\n"); \
				printf("\tCreating new Default one...\n"); \
				/*NOT Exclude the preceding '/' from the folder-path*/ \
				memcpy(wpasup_dir,ProgPath,strlen(ProgPath)); \
				memcpy(wpasup_dir+strlen(ProgPath),ABSINT_ADHOC_CFG_FILE_PATH_PREFIX,strlen(ABSINT_ADHOC_CFG_FILE_PATH_PREFIX)); \
				wpasup_dir[sizeof(wpasup_dir)-1]='\0'; \
				/*Check the complete Path and eventually create the folders*/ \
				CREATE_COMPLETE_FOLDER_PATH(wpasup_dir); \
				wpasupf = fopen(wpasup_file, "w+"); \
				if(!wpasupf){ \
					printf("\t\tERROR: Couldn't create new wpa_supplicant.conf-file!\n"); \
					return FILE_ERR_PERMISSION_DENIED; \
				} \
				fprintf(wpasupf, "ctrl_interface=DIR=/run/wpa_supplicant GROUP=0\n"); \
				fprintf(wpasupf, "update_config=1\n"); \
				fprintf(wpasupf, "\n"); \
				fprintf(wpasupf, "# use 'ap_scan=2' on all devices connected to the network\n"); \
				fprintf(wpasupf, "ap_scan=2\n"); \
				fprintf(wpasupf, "\n"); \
				fprintf(wpasupf, "network={\n"); \
				fprintf(wpasupf, "\tssid=\"AbsintAdhocDefault\"              \n"); \
				fprintf(wpasupf, "\tmode=1\n"); \
				fprintf(wpasupf, "\tfrequency=2412\n"); \
				fprintf(wpasupf, "\tproto=RSN\n"); \
				fprintf(wpasupf, "\tkey_mgmt=WPA-PSK\n"); \
				fprintf(wpasupf, "\tpairwise=CCMP\n"); \
				fprintf(wpasupf, "\tgroup=TKIP\n"); \
				fprintf(wpasupf, "\tpsk=\"meinschluessel\"\n"); \
				fprintf(wpasupf, "}"); \
				fclose(wpasupf); \
				printf("\t\t...created wpa_supplicant.conf-file successfully!\n"); \
			break; \
			default: \
				fprintf(stderr, "Ups, seems like we've encountered a case, which isn't caught yet :o("); \
				return MAIN_ERR_FUNC_INCOMPLETE; \
			break; \
		} \
	} else{ \
		printf("\tI found one file. Hopefully it's a working one (I'm to lazy to check...)\n"); \
	} \
	\
	}









/* Do the memory space Allocation of the Shared Memory dynamically
 * malloc() a fitting size for every msg and store the necessary informations
 * in the struct.
 * Then free() the space after read-out by main
 * It's a Trade-off:
 * Dynamic needs slight more computation time for every message exchange, but occupies less memory
 * I expect not that much messages to exchange, so better don't use much memory and particularly don't occupy
 * it the whole time, but spent the computation time for allocation.
 */
#define ABSINT_INTER_THREAD_COM_SHMEM_AND_SYNCH_SETUP \
	struct ComThrShMem shmem = { \
		.ShMem = NULL, \
		.msgsize = 0, \
		.flags = 0x00 \
	}; \
	sem_init(&(shmem.sem_shmem),0,1); \
	sem_t sem_mainsynch; \
	sem_init(&sem_mainsynch,0,0); \
	sem_t sem_send; \
	sem_init(&sem_send,0,0); \
	
//	pthread_t thread_wlan_mon;//For this look at the global struct for the WLAN Monitor Signal Kill




#define ABSINT_INET_SOCKET_TO_AI_PARTNER_VARIABLE_RESERVATION \




#define ABSINT_INET_SOCKET_SERVER_SETUP \
	int s; \
	struct hostent *hp; \
	struct servent *sp; \
	long timevar; \
	char *ctime(); \
	 \
	time_t systime,systimeold; \
	 \
	struct sockaddr_in myaddr_in; \
	struct sockaddr_in peeraddr_in; \
	 \
	int addrlen; \
	unsigned int bytes_recvd; \
	char msg_recvd[RECVBUFFSIZEOTHERAI]; \
	 \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Starting Server for Inter-AI-Communication.\n"); \
	 \
	int ls; \
	 \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Getting ready to listen for incoming connections...\n"); \
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in)); \
	memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in)); \
	myaddr_in.sin_family = AF_INET; \
	memcpy(&(peeraddr_in.sin_addr),&listenIP4,sizeof(struct in_addr)); \
	myaddr_in.sin_addr.s_addr = INADDR_ANY; \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("\t-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" ...Listening to address (wildcard?): %d\n", myaddr_in.sin_addr.s_addr); \
	 \
	myaddr_in.sin_port = aiport; \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("\t-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" ...Port number: %d\n", myaddr_in.sin_port); \
	 \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Creating listening socket...\n"); \
	ls = socket (AF_INET, SOCK_STREAM, 0); \
	if (ls == -1) { \
		ANSICOLORSET(ANSI_COLOR_GREEN); \
		fprintf(stderr,"-->AI-Com-Thread:"); \
		ANSICOLORRESET; \
		fprintf(stderr, " unable to create socket\n"); \
		exit(1); \
	} \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("\t-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" ...Socket created!\n"); \
	if (bind(ls, &myaddr_in, sizeof(struct sockaddr_in)) == -1) { \
		ANSICOLORSET(ANSI_COLOR_GREEN); \
		fprintf(stderr,"-->AI-Com-Thread:"); \
		ANSICOLORRESET; \
		fprintf(stderr, " unable to bind address\n"); \
		exit(1); \
	} \
	if (listen(ls, 5) == -1) { \
		ANSICOLORSET(ANSI_COLOR_GREEN); \
		fprintf(stderr,"-->AI-Com-Thread:"); \
		ANSICOLORRESET; \
		fprintf(stderr, " unable to listen on socket\n"); \
		exit(1); \
	} \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" ...Initiation complete!\n"); \
	 \
	addrlen = sizeof(struct sockaddr_in); \
	 \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("\n-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Waiting for incoming Connections...\n"); \
	\
	AcceptNewConnection: \
	s = accept(ls, (struct sockaddr *)&peeraddr_in, (socklen_t*)&addrlen); \
	if (s<0) { \
		perror("accept failed"); \
		return 1; \
	} \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("\n-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Incoming Connection accepted\n");



#define ABSINT_INET_SOCKET_SERVER_START_THREAD \
	pthread_t thread_ai_com_server; \
	struct absintComThreadArgPassing *aiservercomthreadArgPass; \
	aiservercomthreadArgPass = malloc(sizeof(struct absintComThreadArgPassing)); \
	aiservercomthreadArgPass->destport = aiport; \
	/*Use this Address as listen Address. Fill in the Wildcard or use it to restrict the socket accept to specific AI Partner*/ \
	memset(&(aiservercomthreadArgPass->destIP4),0,sizeof(struct in_addr)); \
	(aiservercomthreadArgPass->destIP4).s_addr=INADDR_ANY; \
	aiservercomthreadArgPass->shmem=&shmem; \
	aiservercomthreadArgPass->sem_mainsynch=&sem_mainsynch;\
	aiservercomthreadArgPass->threadindex = &thread_ai_com_server; \
	aiservercomthreadArgPass->ifcollect=&ifcollect; \
	pthread_attr_t tattr_ai_server; \
	/* initialized with default attributes*/ \
	if((err=pthread_attr_init(&tattr_ai_server)) < 0) { \
		perror("could not create thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
	if((err=pthread_attr_setdetachstate(&tattr_ai_server,PTHREAD_CREATE_DETACHED)) < 0){ \
		perror("could not modify thread-attribute"); \
		if( (err=pthread_attr_destroy(&tattr_ai_server)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_create(&thread_ai_com_server, &tattr_ai_server, absint_com_AI_server, (void*)aiservercomthreadArgPass)) < 0) { \
		perror("could not create thread"); \
		if( (err=pthread_attr_destroy(&tattr_ai_server)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_attr_destroy(&tattr_ai_server)) != 0 ) { \
		perror("could not destroy thread-attribute"); \
		return MAIN_ERR_STD; \
	}




#define ABSINT_INET_SOCKET_CLIENT_CONNECT \
	int s; \
	struct hostent *hp; \
	struct servent *sp; \
	long timevar; \
	char *ctime(); \
	 \
	time_t systime,systimeold; \
	 \
	struct sockaddr_in myaddr_in; \
	struct sockaddr_in peeraddr_in; \
	 \
	int addrlen; \
	unsigned int bytes_recvd; \
	char msg_recvd[RECVBUFFSIZEOTHERAI]; \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Setting up Connection to SDN Controller...\n"); \
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in)); \
	memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in)); \
	peeraddr_in.sin_family = AF_INET; \
	memcpy(&(peeraddr_in.sin_addr),&aiserverIP4,sizeof(struct in_addr)); \
	char printaddrv4[INET_ADDRSTRLEN]; \
	inet_ntop(AF_INET, &(peeraddr_in.sin_addr), printaddrv4, INET_ADDRSTRLEN); \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("\t-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" ...Connecting to IP Address (IPv4): %s\n", printaddrv4); \
	peeraddr_in.sin_port = aiport; \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("\t-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" ...Port number: %d\n", peeraddr_in.sin_port); \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Creating Socket...\n"); \
	s = socket (AF_INET, SOCK_STREAM, 0); \
	if (s == -1) { \
		ANSICOLORSET(ANSI_COLOR_GREEN); \
		fprintf(stderr,"-->AI-Com-Thread:"); \
		ANSICOLORRESET; \
		fprintf(stderr," unable to create socket\n"); \
		exit(1); \
	} \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("\t-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" ...Socket created!\n"); \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("\t-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Now connecting...\n"); \
	if (connect(s, &peeraddr_in, addrlen=sizeof(struct sockaddr_in)) ==-1) { \
		ANSICOLORSET(ANSI_COLOR_GREEN); \
		printf("\n-->AI-Com-Thread:"); \
		ANSICOLORRESET; \
		printf(" ERROR: Couldn't connect to specified Peer Address. | ERRNO: %d\n",errno); \
		return NETWORK_ERR_NO_CONNECTION; \
	} \
	 \
	if (getsockname(s, &myaddr_in, &addrlen) == -1) { \
		ANSICOLORSET(ANSI_COLOR_GREEN); \
		fprintf(stderr,"\t-->AI-Com-Thread:"); \
		ANSICOLORRESET; \
		fprintf(stderr, " unable to read socket address\n"); \
		exit(1); \
	} \
	time(&timevar); \
	ANSICOLORSET(ANSI_COLOR_GREEN); \
	printf("-->AI-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Connected to %s on port %u at %s\n","TODO",ntohs(myaddr_in.sin_port),ctime(&timevar)); \
	printf("\n");



#define ABSINT_INET_SOCKET_CLIENT_START_THREAD \
	pthread_t thread_ai_com_client; \
	struct absintComThreadArgPassing *aiclientcomthreadArgPass; \
	aiclientcomthreadArgPass = malloc(sizeof(struct absintComThreadArgPassing)); \
	aiclientcomthreadArgPass->destport = aiport; \
	memcpy(&(aiclientcomthreadArgPass->destIP4),&aipaddress,sizeof(struct in_addr)); \
	aiclientcomthreadArgPass->shmem=&shmem; \
	aiclientcomthreadArgPass->sem_mainsynch=&sem_mainsynch;\
	aiclientcomthreadArgPass->threadindex = &thread_ai_com_client; \
	aiclientcomthreadArgPass->ifcollect=&ifcollect; \
	pthread_attr_t tattr_ai_client; \
	/* initialized with default attributes*/ \
	if((err=pthread_attr_init(&tattr_ai_client)) < 0) { \
		perror("could not create thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
	if((err=pthread_attr_setdetachstate(&tattr_ai_client,PTHREAD_CREATE_DETACHED)) < 0){ \
		perror("could not modify thread-attribute"); \
		if( (err=pthread_attr_destroy(&tattr_ai_client)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_create(&thread_ai_com_client, &tattr_ai_client, absint_com_AI_client, (void*)aiclientcomthreadArgPass)) < 0) { \
		perror("could not create thread"); \
		if( (err=pthread_attr_destroy(&tattr_ai_client)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_attr_destroy(&tattr_ai_client)) != 0 ) { \
		perror("could not destroy thread-attribute"); \
		return MAIN_ERR_STD; \
	}



#define ABSINT_INET_SOCKET_START_AI_SENDER_THREAD \
	pthread_t thread_ai_sender; \
	struct absintSendThreadArgPassing *aisendercomthreadArgPass; \
	aisendercomthreadArgPass = malloc(sizeof(struct absintSendThreadArgPassing)); \
	aisendercomthreadArgPass->socket=&s;\
	aisendercomthreadArgPass->threadindex = &thread_ai_sender; \
	aisendercomthreadArgPass->ifcollect=ifcollect; \
	pthread_attr_t tattr_ai_sender; \
	/* initialized with default attributes*/ \
	if((err=pthread_attr_init(&tattr_ai_sender)) < 0) { \
		perror("could not create thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
	if((err=pthread_attr_setdetachstate(&tattr_ai_sender,PTHREAD_CREATE_DETACHED)) < 0){ \
		perror("could not modify thread-attribute"); \
		if( (err=pthread_attr_destroy(&tattr_ai_sender)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_create(&thread_ai_sender, &tattr_ai_sender, absint_com_AI_sender, (void*)aisendercomthreadArgPass)) < 0) { \
		perror("could not create thread"); \
		if( (err=pthread_attr_destroy(&tattr_ai_sender)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_attr_destroy(&tattr_ai_sender)) != 0 ) { \
		perror("could not destroy thread-attribute"); \
		return MAIN_ERR_STD; \
	}




#define ABSINT_INET_SOCKET_CONTROLLER_COM_CONNECT \
	int s; \
	struct hostent *hp; \
	struct servent *sp; \
	long timevar; \
	char *ctime(); \
	 \
	time_t systime,systimeold; \
	 \
	struct sockaddr_in myaddr_in; \
	struct sockaddr_in peeraddr_in; \
	 \
	int addrlen; \
	unsigned int bytes_recvd_ctrl; \
	bytes_recvd_ctrl=1; \
	ANSICOLORSET(ANSI_COLOR_BLUE); \
	printf("-->SDN-Ctrl-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Setting up Connection to SDN Controller...\n"); \
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in)); \
	memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in)); \
	peeraddr_in.sin_family = AF_INET; \
	memcpy(&(peeraddr_in.sin_addr),&sdnctrlIP4,sizeof(struct in_addr)); \
	char printaddrv4[INET_ADDRSTRLEN]; \
	inet_ntop(AF_INET, &(peeraddr_in.sin_addr), printaddrv4, INET_ADDRSTRLEN); \
	ANSICOLORSET(ANSI_COLOR_BLUE); \
	printf("\t-->SDN-Ctrl-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" ...Connecting to IP Address (IPv4): %s\n", printaddrv4); \
	peeraddr_in.sin_port = aictrlport; \
	ANSICOLORSET(ANSI_COLOR_BLUE); \
	printf("\t-->SDN-Ctrl-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" ...Port number: %d\n", peeraddr_in.sin_port); \
	ANSICOLORSET(ANSI_COLOR_BLUE); \
	printf("-->SDN-Ctrl-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Creating Socket...\n"); \
	s = socket (AF_INET, SOCK_STREAM, 0); \
	if (s == -1) { \
		ANSICOLORSET(ANSI_COLOR_BLUE); \
		fprintf(stderr,"-->SDN-Ctrl-Com-Thread:"); \
		ANSICOLORRESET; \
		fprintf(stderr," unable to create socket\n"); \
		exit(1); \
	} \
	ANSICOLORSET(ANSI_COLOR_BLUE); \
	printf("\t-->SDN-Ctrl-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" ...Socket created!\n"); \
	ABSINT_INET_SOCKET_CONTROLLER_COM_CONNECT_AGAIN: \
	ANSICOLORSET(ANSI_COLOR_BLUE); \
	printf("\t-->SDN-Ctrl-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Now connecting...\n"); \
	if (connect(s, &peeraddr_in, addrlen=sizeof(struct sockaddr_in)) ==-1) { \
		ANSICOLORSET(ANSI_COLOR_BLUE); \
		printf("\n-->SDN-Ctrl-Com-Thread:"); \
		ANSICOLORRESET; \
		printf(" ERROR: Couldn't connect to specified Peer Address. | ERRNO: %d\n",errno); \
		ANSICOLORSET(ANSI_COLOR_BLUE); \
		printf("\t-->SDN-Ctrl-Com-Thread:"); \
		ANSICOLORRESET; \
		printf(" Hence waiting a bit and try again.\n"); \
		err=NETWORK_ERR_NO_CONNECTION; \
		struct timespec remainingdelay; \
		remainingdelay.tv_sec = 1; \
		remainingdelay.tv_nsec = 0; \
		do { \
			err = nanosleep(&remainingdelay, &remainingdelay); \
		} while (err<0); \
		goto ABSINT_INET_SOCKET_CONTROLLER_COM_CONNECT_AGAIN; \
	} \
	 \
	if (getsockname(s, &myaddr_in, &addrlen) == -1) { \
		ANSICOLORSET(ANSI_COLOR_BLUE); \
		fprintf(stderr,"\t-->SDN-Ctrl-Com-Thread:"); \
		ANSICOLORRESET; \
		fprintf(stderr, " unable to read socket address\n"); \
		exit(1); \
	} \
	time(&timevar); \
	ANSICOLORSET(ANSI_COLOR_BLUE); \
	printf("-->SDN-Ctrl-Com-Thread:"); \
	ANSICOLORRESET; \
	printf(" Connected to %s on port %u at %s\n","TODO",ntohs(myaddr_in.sin_port),ctime(&timevar)); \
	printf("\n");





#define ABSINT_INET_SOCKET_CONTROLLER_COM_DNS_LOOKUP { \
	if(sdnctrldns!=NULL){ \
		puts(""); \
        printf("Starting DNS Queue for SDN Controller with: %s\n", sdnctrldns); \
        printf("\t...\n"); \
		int dnserr,i; \
		struct addrinfo hints; \
		struct addrinfo *result, *p; \
		struct in_addr falsepositiveip4; \
		 \
		memset(&hints,0,sizeof(hints)); \
		hints.ai_family = AF_INET; \
		/*AF_UNSPEC or AF_INET or AF_INET6*/ \
		hints.ai_socktype = SOCK_STREAM; \
		/*hints.ai_flags = AI_PASSIVE;*/ \
		 \
		memset(&falsepositiveip4,0,sizeof(falsepositiveip4)); \
		 \
		dnserr=1; \
		res_init(); \
		struct sockaddr_in DNSserv; \
		DNSserv.sin_family = AF_INET; \
		DNSserv.sin_port = htons(53); \
		DNSserv.sin_addr = dnsserverip4; \
		/*inet_pton(AF_INET,dnsserverip4,&(DNSserv.sin_addr));*/ \
		for(i=0;i<3;i++){ \
			if((getaddrinfo(sdnctrldns, NULL, &hints, (struct addrinfo**)&result)) == 0) { \
			    for(p = result;p != NULL; p = p->ai_next) { \
			        void *addr; \
			        char ipstr[INET_ADDRSTRLEN]; \
			        if (p->ai_family == AF_INET) { \
			            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr; \
			            addr = &(ipv4->sin_addr); \
			            if(memcmp(addr,&falsepositiveip4,sizeof(struct in_addr))!=0){ \
				            /*Take the first valid Result and leave*/ \
							memcpy(&controlleraddress,addr,sizeof(controlleraddress)); \
					        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr)); \
					        printf("\tDNS Queue for SDN Controller done with: %s\n", sdnctrldns); \
					        printf("\tFirst valid IPv4 Result: %s\n", ipstr); \
					        printf("\t\t %d. Iteration.\n",i); \
							dnserr=0; \
				            goto ResultFound; \
			            } \
			        } \
			    } \
				freeaddrinfo((struct addrinfo *)result); \
			} \
			/*Haven't found any result. Modify _res and try again*/ \
			_res.nscount = 1; \
			_res.nsaddr_list[0] = DNSserv; \
		} \
		ResultFound: \
		 \
		if(dnserr){ \
			printf("DNS Queue to get the IP Address of the SDN Controller wasn't successful!\n"); \
			exit(1); \
		} \
		puts(""); \
	} \
}


#define ABSINT_INET_SOCKET_CONTROLLER_COM_START_THREAD \
	ABSINT_INET_SOCKET_CONTROLLER_COM_DNS_LOOKUP; \
	pthread_t thread_ctrl_com; \
	struct absintComThreadArgPassing *ctrlcomthreadArgPass; \
	ctrlcomthreadArgPass = malloc(sizeof(struct absintComThreadArgPassing)); \
	ctrlcomthreadArgPass->destport = aictrlport; \
	memcpy(&(ctrlcomthreadArgPass->destIP4),&controlleraddress,sizeof(controlleraddress)); \
	ctrlcomthreadArgPass->shmem=&shmem; \
	ctrlcomthreadArgPass->sem_mainsynch=&sem_mainsynch;\
	ctrlcomthreadArgPass->sem_send=&sem_send;\
	ctrlcomthreadArgPass->threadindex = &thread_ctrl_com; \
	ctrlcomthreadArgPass->ifcollect=&ifcollect; \
	ctrlcomthreadArgPass->wifcollect=&wifcollect; \
	pthread_attr_t tattr_ctrl; \
	/* initialized with default attributes*/ \
	if((err=pthread_attr_init(&tattr_ctrl)) < 0) { \
		perror("could not create thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
	if((err=pthread_attr_setdetachstate(&tattr_ctrl,PTHREAD_CREATE_DETACHED)) < 0){ \
		perror("could not modify thread-attribute"); \
		if( (err=pthread_attr_destroy(&tattr_ctrl)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_create(&thread_ctrl_com, &tattr_ctrl, absint_com_controller, (void*)ctrlcomthreadArgPass)) < 0) { \
		perror("could not create thread"); \
		if( (err=pthread_attr_destroy(&tattr_ctrl)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_attr_destroy(&tattr_ctrl)) != 0 ) { \
		perror("could not destroy thread-attribute"); \
		return MAIN_ERR_STD; \
	}

#define ABSINT_INET_SOCKET_CONTROLLER_COM_START_LISTEN_THREAD \
	pthread_t thread_ctrl_com_listen; \
	{ \
	struct absintListenThreadArgPassing *ctrlcomthreadArgPass; \
	ctrlcomthreadArgPass = malloc(sizeof(struct absintListenThreadArgPassing)); \
	ctrlcomthreadArgPass->socket = s; \
	ctrlcomthreadArgPass->bytes_recvd = &bytes_recvd_ctrl; \
	ctrlcomthreadArgPass->shmem=shmem; \
	ctrlcomthreadArgPass->sem_mainsynch=sem_mainsynch;\
	ctrlcomthreadArgPass->sem_send=sem_send;\
	ctrlcomthreadArgPass->threadindex = &thread_ctrl_com_listen; \
	ctrlcomthreadArgPass->ifcollect=ifcollect; \
	pthread_attr_t tattr_ctrl; \
	/* initialized with default attributes*/ \
	if((err=pthread_attr_init(&tattr_ctrl)) < 0) { \
		perror("could not create thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
	if((err=pthread_attr_setdetachstate(&tattr_ctrl,PTHREAD_CREATE_DETACHED)) < 0){ \
		perror("could not modify thread-attribute"); \
		if( (err=pthread_attr_destroy(&tattr_ctrl)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_create(&thread_ctrl_com_listen, &tattr_ctrl, absint_com_controller_listen, (void*)ctrlcomthreadArgPass)) < 0) { \
		perror("could not create thread"); \
		if( (err=pthread_attr_destroy(&tattr_ctrl)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_attr_destroy(&tattr_ctrl)) != 0 ) { \
		perror("could not destroy thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
}




//#define NO_WLAN_MON //Debuging Purpose
/*Gets Pointer to the Device-String. The struct timespec just has to be present*/
#ifdef NO_WLAN_MON
#define ABSINT_THREAD_START_WLAN_MONITOR
#else
#define ABSINT_THREAD_START_WLAN_MONITOR(DEV) { \
	struct absintWLANMonThreadArgPassing *monArgPass; \
	 \
	monArgPass = malloc(sizeof(struct absintWLANMonThreadArgPassing)); \
	monArgPass->shmem=&shmem; \
	monArgPass->sem_mainsynch=&sem_mainsynch; \
	monArgPass->sem_send=&sem_send; \
	/*The pthread_t is not needed here, because it is accessible global*/ \
	/*monArgPass->threadindex=&thread_wlan_mon;*/ \
	strcpy(monArgPass->dev,DEV); \
	monArgPass->WLANChanTrafficMonTime = (double)WLANChanTrafficMonTime.tv_sec + (double)WLANChanTrafficMonTime.tv_nsec / 1000000000.0; \
	monArgPass->ChannelDistance = WLANChanTrafficMonChanDistance; \
	 \
	pthread_attr_t tattr; \
	if((err=pthread_attr_init(&tattr)) < 0) { \
		perror("could not create thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
	if((err=pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_JOINABLE)) < 0){ \
		perror("could not modify thread-attribute"); \
		if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_create(&(sigkill_wlanmon_stuff.thread_wlan_mon), &tattr, absint_wlan_monitor_thread, (void*)monArgPass)) < 0) { \
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
#endif



#ifdef NO_WLAN_MON
#define ABSINT_THREAD_STOP_WLAN_MONITOR
#else
#define ABSINT_THREAD_STOP_WLAN_MONITOR { \
	raise(SIGKILL_WLANMON); \
	err=pthread_join(sigkill_wlanmon_stuff.thread_wlan_mon,NULL); \
	printfc(magenta,"-->WLAN-Mon");printf("-Thread is terminated.\n"); \
}
#endif




#define ABSINT_THREAD_START_STATION_MONITOR { \
	pthread_t thread_sta_mon; \
	struct absintStationMonThreadArgPassing *monArgPass; \
	 \
	monArgPass = malloc(sizeof(struct absintStationMonThreadArgPassing)); \
	monArgPass->shmem=&shmem; \
	monArgPass->sem_mainsynch=&sem_mainsynch; \
	monArgPass->sem_send=&sem_send; \
	monArgPass->threadindex=&thread_sta_mon; \
	monArgPass->ifcollect=&ifcollect; \
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
	if( (err=pthread_create(&thread_sta_mon, &tattr, absint_station_monitor_thread, (void*)monArgPass)) < 0) { \
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



/* You could remember the Thread-IDs of all active Timeouts and if one is no longer needed,
 * you could kill it before expiring. But for now just let it stay simple:
 * The Timeout expires and throws a ShMem msg, but this msg is just ignored.
 * Hence do not Declare one pthread_t before calling this macro, but create on inside a scope
 * Use it to create the thread and let it fall away after the scope.
 * If someone want to track them later on, some preparations are done (e.g. ArgPassing)
 */
#define ABSINT_THREAD_START_TIMEOUT(time_sec,time_nsec,timeoutTypeArg,timeoutIDArg) { \
	pthread_t thread_timout; \
	struct absintTimeoutThreadArgPassing *timeoutArgPass; \
	 \
	timeoutArgPass = malloc(sizeof(struct absintTimeoutThreadArgPassing)); \
	timeoutArgPass->shmem=&shmem; \
	timeoutArgPass->sem_mainsynch=&sem_mainsynch; \
	/*timeoutArgPass->sem_send=&sem_send; \
	timeoutArgPass->threadindex=&thread_timout;*/ \
	(timeoutArgPass->timeoutduration).tv_sec=time_sec; \
	(timeoutArgPass->timeoutduration).tv_nsec=time_nsec; \
	timeoutArgPass->timeoutID=timeoutIDArg; \
	timeoutArgPass->timeoutType=timeoutTypeArg; \
	 \
	pthread_attr_t tattr; \
	if((err=pthread_attr_init(&tattr)) < 0) { \
		perror("could not create thread-attribute"); \
		return MAIN_ERR_STD; \
	} \
	/*Pretty important to create the Timeout Thread "DETACHED"*/ \
	if((err=pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED)) < 0){ \
		perror("could not modify thread-attribute"); \
		if( (err=pthread_attr_destroy(&tattr)) != 0 ) { \
			perror("could not destroy thread-attribute"); \
			return MAIN_ERR_STD; \
		} \
		return MAIN_ERR_STD; \
	} \
	if( (err=pthread_create(&thread_timout, &tattr, absint_switch_mode_timeout_thread, (void*)timeoutArgPass)) < 0) { \
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





#define ABSINT_SEND_INTERFACES_WLAN printfc(blue,"Ctrl-Com");printf(": Sending WLAN-Interfaces\n"); \
msg=malloc(sizeof(struct AICtrlMsgHeader)+((wifcollect->wlanc)*sizeof(struct AICtrlConnectedAIIface))); \
memset(msg,0,sizeof(struct AICtrlMsgHeader)+((wifcollect->wlanc)*sizeof(struct AICtrlConnectedAIIface))); \
msgh.msgsize=((wifcollect->wlanc)*sizeof(struct AICtrlConnectedAIIface)); \
msgh.type=AIC_AITC_IFACE; \
for(err=0;err<(wifcollect->wlanc);err++){ \
	memcpy(((struct AICtrlConnectedAIIface *)msgp)[err].iface,(ifcollect->ifacestart)[(wifcollect->wlanidx)[err]],strlen((ifcollect->ifacestart)[(wifcollect->wlanidx)[err]])); \
	memcpy(&(((struct AICtrlConnectedAIIface *)msgp)[err].IP4),(ifcollect->ifaddrstart)[(wifcollect->wlanidx)[err]],sizeof(struct in_addr)); \
	get_freq_ssid((ifcollect->ifacestart)[(wifcollect->wlanidx)[err]],&(((struct AICtrlConnectedAIIface *)msgp)[err].freq), ((struct AICtrlConnectedAIIface *)msgp)[err].adhocssid); \
	/*get_packeterrorrateTX((ifcollect->ifacestart)[(wifcollect->wlanidx)[err]],&(((struct AICtrlConnectedAIIface *)msgp)[err].packerrrateTX));*/ \
} \
err = senddetermined(s,msg,sizeof(struct AICtrlMsgHeader)+(msgh.msgsize)); \
free(msg);
//	char test[128];
//	inet_ntop(AF_INET,&(((struct AICtrlConnectedAIIface *)msgp)->IP4),test,100);
//	depr(1,"iface %s | %s",((struct AICtrlConnectedAIIface *)msgp)->iface,test);
//	depr(1,"freq %d | ssid %s",((struct AICtrlConnectedAIIface *)msgp)->freq,((struct AICtrlConnectedAIIface *)msgp)->adhocssid);




#ifdef DEVELOPMENT_MODE
#define ABSINT_SEND_MAC_TEST_PRINT printf("Sending MAC (inverted): ");printMAC(((unsigned char *)msgp)+err*sizeof(uint64_t),6);puts(""); \
printf("\t(dezimal) %llu\n",(((uint64_t *)msgp)[err]));
#else
#define ABSINT_SEND_MAC_TEST_PRINT
#endif
#define ABSINT_SEND_MAC printfc(blue,"Ctrl-Com");printf(": Sending MAC-Addresses (Byte-inverted)\n"); \
msg=malloc(sizeof(struct AICtrlMsgHeader)+((ifcollect->ifc)*sizeof(uint64_t))); \
memset(msg,0,sizeof(struct AICtrlMsgHeader)+((ifcollect->ifc)*sizeof(uint64_t))); \
msgh.msgsize=((ifcollect->ifc)*sizeof(uint64_t)); \
msgh.type=AIC_AITC_MAC; \
int innerloop; \
for(err=0;err<(ifcollect->ifc);err++){ \
	for(innerloop=0;innerloop<6;innerloop++){ \
		*(((char *)(((uint64_t *)msgp)+err))+innerloop)=*(((ifcollect->ifmacstart)[err])+5-innerloop); \
	} \
	ABSINT_SEND_MAC_TEST_PRINT \
} \
err = senddetermined(s,msg,sizeof(struct AICtrlMsgHeader)+(msgh.msgsize)); \
free(msg);



#define ABSINT_SEND_MAC_WITHOUT_INVERSION printfc(blue,"Ctrl-Com");printf(": Sending MAC-Addresses\n"); \
msg=malloc(sizeof(struct AICtrlMsgHeader)+((ifcollect->ifc)*sizeof(uint64_t))); \
memset(msg,0,sizeof(struct AICtrlMsgHeader)+((ifcollect->ifc)*sizeof(uint64_t))); \
msgh.msgsize=((ifcollect->ifc)*sizeof(uint64_t)); \
msgh.type=AIC_AITC_MAC; \
for(err=0;err<(ifcollect->ifc);err++){ \
	memcpy(((char *)msgp)+2+err*sizeof(uint64_t),((ifcollect->ifmacstart)[err]),6); \
	printMAC(((unsigned char *)msgp)+2+err*sizeof(uint64_t),6);puts(""); \
	ABSINT_SEND_MAC_TEST_PRINT \
} \
err = senddetermined(s,msg,sizeof(struct AICtrlMsgHeader)+(msgh.msgsize)); \
free(msg);



#ifdef DEVELOPMENT_MODE
#define ABSINT_SEND_RSSI_DEV_MODE_PRINT_FOR_CHECK err=printf_sniffed_wlan_packets(wlanp);
#else
#define ABSINT_SEND_RSSI_DEV_MODE_PRINT_FOR_CHECK
#endif

#define ABSINT_SEND_RSSI_BY_CHAN struct wlansniff_chain_start *wlanp; \
wlanp=NULL; \
int freq; \
chan=1; \
{ \
enum nl80211_band band; \
band = chan <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ; \
freq = ieee80211_channel_to_frequency(chan, band); \
} \
/*Better stop a running wpa_supplicant daemon (if any) before Wifi-Sniffing on this port.*/ \
{ \
char wpasupprunning = check_running_wpa_supplicant(dev); \
stop_adhoc_wpa_supplicant(dev,wpasupprunning); \
err=wifi_package_parse(dev,freq,&wlanp); \
switch(err){ \
case ERR_WLAN_SNIFF_BAD_DEVICE: \
	printfc(YELLOW,"WARNING: "); \
	printf("Raw-WLAN-Parser was called on something not a WLAN-Device.\n\tMaybe this messed up the Interface Configuration.\n"); \
	break; \
} \
ABSINT_SEND_RSSI_DEV_MODE_PRINT_FOR_CHECK; \
/*If wpa_supplicant even was running befor sniffing*/ \
if(wpasupprunning) \
	err=start_adhoc_wpa_supplicant(dev,check_running_wpa_supplicant(dev)); \
} \
msg=malloc(sizeof(struct AICtrlMsgHeader)+sizeof(struct AICtrlMsgRSSIHeader)+((wlanp->count)*sizeof(struct WifiPackageParseData))); \
memset(msg,0,sizeof(struct AICtrlMsgHeader)+sizeof(struct AICtrlMsgRSSIHeader)+((wlanp->count)*sizeof(struct WifiPackageParseData))); \
msgh.msgsize=(((wlanp->count)*sizeof(struct WifiPackageParseData))+sizeof(struct AICtrlMsgRSSIHeader)); \
msgh.type=AIC_AITC_WLANSTAT_RSSI; \
msgprssi->count=wlanp->count; \
msgprssi->freq=wlanp->freq; \
memcpy(msgprssi+1,wlanp->start,(wlanp->count)*sizeof(struct WifiPackageParseData)); \
/*depr(1,"rssi %d",((struct WifiPackageParseData *)(msgp+1))->rssi);*/ \
err = senddetermined(s,msg,sizeof(struct AICtrlMsgHeader)+(msgh.msgsize)); \
free(msg);

#define ABSINT_SEND_RSSI struct wlansniff_chain_start *wlanp; \
wlanp=NULL; \
/*Better stop a running wpa_supplicant daemon (if any) before Wifi-Sniffing on this port.*/ \
{ \
char wpasupprunning = check_running_wpa_supplicant(dev); \
stop_adhoc_wpa_supplicant(dev,wpasupprunning); \
err=wifi_package_parse(dev,freq,&wlanp,1.0,NULL); \
switch(err){ \
case ERR_WLAN_SNIFF_BAD_DEVICE: \
	printfc(YELLOW,"WARNING: "); \
	printf("Raw-WLAN-Parser was called on something not a WLAN-Device.\n\tMaybe this messed up the Interface Configuration.\n"); \
	break; \
} \
ABSINT_SEND_RSSI_DEV_MODE_PRINT_FOR_CHECK; \
/*If wpa_supplicant even was running befor sniffing*/ \
if(wpasupprunning) \
	err=start_adhoc_wpa_supplicant(dev,check_running_wpa_supplicant(dev)); \
} \
msg=malloc(sizeof(struct AICtrlMsgHeader)+sizeof(struct AICtrlMsgRSSIHeader)+((wlanp->count)*sizeof(struct WifiPackageParseData))); \
memset(msg,0,sizeof(struct AICtrlMsgHeader)+sizeof(struct AICtrlMsgRSSIHeader)+((wlanp->count)*sizeof(struct WifiPackageParseData))); \
msgh.msgsize=(((wlanp->count)*sizeof(struct WifiPackageParseData))+sizeof(struct AICtrlMsgRSSIHeader)); \
msgh.type=AIC_AITC_WLANSTAT_RSSI; \
msgprssi->count=wlanp->count; \
msgprssi->freq=wlanp->freq; \
memcpy(msgprssi+1,wlanp->start,(wlanp->count)*sizeof(struct WifiPackageParseData)); \
/*depr(1,"rssi %d",((struct WifiPackageParseData *)(msgp+1))->rssi);*/ \
err = senddetermined(s,msg,sizeof(struct AICtrlMsgHeader)+(msgh.msgsize)); \
free(msg);





#endif /* ABSINT_H */
