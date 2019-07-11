/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */

#define NO_ABSINT_CTRL_CLI_LIVE_C_FUNCTIONS


#include <readline/readline.h>
#include <readline/history.h>
#include <net/if.h>

#include "ollerus_globalsettings.h"
#include "ollerus.h"
#include "absint.h"
#include "absint_ctrl.h"

#include "head/ollerus_extern_functions.h"







/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char *cli_rl(char *cliline){
	if(cliline){
		free (cliline);
		cliline = (char *)NULL;
	}
	/* Get a line from Terminal */
//	cliline = readline ("AI-Ctrl CLI:> ");
	cliline = readline ("");
	/* If the line has any text in it, save it on the history. */
	if (cliline && *cliline)
		add_history (cliline);
	return cliline;
}




void *absint_ctrl_cli (void* arg){
	printfc(cyan,"AI-Ctrl CLI: ");printf("Starting up.\n");
	sem_t *sem_all = (((struct AICtrlManageThreadArgPassing *)arg)->sem_all);
	//sem_t *semrd=&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_READ];
	//sem_t *semwr=&sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE];
	struct AICtrlInterThreadManageMsgHeader *manmsg=((struct AICtrlManageThreadArgPassing *)arg)->manmsg;
	free(arg);
	int err;
	int pthreadoldcancelstate;
	#define semrd &sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_READ]
	#define semwr &sem_all[ABSINT_CTRL_SEM_MANAGE_MSG_WRITE]

	static char *cliline=NULL;

	while(1){BreakCmd:
		cliline=cli_rl(cliline);
		char *substr;
		int idx;
		char *endpoint;
		err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
		if (strstr(cliline, "exit") != 0){
			exit(0);//I know, this isn't really nice. But quick for now...
		}else if (strstr(cliline, "help") != 0){

		}else if (strstr(cliline, "print") != 0){
			if (strstr(cliline, "connections") != 0){
				sem_wait(semwr);
				manmsg->msgsize=0;
				manmsg->flags=FLAG_ABSINT_CTRL_INTER_MSG_CLI;
				manmsg->idx=ABSINT_CTRL_CLI_THREAD;
				manmsg->type=AIC_INTERN_PRINT_CONNECTIONS;
				manmsg->msg=NULL;
			}else if (strstr(cliline, "dpids") != 0){
				sem_wait(semwr);
				manmsg->msgsize=0;
				manmsg->flags=FLAG_ABSINT_CTRL_INTER_MSG_CLI;
				manmsg->idx=ABSINT_CTRL_CLI_THREAD;
				manmsg->type=AIC_INTERN_PRINT_DPIDS;
				manmsg->msg=NULL;
			}
		}else if (strstr(cliline, "send") != 0){
			char dev[IFNAMSIZ];
			if (strstr(cliline, "adhoc") != 0){
				if ((substr=strstr(cliline, "ssid")) != 0){
					substr+=5;
					char ssid[33];
					endpoint=substr;
					err=0;
					while((ssid[err]=endpoint[err])!=' '){
						if((endpoint[err])=='\0'){
							printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
							goto BreakCmd;
						}
						err++;
					}
					ssid[err]='\0';
					if ((substr=strstr(cliline, "dev")) == 0){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Device-Name [dev] missing!)\n\t%s\n",cliline);
						continue;
					}
//					if(endpoint != (substr-1)){
//						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Bad ssid Value)\n\t%s\n",cliline);
//						continue;
//					}
					while((*substr)!=' '){
						if((*substr)=='\0'){
							printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
							goto BreakCmd;
						}
						substr++;
					}
					substr++;
					err=0;
					while((dev[err]=substr[err])!=' '){
						if((substr[err])=='\0'){
							printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
							goto BreakCmd;
						}
						err++;
					}
					dev[err]='\0';
					if ((substr=strstr(cliline, "idx")) == 0){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Thread-Index [idx] missing!)\n\t%s\n",cliline);
						continue;
					}
					substr+=4;
					if(((uintptr_t)substr-(uintptr_t)cliline+1) > (int)strlen(cliline)){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (No [idx] Value!)\n\t%s\n",cliline);
						continue;
					}
					idx=(int)strtol(substr, &endpoint, 10);
					if (*endpoint){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Bad Thread-Index Value [idx])\n\t%s\n",cliline);
						continue;
					}
					sem_wait(semwr);
					manmsg->msgsize=strlen(ssid)+1+strlen(dev);
					manmsg->flags=FLAG_ABSINT_CTRL_INTER_MSG_CLI;
					manmsg->idx=idx;
					manmsg->type=AIC_CTAI_SET_ADHOC_ESSID;
					manmsg->msg=malloc(strlen(ssid)+1+strlen(dev));
					memcpy(manmsg->msg,ssid,strlen(ssid));
					(manmsg->msg)[strlen(ssid)]='\0';
					memcpy((manmsg->msg)+strlen(ssid)+1,dev,strlen(dev));
				}else if ((substr=strstr(cliline, "chan")) != 0){
					substr+=5;
					int chan;
					chan=(int)strtol(substr, &endpoint, 10);
					if ((substr=strstr(cliline, "dev")) == 0){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Device-Name [dev] missing!)\n\t%s\n",cliline);
						continue;
					}
					if(endpoint != (substr-1)){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Bad chan Value)\n\t%s\n",cliline);
						continue;
					}
					while((*substr)!=' '){
						if((*substr)=='\0'){
							printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
							goto BreakCmd;
						}
						substr++;
					}
					substr++;
					err=0;
					while((dev[err]=substr[err])!=' '){
						if((substr[err])=='\0'){
							printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
							goto BreakCmd;
						}
						err++;
					}
					dev[err]='\0';
					if ((substr=strstr(cliline, "idx")) == 0){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Thread-Index [idx] missing!)\n\t%s\n",cliline);
						continue;
					}
					substr+=4;
					if(((uintptr_t)substr-(uintptr_t)cliline+1) > (int)strlen(cliline)){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (No [idx] Value!)\n\t%s\n",cliline);
						continue;
					}
					idx=(int)strtol(substr, &endpoint, 10);
					if (*endpoint){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Bad Thread-Index Value [idx])\n\t%s\n",cliline);
						continue;
					}
					sem_wait(semwr);
					manmsg->msgsize=sizeof(msgchan)+strlen(dev);
					manmsg->flags=FLAG_ABSINT_CTRL_INTER_MSG_CLI;
					manmsg->idx=idx;
					manmsg->type=AIC_CTAI_SET_ADHOC_CHAN;
					manmsg->msg=malloc(sizeof(msgchan)+strlen(dev));
					*((msgchan*)(manmsg->msg))=chan;
					memcpy(((msgchan*)(manmsg->msg))+1,dev,strlen(dev));
				}else if ((substr=strstr(cliline, "start")) != 0){
					if ((substr=strstr(cliline, "dev")) == 0){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Device-Name [dev] missing!)\n\t%s\n",cliline);
						continue;
					}
					while((*substr)!=' '){
						if((*substr)=='\0'){
							printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
							goto BreakCmd;
						}
						substr++;
					}
					substr++;
					err=0;
					while((dev[err]=substr[err])!=' '){
						if((substr[err])=='\0'){
							printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
							goto BreakCmd;
						}
						err++;
					}
					dev[err]='\0';
					if ((substr=strstr(cliline, "idx")) == 0){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Thread-Index [idx] missing!)\n\t%s\n",cliline);
						continue;
					}
					substr+=4;
					if(((uintptr_t)substr-(uintptr_t)cliline+1) > (int)strlen(cliline)){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (No [idx] Value!)\n\t%s\n",cliline);
						continue;
					}
					idx=(int)strtol(substr, &endpoint, 10);
					if (*endpoint){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Bad Thread-Index Value [idx])\n\t%s\n",cliline);
						continue;
					}
					sem_wait(semwr);
					manmsg->msgsize=strlen(dev);
					manmsg->flags=FLAG_ABSINT_CTRL_INTER_MSG_CLI;
					manmsg->idx=idx;
					manmsg->type=AIC_CTAI_START_ADHOC;
					manmsg->msg=malloc(strlen(dev));
					memcpy(manmsg->msg,dev,strlen(dev));
				}else if ((substr=strstr(cliline, "stop")) != 0){
					if ((substr=strstr(cliline, "dev")) == 0){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Device-Name [dev] missing!)\n\t%s\n",cliline);
						continue;
					}
					while((*substr)!=' '){
						if((*substr)=='\0'){
							printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
							goto BreakCmd;
						}
						substr++;
					}
					substr++;
					err=0;
					while((dev[err]=substr[err])!=' '){
						if((substr[err])=='\0'){
							printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
							goto BreakCmd;
						}
						err++;
					}
					dev[err]='\0';
					if ((substr=strstr(cliline, "idx")) == 0){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Thread-Index [idx] missing!)\n\t%s\n",cliline);
						continue;
					}
					substr+=4;
					if(((uintptr_t)substr-(uintptr_t)cliline+1) > (int)strlen(cliline)){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (No [idx] Value!)\n\t%s\n",cliline);
						continue;
					}
					idx=(int)strtol(substr, &endpoint, 10);
					if (*endpoint){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Bad Thread-Index Value [idx])\n\t%s\n",cliline);
						continue;
					}
					sem_wait(semwr);
					manmsg->msgsize=strlen(dev);
					manmsg->flags=FLAG_ABSINT_CTRL_INTER_MSG_CLI;
					manmsg->idx=idx;
					manmsg->type=AIC_CTAI_STOP_ADHOC;
					manmsg->msg=malloc(strlen(dev));
					memcpy(manmsg->msg,dev,strlen(dev));
				}else {
					printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
					continue;
				}
			}else {
				printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
				continue;
			}
		}else if (strstr(cliline, "get") != 0){
			if ((substr=strstr(cliline, "iface")) != 0){
				if ((substr=strstr(cliline, "idx")) == 0){
					printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Thread-Index [idx] missing!)\n\t%s\n",cliline);
					continue;
				}
				substr+=4;
				if(((uintptr_t)substr-(uintptr_t)cliline+1) > (int)strlen(cliline)){
					printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (No [idx] Value!)\n\t%s\n",cliline);
					continue;
				}
				idx=(int)strtol(substr, &endpoint, 10);
				if (*endpoint){
					printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Bad Thread-Index Value [idx])\n\t%s\n",cliline);
					continue;
				}
				sem_wait(semwr);
				manmsg->msgsize=0;
				manmsg->flags=FLAG_ABSINT_CTRL_INTER_MSG_CLI;
				manmsg->idx=idx;
				manmsg->type=AIC_CTAI_INQUIRE_IFACE;
				manmsg->msg=NULL;
			} else if ((substr=strstr(cliline, "rssi")) != 0){
				char dev[IFNAMSIZ];
				if ((substr=strstr(cliline, "chan")) == 0){
					printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Channel [chan] missing!)\n\t%s\n",cliline);
					continue;
				}
				substr+=5;
				int chan;
				chan=(int)strtol(substr, &endpoint, 10);
				if ((substr=strstr(cliline, "dev")) == 0){
					printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Device-Name [dev] missing!)\n\t%s\n",cliline);
					continue;
				}
				if(endpoint != (substr-1)){
					printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Bad chan Value) [after get rssi]\n\t%s\n",cliline);
					continue;
				}
				while((*substr)!=' '){
					if((*substr)=='\0'){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
						goto BreakCmd;
					}
					substr++;
				}
				substr++;
				err=0;
				while((dev[err]=substr[err])!=' '){
					if((substr[err])=='\0'){
						printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
						goto BreakCmd;
					}
					err++;
				}
				dev[err]='\0';
				if ((substr=strstr(cliline, "idx")) == 0){
					printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Thread-Index [idx] missing!)\n\t%s\n",cliline);
					continue;
				}
				substr+=4;
				if(((uintptr_t)substr-(uintptr_t)cliline+1) > (int)strlen(cliline)){
					printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (No [idx] Value!)\n\t%s\n",cliline);
					continue;
				}
				idx=(int)strtol(substr, &endpoint, 10);
				if (*endpoint){
					printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Bad Thread-Index Value [idx])\n\t%s\n",cliline);
					continue;
				}
				sem_wait(semwr);
				manmsg->msgsize=sizeof(msgchan)+strlen(dev);
				manmsg->flags=FLAG_ABSINT_CTRL_INTER_MSG_CLI;
				manmsg->idx=idx;
				manmsg->type=AIC_CTAI_INQUIRE_RSSI;
				manmsg->msg=malloc(sizeof(msgchan)+strlen(dev));
				*((msgchan*)(manmsg->msg))=chan;
				memcpy(((msgchan*)(manmsg->msg))+1,dev,strlen(dev));
			} else if ((substr=strstr(cliline, "dpids")) != 0){
				sem_wait(semwr);
				manmsg->msgsize=0;
				manmsg->flags=FLAG_ABSINT_CTRL_INTER_MSG_CLI;
				manmsg->idx=ABSINT_CTRL_CLI_THREAD;
				manmsg->type=AIC_INTERN_GET_DPIDS;
				manmsg->msg=NULL;
			}else {
				printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
				continue;
			}
		} else if ((substr=strstr(cliline, "channelswitch")) != 0){
			substr+=14;
			if ((substr=strstr(substr, "chan")) == 0){
				printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Channel [chan] missing!)\n\t%s\n",cliline);
				continue;
			}
			substr+=5;
			int chan;
			chan=(int)strtol(substr, &endpoint, 10);
			if ((substr=strstr(substr, "idx")) == 0){
				printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Thread-Index [idx] missing!)\n\t%s\n",cliline);
				continue;
			}
			if(endpoint != (substr-1)){
				printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Bad chan Value; read: %d) [after channelswitch]\n\t%s\n",chan,cliline);
				continue;
			}
			while((*substr)!=' '){
				if((*substr)=='\0'){
					printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
					goto BreakCmd;
				}
				substr++;
			}
			substr++;
			if(((uintptr_t)substr-(uintptr_t)cliline+1) > (int)strlen(cliline)){
				printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (No [idx] Value!)\n\t%s\n",cliline);
				continue;
			}
			idx=(int)strtol(substr, &endpoint, 10);
			if (*endpoint){
				printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input: (Bad Thread-Index Value [idx])\n\t%s\n",cliline);
				continue;
			}
			msgfreq switchfreq=(msgfreq)ieee80211_channel_to_frequency(chan, NL80211_BAND_2GHZ);// NL80211_BAND_5GHZ
			sem_wait(semwr);
			manmsg->msgsize=sizeof(msgfreq);
			manmsg->flags=FLAG_ABSINT_CTRL_INTER_MSG_CLI;
			manmsg->idx=idx;
			manmsg->type=AIC_INTERN_WLAN_SWITCH_FSM_START_CTRL_THREAD;
			manmsg->msg=malloc(sizeof(msgfreq));
			*(msgfreq *)(manmsg->msg)=(msgfreq)switchfreq;
		}else {
			printfc(cyan,"AI-Ctrl CLI: ");printf("Not supported Input:\n\t%s\n",cliline);
			continue;
		}

		sem_post(semrd);
		err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);
	}

	#undef semrd
	#undef smwr
}



#undef NO_ABSINT_CTRL_CLI_LIVE_C_FUNCTIONS
