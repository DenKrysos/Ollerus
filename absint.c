/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */

//#include <netlink/object.h>
//#include <netlink/utils.h>
//#include <netlink/socket.h>
//#include <netlink/route/link.h>
//#include <netlink/cache.h>
//#include <netlink/msg.h>
//#include <netlink/attr.h>
#include <net/if.h>

#include "ollerus_globalsettings.h"
#include "ollerus.h"
#include "absint.h"
#include "remainder_extended.h"

#include "head/ollerus_extern_functions.h"



msgfreq currentfreq;
char currentssid[33];





void *absint_switch_mode_timeout_thread (void* arg) {
	/* Misuse the 'msgsize' inside the shmem to pass back the timeout ID.
	 * It gets a "timeoutType" delivered. Put this as the ShMem msgtype at timeout-throwing
	 */
	struct ComThrShMem *shmem=((struct absintTimeoutThreadArgPassing *)arg)->shmem;
	sem_t *sem_mainsynch=((struct absintTimeoutThreadArgPassing *)arg)->sem_mainsynch;
//	sem_t *sem_send=((struct absintTimeoutThreadArgPassing *)arg)->sem_send;
//	pthread_t *threadindex=((struct absintTimeoutThreadArgPassing *)arg)->threadindex;
	int timeoutID =((struct absintTimeoutThreadArgPassing *)arg)->timeoutID;
	enum AICtrlMsgType timeoutType=((struct absintTimeoutThreadArgPassing *)arg)->timeoutType;
	struct timespec timeoutduration;
	memcpy(&timeoutduration,&(((struct absintTimeoutThreadArgPassing *)arg)->timeoutduration),sizeof(struct timespec));

	int err;


	do {
		err = nanosleep(&timeoutduration, &timeoutduration);
	} while (err<0);

	//Shared Memory Access
	TryShMemAccessAgain:
	sem_wait(&(shmem->sem_shmem));
	if(shmem->flags & FLAG_ABSINT_COMTHR_SHMEM_NEW){
		sem_post(&(shmem->sem_shmem));
		goto TryShMemAccessAgain;
	}
	/* Critical Section */
	FLAG_SET(shmem->flags,FLAG_ABSINT_COMTHR_SHMEM_NEW);
	shmem->type=timeoutType;
	shmem->msgsize=timeoutID;
	shmem->ShMem=NULL;
	sem_post(sem_mainsynch);
	sem_post(&(shmem->sem_shmem));

	return 0;
}







static void sigkill_wlanmon_handler(){
	int err;
	sigkill_wlanmon_stuff.cancel=1;
	pcap_breakloop(sigkill_wlanmon_stuff.handlectrl);
//	pcap_close(sigkill_wlanmon_stuff.handlectrl);
	printf("Killing ");printfc(magenta," WLAN-Mon");printf("-Thread\n");
//	pthread_cancel(sigkill_wlanmon_stuff.thread_wlan_mon);
//	err=pthread_join(sigkill_wlanmon_stuff.thread_wlan_mon,NULL);
//	printfc(green,"ERROR FROM JOIN (HANDLER): %d\n",err);
//	printfc(magenta,"-->WLAN-Mon");printf("-Thread is terminated.\n");
}
extern int wifi_package_parse(char *dev,int freq,struct wlansniff_chain_start **wlanp,double timeToMonitor,struct wlansniff_pack_stat *pack_stat);
void *absint_wlan_monitor_thread (void* arg) {
	signal(SIGKILL_WLANMON, sigkill_wlanmon_handler);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
	sigkill_wlanmon_stuff.cancel=0;

	printfc(magenta,"-->WLAN-Mon:");printf(" Thread is starting up.\n");
	struct ComThrShMem *shmem=((struct absintWLANMonThreadArgPassing *)arg)->shmem;
	sem_t *sem_mainsynch=((struct absintWLANMonThreadArgPassing *)arg)->sem_mainsynch;
	sem_t *sem_send=((struct absintWLANMonThreadArgPassing *)arg)->sem_send;
//	pthread_t *threadindex=((struct absintWLANMonThreadArgPassing *)arg)->threadindex;
	char dev[IFNAMSIZ];
	strcpy(dev,((struct absintWLANMonThreadArgPassing *)arg)->dev);
	struct timespec WLANChanTrafficMonTime;
	double timeToMonitor = ((struct absintWLANMonThreadArgPassing *)arg)->WLANChanTrafficMonTime;
	char ChanDist = ((struct absintWLANMonThreadArgPassing *)arg)->ChannelDistance;
	free(arg);
	int err;
	int pthreadoldcancelstate;
	printfc(magenta,"-->WLAN-Mon:");printf(" Working on %s.\n",dev);


	struct wlansniff_chain_start *wlanp=NULL;
	int freq, chan;
	enum nl80211_band band;

	chan=1;

	//TEMP:
//	pthread_t idx=sigkill_wlanmon_stuff.thread_wlan_mon;

	ChanLoopStart:
	if(chan>13){
		chan=1;
	}
		band = chan <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
		freq = ieee80211_channel_to_frequency(chan, band);
		if(wlanp){
			free(wlanp);
			wlanp=NULL;
		}
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
		pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
		err=wifi_package_parse(dev,freq,&wlanp,timeToMonitor,NULL);

		if(sigkill_wlanmon_stuff.cancel){
			err=MAIN_ERR_CANCELED;
			if(wlanp){
				free(wlanp);
				wlanp=NULL;
			}
			goto TerminateWLANMonThread;
		}

			#ifdef DEBUG
			printfc(yellow,"WiFi-Sniffer: ");printfc(green,"Measured Statistics: ");printf("Freq: %d | Stat: %f | CurrentFreq: %d\n",wlanp->freq,wlanp->trafficstat,currentfreq);
			#endif

//		goto SkipStatSending;

		//Shared Memory Access
		TryShMemAccessAgain:
		sem_wait(&(shmem->sem_shmem));
		if(shmem->flags & FLAG_ABSINT_COMTHR_SHMEM_NEW){
			sem_post(&(shmem->sem_shmem));
			goto TryShMemAccessAgain;
		}
		/* Critical Section */
		/* Every Time send together the currently used frequency, so the controller doesn't have to keep Track of it
		 * and this little bit more information in a message doesn't hurt */
		FLAG_SET(shmem->flags,FLAG_ABSINT_COMTHR_SHMEM_NEW | FLAG_ABSINT_COMTHR_SHMEM_SEND);
		shmem->type=AIC_AITC_WLANSTAT_TRAFFICSTAT;
		shmem->msgsize=sizeof(struct AICtrlMsgTrafficstat);
		shmem->ShMem=malloc(sizeof(struct AICtrlMsgTrafficstat));
		((struct AICtrlMsgTrafficstat *)(shmem->ShMem))->currentfreq=currentfreq;
		((struct AICtrlMsgTrafficstat *)(shmem->ShMem))->freq=wlanp->freq;
		((struct AICtrlMsgTrafficstat *)(shmem->ShMem))->trafficstat=wlanp->trafficstat;
		sem_post(sem_send);
		sem_post(&(shmem->sem_shmem));

			#ifdef DEBUG
		SkipStatSending://For Debug purpose
			#endif

	chan+=ChanDist;
	goto ChanLoopStart;


	TerminateWLANMonThread:
//	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);

	return (void *)(uintptr_t)err;
}



void absint_wlan_monitor_standalone(int argc, char **argstart,char* dev,double timeToMonitor,char ChanDist){
#define GHz5_CHANNELS_EU 1
#define GHz5_CHANNELS_EU_LOWER 2
#define GHz5_CHANNELS_EU_UPPER 3
#define GHz5_CHANNELS_USA 4
#define GHz5_CHANNELS_USA_LOWER 5
#define GHz5_CHANNELS_USA_MIDDLE 6
#define GHz5_CHANNELS_USA_UPPER 7
#define GHz5_CHANNELS_JP 8
#define GHz5_CHANNELS_NO_DFS 9

//	#ifdef DEBUG
//	{
//	char test1;
//	uint8_t test2;
//	test1=2;
//	test2=2;
//	printfc(blue,"DEBUG Dec-2.")printf(" test-char: "BYTETOBINPATTERN" | test-uint8_t: "BYTETOBINPATTERN"\n",BYTETOBIN(test1),BYTETOBIN(test2));
//	test1=0x02;
//	test2=0x02;
//	printfc(blue,"DEBUG 0x02.")printf(" test-char: "BYTETOBINPATTERN" | test-uint8_t: "BYTETOBINPATTERN"\n",BYTETOBIN(test1),BYTETOBIN(test2));
//	test1=64;
//	test2=64;
//	printfc(blue,"DEBUG Dec-64.")printf(" test-char: "BYTETOBINPATTERN" | test-uint8_t: "BYTETOBINPATTERN"\n",BYTETOBIN(test1),BYTETOBIN(test2));
//	test1=0x40;
//	test2=0x40;
//	printfc(blue,"DEBUG 0x40.")printf(" test-char: "BYTETOBINPATTERN" | test-uint8_t: "BYTETOBINPATTERN"\n",BYTETOBIN(test1),BYTETOBIN(test2));
//	}
//	{
//	short test1;
//	uint16_t test2;
//	test1=2;
//	test2=2;
//	printfc(blue,"DEBUG Dec-2.")
//		printf("test-short: "BYTETOBINPATTERN"-"BYTETOBINPATTERN,BYTETOBIN( (*((char *)(&(test1)))) ),BYTETOBIN( (*(((char *)(&(test1)))+1)) ));
//		printf(" | test-uint16_t: "BYTETOBINPATTERN"-"BYTETOBINPATTERN"\n",BYTETOBIN( (*((char *)(&(test2)))) ),BYTETOBIN( (*(((char *)(&(test2)))+1)) ));
//	test1=0x02;
//	test2=0x02;
//	printfc(blue,"DEBUG 0x02.")
//		printf("test-short: "BYTETOBINPATTERN"-"BYTETOBINPATTERN,BYTETOBIN( (*((char *)(&(test1)))) ),BYTETOBIN( (*(((char *)(&(test1)))+1)) ));
//		printf(" | test-uint16_t: "BYTETOBINPATTERN"-"BYTETOBINPATTERN"\n",BYTETOBIN( (*((char *)(&(test2)))) ),BYTETOBIN( (*(((char *)(&(test2)))+1)) ));
//	test1=64;
//	test2=64;
//	printfc(blue,"DEBUG Dec-64.")
//		printf("test-short: "BYTETOBINPATTERN"-"BYTETOBINPATTERN,BYTETOBIN( (*((char *)(&(test1)))) ),BYTETOBIN( (*(((char *)(&(test1)))+1)) ));
//		printf(" | test-uint16_t: "BYTETOBINPATTERN"-"BYTETOBINPATTERN"\n",BYTETOBIN( (*((char *)(&(test2)))) ),BYTETOBIN( (*(((char *)(&(test2)))+1)) ));
//	test1=0x40;
//	test2=0x40;
//	printfc(blue,"DEBUG 0x40.")
//		printf("test-short: "BYTETOBINPATTERN"-"BYTETOBINPATTERN,BYTETOBIN( (*((char *)(&(test1)))) ),BYTETOBIN( (*(((char *)(&(test1)))+1)) ));
//		printf(" | test-uint16_t: "BYTETOBINPATTERN"-"BYTETOBINPATTERN"\n",BYTETOBIN( (*((char *)(&(test2)))) ),BYTETOBIN( (*(((char *)(&(test2)))+1)) ));
//	}
//	#endif

	signal(SIGKILL_WLANMON, sigkill_wlanmon_handler);
	pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
	sigkill_wlanmon_stuff.cancel=0;

	printfc(magenta,"-->WLAN-Mon");printf(" is starting up.\n");
	int err;
	printfc(magenta,"-->WLAN-Mon:");printf(" Working on %s.\n",dev);

	char chan_range_5ghz;
	struct wlansniff_chain_start *wlanp=NULL;
	int freq, chan;
	enum nl80211_band band;
	struct wlansniff_pack_stat pack_stat;

	memset(&pack_stat,0,sizeof(pack_stat));

	//TEMP:
//	pthread_t idx=sigkill_wlanmon_stuff.thread_wlan_mon;

	if(argc<1){
		printfc(gray,"NOTE: ");printf("No Radio-Band given. So i'll monitor 2.4 GHz, with channels specified in cfg-File.\n");
		goto Trace2_4GHz;
	}else{
		if (strcmp(*(argstart), "2GHz") == 0){
			Trace2_4GHz:
			printf("Start to monitor the 2.4GHz Band, with channels specified in the cfg-File.\n");

			Monitor2GHZ:
			chan=1;
		//	band = chan <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
			ChanLoopStart2GHZ:
			if(chan>13){
				chan=1;
			}
			band = chan <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
			freq = ieee80211_channel_to_frequency(chan, band);
			if(wlanp){
				free(wlanp);
				wlanp=NULL;
			}
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
			err=wifi_package_parse(dev,freq,&wlanp,timeToMonitor,&pack_stat);

			if(sigkill_wlanmon_stuff.cancel){
				err=MAIN_ERR_CANCELED;
				if(wlanp){
					free(wlanp);
					wlanp=NULL;
				}
				goto TerminateWLANMonThread;
			}

				#ifdef DEBUG
				printfc(yellow,"WiFi-Sniffer: ");printfc(green,"Measured Statistics: ");printf("Freq: %d | Stat: %f\n",wlanp->freq,wlanp->trafficstat);
				#endif


			chan+=ChanDist;
			goto ChanLoopStart2GHZ;
		}else if (strcmp(*(argstart), "5GHz") == 0){
			argc--;
			argstart++;
			printf("Start to monitor the 5GHz Band.\n");
			printfc(gray,"NOTE: ");printf("After 5GHz you can also specify the used channel-range:\n");
				#define YES ANSI_COLOR_GREEN"Yes"ANSI_COLOR_RESET
				#define NO ANSI_COLOR_RED"No "ANSI_COLOR_RESET
				#define SPACE1 "    "
				#define SPACE1_1 "   "
				#define SPACE2 "    "
				#define SPACE3 "    "
				#define SPACE4 "      "
				#define SPACE5 "    "
				#define SPACE6 "    "
				#define SPACE7 "       "
				#define SPACE8 "     "
				#define SPACE9 "   "
			printf(" Channel  eu   eulower  euupper  usa  usalower  usaupper  jp   nodfs\n");
			printf(SPACE1"36"SPACE2 YES SPACE3 YES SPACE4 NO SPACE5 YES SPACE6 YES SPACE7 NO SPACE8 YES SPACE9 YES "\n");
			printf(SPACE1"40"SPACE2 YES SPACE3 YES SPACE4 NO SPACE5 YES SPACE6 YES SPACE7 NO SPACE8 YES SPACE9 YES "\n");
			printf(SPACE1"44"SPACE2 YES SPACE3 YES SPACE4 NO SPACE5 YES SPACE6 YES SPACE7 NO SPACE8 YES SPACE9 YES "\n");
			printf(SPACE1"48"SPACE2 YES SPACE3 YES SPACE4 NO SPACE5 YES SPACE6 YES SPACE7 NO SPACE8 YES SPACE9 YES "\n");
			printf(SPACE1"52"SPACE2 YES SPACE3 YES SPACE4 NO SPACE5 YES SPACE6 YES SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1"56"SPACE2 YES SPACE3 YES SPACE4 NO SPACE5 YES SPACE6 YES SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1"60"SPACE2 YES SPACE3 YES SPACE4 NO SPACE5 YES SPACE6 YES SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1"64"SPACE2 YES SPACE3 YES SPACE4 NO SPACE5 YES SPACE6 YES SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1);printfc(gray,"===--------------------------------------------------------------");printf("\n");
			printf(SPACE1_1"100"SPACE2 YES SPACE3 NO SPACE4 YES SPACE5 NO SPACE6 NO SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"104"SPACE2 YES SPACE3 NO SPACE4 YES SPACE5 NO SPACE6 NO SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"108"SPACE2 YES SPACE3 NO SPACE4 YES SPACE5 NO SPACE6 NO SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"112"SPACE2 YES SPACE3 NO SPACE4 YES SPACE5 NO SPACE6 NO SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"116"SPACE2 YES SPACE3 NO SPACE4 YES SPACE5 NO SPACE6 NO SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"120"SPACE2 YES SPACE3 NO SPACE4 YES SPACE5 NO SPACE6 NO SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"124"SPACE2 YES SPACE3 NO SPACE4 YES SPACE5 NO SPACE6 NO SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"128"SPACE2 YES SPACE3 NO SPACE4 YES SPACE5 NO SPACE6 NO SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"132"SPACE2 YES SPACE3 NO SPACE4 YES SPACE5 NO SPACE6 NO SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"136"SPACE2 YES SPACE3 NO SPACE4 YES SPACE5 NO SPACE6 NO SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"140"SPACE2 YES SPACE3 NO SPACE4 YES SPACE5 NO SPACE6 NO SPACE7 NO SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1);printfc(gray,"===--------------------------------------------------------------");printf("\n");
			printf(SPACE1_1"147"SPACE2 NO SPACE3 NO SPACE4 NO SPACE5 YES SPACE6 NO SPACE7 YES SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"151"SPACE2 NO SPACE3 NO SPACE4 NO SPACE5 YES SPACE6 NO SPACE7 YES SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1"155"SPACE2 NO SPACE3 NO SPACE4 NO SPACE5 YES SPACE6 NO SPACE7 YES SPACE8 NO SPACE9 NO "\n");
			printf(SPACE1_1);printfc(gray,"===--------------------------------------------------------------");printf("\n");
			printf(SPACE1_1"167"SPACE2 NO SPACE3 NO SPACE4 NO SPACE5 YES SPACE6 NO SPACE7 YES SPACE8 NO SPACE9 NO "\n");
				#undef YES
				#undef NO
				#undef SPACE1
				#undef SPACE1_1
				#undef SPACE2
				#undef SPACE3
				#undef SPACE4
				#undef SPACE5
				#undef SPACE6
				#undef SPACE7
				#undef SPACE8
				#undef SPACE9
			printf("If no explicit range is passed, the monitor will only use the first 4 lower channels (36-48).\n\tThese are the ones without DFS (and equally to the Japanese Standard).\n");
			if(argc<1){
				chan_range_5ghz=GHz5_CHANNELS_NO_DFS;
			}else{
				if (strcmp(*(argstart), "eu") == 0){
					printf("Monitoring Channels [36-64], [100-140]\n");
					chan_range_5ghz=GHz5_CHANNELS_EU;
				}else if (strcmp(*(argstart), "eulower") == 0){
					printf("Monitoring Channels [36-64]\n");
					chan_range_5ghz=GHz5_CHANNELS_EU_LOWER;
				}else if (strcmp(*(argstart), "euupper") == 0){
					printf("Monitoring Channels [100-140]\n");
					chan_range_5ghz=GHz5_CHANNELS_EU_UPPER;
				}else if (strcmp(*(argstart), "usa") == 0){
					printf("Monitoring Channels [36-64], [147-155], 167\n");
					chan_range_5ghz=GHz5_CHANNELS_USA;
				}else if (strcmp(*(argstart), "usalower") == 0){
					printf("Monitoring Channels [36-64]\n");
					chan_range_5ghz=GHz5_CHANNELS_USA_LOWER;
				}else if (strcmp(*(argstart), "usaupper") == 0){
					printf("Monitoring Channels [147-155], 167\n");
					chan_range_5ghz=GHz5_CHANNELS_USA_UPPER;
				}else if (strcmp(*(argstart), "jp") == 0){
					printf("Monitoring Channels [36-48]\n");
					chan_range_5ghz=GHz5_CHANNELS_JP;
				}else if (strcmp(*(argstart), "nodfs") == 0){
					printf("Monitoring Channels [36-48]\n");
					chan_range_5ghz=GHz5_CHANNELS_NO_DFS;
				}else{
					printf("Invalid Argument passed as the 5GHz-Channel-Range (after 5GHz). So i'll use the \"No DFS Range\" (36-48).\n");
					chan_range_5ghz=GHz5_CHANNELS_NO_DFS;
				}
			}
//			goto Monitor5GHZ;

//			Monitor5GHZ:
			//The whole Processing into a macro
			//With this i've done the channel-range switching only once at the beginning
			//-> These steps are only done once at startup and not on every run through: Better Performance.
			//=======================================================
				#ifdef DEBUG
					#define GHz5_MONITORING_PROCESSING_DEBUG_STAT_PRINT printfc(yellow,"WiFi-Sniffer: ");printfc(green,"Measured Statistics: ");printf("Freq: %d | Stat: %f\n",wlanp->freq,wlanp->trafficstat);
				#else
					#define GHz5_MONITORING_PROCESSING_DEBUG_STAT_PRINT
				#endif
				#define GHz5_MONITORING_PROCESSING(entrance_point,start_channel,channel_range) \
					chan=start_channel; \
					band = chan <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ; \
					goto EntrancePoint_##entrance_point; \
					while(1){ \
						channel_range else{chan+=4;} \
						EntrancePoint_##entrance_point: \
						freq = ieee80211_channel_to_frequency(chan, band); \
						if(wlanp){ \
							free(wlanp); \
							wlanp=NULL; \
						} \
						pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL); \
						pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL); \
						err=wifi_package_parse(dev,freq,&wlanp,timeToMonitor,&pack_stat); \
						 \
						if(sigkill_wlanmon_stuff.cancel){ \
							err=MAIN_ERR_CANCELED; \
							if(wlanp){ \
								free(wlanp); \
								wlanp=NULL; \
							} \
							goto TerminateWLANMonThread; \
						} \
						 \
						GHz5_MONITORING_PROCESSING_DEBUG_STAT_PRINT \
						 \
					}
			//=======================================================

			switch(chan_range_5ghz){
			case GHz5_CHANNELS_EU:
				GHz5_MONITORING_PROCESSING(GHz5_CHANNELS_EU,36,
					if(140<=chan){
						chan=36;
					}else if(64<=chan && 100>=chan){
						chan=100;
					}
				)
				break;
			case GHz5_CHANNELS_EU_LOWER:
				GHz5_MONITORING_PROCESSING(GHz5_CHANNELS_EU_LOWER,36,
					if(64<=chan){
						chan=36;
					}
				)
				break;
			case GHz5_CHANNELS_EU_UPPER:
				GHz5_MONITORING_PROCESSING(GHz5_CHANNELS_EU_UPPER,100,
					if(140<=chan){
						chan=100;
					}
				)
				break;
			case GHz5_CHANNELS_USA:
				GHz5_MONITORING_PROCESSING(GHz5_CHANNELS_USA,36,
					if(167<=chan){
						chan=36;
					}else if(155<=chan && 167>=chan){
						chan=167;
					}else if(64<=chan && 147>=chan){
						chan=147;
					}
				)
				break;
			case GHz5_CHANNELS_USA_LOWER:
				GHz5_MONITORING_PROCESSING(GHz5_CHANNELS_USA_LOWER,36,
					if(64<=chan){
						chan=36;
					}
				)
				break;
			case GHz5_CHANNELS_USA_MIDDLE:
				//Not 'really' existent. Channel 100 - 140 are forbidden...
				break;
			case GHz5_CHANNELS_USA_UPPER:
				GHz5_MONITORING_PROCESSING(GHz5_CHANNELS_USA_MIDDLE,147,
					if(167<=chan){
						chan=147;
					}else if(155<=chan && 167>=chan){
						chan=167;
					}
				)
				break;
			case GHz5_CHANNELS_JP:
			case GHz5_CHANNELS_NO_DFS:
				GHz5_MONITORING_PROCESSING(GHz5_CHANNELS_NO_DFS,36,
					if(48<=chan){
						chan=36;
					}
				)
				break;
			default:
				break;
			}
			#undef GHz5_MONITORING_PROCESSING
			#undef GHz5_CHANNELS_EU
			#undef GHz5_CHANNELS_EU_LOWER
			#undef GHz5_CHANNELS_EU_UPPER
			#undef GHz5_CHANNELS_USA
			#undef GHz5_CHANNELS_USA_LOWER
			#undef GHz5_CHANNELS_USA_MIDDLE
			#undef GHz5_CHANNELS_USA_UPPER
			#undef GHz5_CHANNELS_JP
			#undef GHz5_CHANNELS_NO_DFS
		}else if (strcmp(*(argstart), "chan") == 0){
			argc--;
			argstart++;
			if(argc<1){
				printfc(red,"ERROR: ");printf("No channel-list given after \"wlanmonitor chan\". Exiting.\n");
				exit(MAIN_ERR_BAD_CMDLINE);
			}
			static struct monitor_channel_list {
				void (*inc)();
				unsigned int count;//After initialization the count gets a --, so that it matches the max index instead of the number further on
				unsigned int current;
				unsigned int *channels;
				unsigned int *freq;
				unsigned int *band;
			} chan_lst;
			void *monitor_channel_list_increment(struct monitor_channel_list *self){
					if(self->current>=self->count){
						self->current=0;
					}else{
						self->current++;
					}
					return NULL;
				};
			chan_lst.inc=monitor_channel_list_increment;//(&chan_lst);
			#define CHAN_LST_NEXT(CHAN_LST) ((CHAN_LST.inc)(&CHAN_LST));
			#define CHAN_LST_CUR(CHAN_LST) ((CHAN_LST.channels)[CHAN_LST.current])
			#define CHAN_LST_CUR_BAND(CHAN_LST) ((CHAN_LST.band)[CHAN_LST.current])
			#define CHAN_LST_CUR_FREQ(CHAN_LST) ((CHAN_LST.freq)[CHAN_LST.current])
			chan_lst.count=1;
			{
				char *arg_lst_proc_loop;
				char *endpoint;
				for(arg_lst_proc_loop=*argstart;*arg_lst_proc_loop!='\0' && *arg_lst_proc_loop!=' ';arg_lst_proc_loop++){
					if(','==*arg_lst_proc_loop)
						chan_lst.count++;
				}
				chan_lst.channels=malloc((chan_lst.count)*sizeof(*(chan_lst.channels)));
				chan_lst.freq=malloc((chan_lst.count)*sizeof(*(chan_lst.freq)));
				chan_lst.band=malloc((chan_lst.count)*sizeof(*(chan_lst.band)));
				chan_lst.current=0;
				arg_lst_proc_loop=*argstart;
				while(chan_lst.current<chan_lst.count){
					chan_lst.channels[chan_lst.current] = (unsigned int)strtol(arg_lst_proc_loop, &endpoint, 10);
					if ( ','==*endpoint || '\0'==*endpoint) {
						arg_lst_proc_loop=endpoint+1;
						chan_lst.band[chan_lst.current] = CHAN_LST_CUR(chan_lst) <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
						chan_lst.freq[chan_lst.current] = ieee80211_channel_to_frequency(CHAN_LST_CUR(chan_lst), CHAN_LST_CUR_BAND(chan_lst));
						chan_lst.current++;
					}else{
						printfc(red,"ERROR: ");printf("Bad Format at the channel list. Terminating... Please try again.\n");
						exit(MAIN_ERR_BAD_CMDLINE);
					}
				}
				chan_lst.count--;
				chan_lst.current=0;
			}

//			for(err=0;err<7;err++){
//				printf("Chan: %d | Freq: %d | Band: %d\n",CHAN_LST_CUR(chan_lst),CHAN_LST_CUR_FREQ(chan_lst),CHAN_LST_CUR_BAND(chan_lst));
//				CHAN_LST_NEXT(chan_lst)
//			}
//			exit(1);

			while(1){
				if(wlanp){
					free(wlanp);
					wlanp=NULL;
				}
				pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
				pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
				err=wifi_package_parse(dev,CHAN_LST_CUR_FREQ(chan_lst),&wlanp,timeToMonitor,&pack_stat);

				if(sigkill_wlanmon_stuff.cancel){
					err=MAIN_ERR_CANCELED;
					if(wlanp){
						free(wlanp);
						wlanp=NULL;
					}
					goto TerminateWLANMonThread;
				}

					#ifdef DEBUG
					printfc(yellow,"WiFi-Sniffer: ");printfc(green,"Measured Statistics: ");printf("Freq: %d | Stat: %f\n",wlanp->freq,wlanp->trafficstat);
					#endif

				CHAN_LST_NEXT(chan_lst)
			}


			#undef CHAN_LST_NEXT
			#undef CHAN_LST_CUR
			#undef CHAN_LST_CUR_BAND
			#undef CHAN_LST_CUR_FREQ
		}else{
			printfc(gray,"NOTE: ");printf("Invalid Argument given after \"wlanmonitor\" (please tell me the Band to monitor [2GHZ, 5GHz])\n");
			printf("Resulting i'm going to monitor the 2.4GHz Band, with channels specified in the cfg-File.\n");
		}
	}

	TerminateWLANMonThread:
//	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);

	return (void)(uintptr_t)err;
}





void *absint_station_monitor_thread (void* arg) {
	/* Monitors the Station for Changes (like a bad Wireless Connection
	 * - Bitrate, Noise, ...)
	 * If there is a noticeable Change -> pass new Value to main-thread (shmem)
	 * The main handles it - evtl. send something to controller
	 */
	struct ComThrShMem *shmem=((struct absintStationMonThreadArgPassing *)arg)->shmem;
	sem_t *sem_mainsynch=((struct absintStationMonThreadArgPassing *)arg)->sem_mainsynch;
	sem_t *sem_send=((struct absintStationMonThreadArgPassing *)arg)->sem_send;
	pthread_t *threadindex=((struct absintStationMonThreadArgPassing *)arg)->threadindex;
	struct AbsintInterfaces *ifcollect=((struct absintStationMonThreadArgPassing *)arg)->ifcollect;
	free(arg);
	int err;
	int pthreadoldcancelstate;

	err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
	err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);

	return (void *)(uintptr_t)err;
}


void *absint_com_controller_listen(void* arg){
	struct ComThrShMem *shmem=((struct absintListenThreadArgPassing *)arg)->shmem;
	sem_t *sem_mainsynch=((struct absintListenThreadArgPassing *)arg)->sem_mainsynch;
	sem_t *sem_send=((struct absintListenThreadArgPassing *)arg)->sem_send;
	pthread_t *threadindex=((struct absintListenThreadArgPassing *)arg)->threadindex;
	struct AbsintInterfaces *ifcollect=((struct absintListenThreadArgPassing *)arg)->ifcollect;
	int s=((struct absintListenThreadArgPassing *)arg)->socket;
	unsigned int *bytes_recvd_ctrl=((struct absintListenThreadArgPassing *)arg)->bytes_recvd;
	//Free the arguments passing memory space
	free(arg);
	int err;
	int pthreadoldcancelstate;
	struct AICtrlMsgHeader recvh;
	unsigned int bytes_recvd_local;


	//REMEMBER:
	//After every socket-readout check for bytes_recvd==0
	//Then there is a disconnect -> handle it
    while((bytes_recvd_local=recv(s,&recvh,sizeof(struct AICtrlMsgHeader),0))>0){
		printf(ANSI_COLOR_RED);
    	printf("\n-->Message from Server: ");
		printf(ANSI_COLOR_RESET);
		printf("%d Bytes (+%d Bytes Header). Type: ",recvh.msgsize,bytes_recvd_local);
		PRINT_AICTRL_MSGTYPE(recvh.type);puts("");

		err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
		TryShMemAccessAgain:
		sem_wait(&(shmem->sem_shmem));
		if(shmem->flags & FLAG_ABSINT_COMTHR_SHMEM_NEW){
			sem_post(&(shmem->sem_shmem));
			goto TryShMemAccessAgain;
		}
		//Shared Memory Access
		shmem->msgsize=recvh.msgsize;
		shmem->type=recvh.type;
		shmem->flags=FLAG_ABSINT_COMTHR_SHMEM_NEW|FLAG_ABSINT_COMTHR_SHMEM_SRC_CTRL;
		shmem->ShMem=malloc(recvh.msgsize);
		char temp[128];
//		depr(rec2,"bytes rec %d | msgsize %d | rest %d",bytes_recvd_local,recvh.msgsize,recv(s,temp,128,MSG_DONTWAIT|MSG_PEEK));
		if(recvh.msgsize>0){
			bytes_recvd_local=recv(s,shmem->ShMem,shmem->msgsize,0);
//			depr(rec3,"bytes rec %d | msgsize %d | rest %d",bytes_recvd_local,recvh.msgsize,recv(s,temp,128,MSG_DONTWAIT|MSG_PEEK));
			while(bytes_recvd_local<recvh.msgsize){
				bytes_recvd_local+=recv(s,(shmem->ShMem)+bytes_recvd_local,(shmem->msgsize)-bytes_recvd_local,0);
//				depr(rec4,"bytes rec %d | msgsize %d | rest %d",bytes_recvd_local,recvh.msgsize,recv(s,temp,128,MSG_DONTWAIT|MSG_PEEK));
			}
		}
		sem_post(sem_mainsynch);
		NoMain:
		sem_post(&(shmem->sem_shmem));
		err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);
//		depr(rec5,"bytes rec %d | msgsize %d | rest %d",bytes_recvd_local,recvh.msgsize,recv(s,temp,128,MSG_DONTWAIT|MSG_PEEK));

    }

	*bytes_recvd_ctrl = bytes_recvd_local;
	sem_post(sem_send);

	return (void *)(uintptr_t)err;
}
/*
 * Die Komm-Threads brauchen:
 * - Destination IP4 Address
 * - Destination Port
 * - Semaphore1
 * - Semaphore2
 * - Pointer auf ShMem
 * - Pointer auf die Device Informationen (Interfaces)
*/
/*
 * Als Nachrichtenempfangskonzept sowas wie:
 * Die Empfänger hängen prinzipiell blockt am socket recv();
 * Die Sender schicken Nachrichten von prinzipiell unbestimmter Länge, schicken jedoch als erstes
 * die Nachrichtenlänge.
 * Wenn also begonnen wird aus einem Socket auszulesen, wird sichergestellt, dass man genügend Bytes für den ersten Eintrag hat
 * (je nachdem wie ich die Nachrichtenlänge nun kodiere; int, long, long long; zwischen 8 und 64 Bit)
 * Mit diesem Wert schließlich wird das Socket solange ausgelesen, bis die gesamte Nachricht im lokalen Puffer steht.
 * Dann auswerten -> Operationen unternehmen -> Sprung zurück nach oben und blocked vor das Socket
	//ZU BEACHTEN:
	//After every socket-readout check for bytes_recvd==0
	//Then there is a disconnect -> handle it
 *
 * Damit lässt sich das "Problem" beheben, dass nicht bestimmt werden kann wieviel von einem Sendevorgang, man
 * pro Auslesevorgang aus einem Socket bekommt
 *
 * Dies zumindest für meine Inter-AI Kommunikation.
 * Mag sich ergeben, dass vom Controller Vorgaben bestehen, die ich beachten muss.
 */
void *absint_com_controller(void* arg){
	int aictrlport=((struct absintComThreadArgPassing *)arg)->destport;
	struct in_addr sdnctrlIP4=((struct absintComThreadArgPassing *)arg)->destIP4;
	struct ComThrShMem *shmem=((struct absintComThreadArgPassing *)arg)->shmem;
	sem_t *sem_mainsynch=((struct absintComThreadArgPassing *)arg)->sem_mainsynch;
	sem_t *sem_send=((struct absintComThreadArgPassing *)arg)->sem_send;
	pthread_t *threadindex=((struct absintComThreadArgPassing *)arg)->threadindex;
	struct AbsintInterfaces *ifcollect=((struct absintComThreadArgPassing *)arg)->ifcollect;
	struct AbsintInterfacesWLAN *wifcollect=((struct absintComThreadArgPassing *)arg)->wifcollect;
	//Free the arguments passing memory space
	free(arg);
	int err;
	int pthreadoldcancelstate;

	#define msgh (*((struct AICtrlMsgHeader *)msg))
	#define msgp ((char*)((struct AICtrlMsgHeader *)msg+1))
	#define msgprssi ((struct AICtrlMsgRSSIHeader *)((struct AICtrlMsgHeader *)msg+1))
	char *msg;

	ABSINT_INET_SOCKET_CONTROLLER_COM_CONNECT;

	ABSINT_INET_SOCKET_CONTROLLER_COM_START_LISTEN_THREAD;

	ABSINT_SEND_INTERFACES_WLAN;

	//Now, first wait a bit, to let the Ai-Controller do some neccessary stuff, like
	//refreshing his known DPIDs from the SDN-Ctrl
	WAITNS(1,MS_TO_NS(0))

	ABSINT_SEND_MAC;

	//Sniffs WLAN Packets and sends them to the controller
//	{
//	char *dev;
//	dev=(ifcollect->ifacestart)[(wifcollect->wlanidx)[0]];
//	int chan=1;
//	ABSINT_SEND_RSSI_BY_CHAN;
//	}


	//REMEMBER:
	//After every socket-readout check for bytes_recvd==0
	//Then there is a disconnect -> handle it
    while(1){
    	NextRun:
		err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
		sem_wait(sem_send);
    	//First check if the connection is still standing.
    	//The listening thread changes the bytes_recvd value to zero
    	//if the connection got closed from the client
		if(bytes_recvd_ctrl == 0) {
			printf("\t- Connection: Server disconnected | ERRNO: %d\n",errno);
			fflush(stdout);
			goto ConnectionClosed;
		} else if(bytes_recvd_ctrl == -1) {
			fprintf(stderr, "\t- Connection: recv failed\n");
			goto ConnectionClosed;
		}
//		if(FLAG_CHECK(shmem->flags,FLAG_ABSINT_COMTHR_SHMEM_CHAN_SWITCH_MODE)){
//
//		}
		TryShMemAccessAgain:
		sem_wait(&(shmem->sem_shmem));
		if((shmem->flags & FLAG_ABSINT_COMTHR_SHMEM_NEW)&&(shmem->flags & FLAG_ABSINT_COMTHR_SHMEM_SEND)){
			//Thats the condition to have something to send
		}else {
			sem_post(&(shmem->sem_shmem));
			goto TryShMemAccessAgain;
		}
		//Shared Memory Access
		switch(shmem->type){
		int freq;
		char dev[IFNAMSIZ];
		case AIC_AITC_WLANSTAT_TRAFFICSTAT:
		case AIC_AITC_BITRATE:
			goto GenericSend;
			break;
		case AIC_CTAI_INQUIRE_IFACE:
			free(shmem->ShMem);
			shmem->ShMem=NULL;
			shmem->flags=0;
			shmem->msgsize=0;
			shmem->type=0;
			sem_post(&(shmem->sem_shmem));
			err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);
			ABSINT_SEND_INTERFACES_WLAN;
			int temp;
			printfc(yellow,"\n--------------------------------------------------\n");
//			get_packeterrorrateTX("sta2-eth0",&temp);
			goto NextRun;
			break;
		case AIC_CTAI_INQUIRE_RSSI:
			freq=*((msgfreq*)(shmem->ShMem));
			memcpy(dev,(char*)((msgfreq*)(shmem->ShMem)+1),(shmem->msgsize)-sizeof(msgfreq));
			dev[(shmem->msgsize)-sizeof(msgfreq)]='\0';
			free(shmem->ShMem);
			shmem->ShMem=NULL;
			shmem->flags=0;
			shmem->msgsize=0;
			shmem->type=0;
			sem_post(&(shmem->sem_shmem));
			err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);
			ABSINT_SEND_RSSI;
			goto NextRun;
			break;
		default:
//			printfc(yellow,"\tWARNING: ");printf("Message Sending of undefined Type: Type %d\n",shmem->type);
//			printf("\t--> Hence trying generic Sending. (Direct Passthrough)\n");
			GenericSend:
			msg=malloc(sizeof(struct AICtrlMsgHeader)+(shmem->msgsize));
			msgh.msgsize=(shmem->msgsize);
			msgh.type=shmem->type;
			memcpy(msgp,shmem->ShMem,shmem->msgsize);
			free(shmem->ShMem);
			shmem->ShMem=NULL;
			shmem->flags=0;
			shmem->msgsize=0;
			shmem->type=0;
			sem_post(&(shmem->sem_shmem));
			err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);
			printfc(green,"-->Sending Msg:");printf(" %d Bytes (+%d Bytes Header)\n",msgh.msgsize,sizeof(struct AICtrlMsgHeader));
			err = senddetermined(s,msg,sizeof(struct AICtrlMsgHeader)+(msgh.msgsize));
			free(msg);
			goto NextRun;
			break;
		}
		//Free and let go
		free(shmem->ShMem);
		shmem->ShMem=NULL;
		shmem->flags=0;
		shmem->msgsize=0;
		shmem->type=0;
		sem_post(&(shmem->sem_shmem));
		err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);

    }
    ConnectionClosed:
	//TODO: Something like a proper handling of a reconnect
	#undef msgh
	#undef msgp
	#undef msgprssi
	printfc(red,"ERROR");printf(": Server Connection closed. Exiting.\n");
	close(s);
	exit(NETWORK_ERR_CONNECTION_CLOSED);

	return (void *)(uintptr_t)err;
}


/* About the different Working-Modes: >>Autonom<< and >>With Controller Messages<<
 * Later the AI will be extended with an autonomous Working Mode, where it takes the Switch-Decisions by itself.
 * Then it has to be able to jump between this Working Modes (like when the Controller becomes [temporarily] unreachable)
 * Because of this Capability to jump between the working Modes we go this way:
 * 1. Rename the "absint_auto_controller" to "absint_auto" (maybe done, when you read this comment and i have forgotten to change the comment ;oP)
 * 2. Change the names on the Call-Places so that every Working-Mode Entrance initially calls the same function
 * 3. Implement the two "Working Modes" in two looped Traces.
 * 4. This different (independent) Loop-Traces get Entrance and Exit Points (Like Jump Markers and Jumps to a Point after both.
 * 5. In between this traces comes the calculations/checks if we should switch the Working Mode -> Jump to other Entrance Point
 * For the initial Start
 * 6. Give the "absint_auto" Function another additional Argument. With this Argument comes a Switch after all the Setup at the function start
 * 		(the Config-Read, Connections, Threads and everything...), which decides which Working-Mode is initially choosen
 * 		--> Jump to corresponding Entrance-Point.
 * 		Alternative: Start the function every time the same: Let it connect to the Controller. If this Connection establishes appropriate,
 * 		its fine and we work with the controller, like for now. If the AI can't connect to a Controller, than we go into autonomous mode
 *
 * An Alternative would be to implement them in two Threads, which activate/deactivate themselves and each other with Semaphores.
 */
int absint_auto_controller(int argc, char **argstart,struct nl80211_state *sktctr,struct CommandContainer *cmd,struct aiconnectmode aiconnect){
	int err;err=0;
	int i;
	argc--;argstart++;//Now the arguments are right after the "controller" from "absint auto controller" (if any)
	struct SwitchFSM switchfsm;
	switchfsm.state=CHANSWITCH_FSM_STATE_NONE;
	switchfsm.instanceid=0;
	int currentTimeoutID;currentTimeoutID=0;
	printf("\nStarting Abstract Interface...\n");
	printf("Going into >>non-stop operation<< with PHYsical Interface Switch ");
	ANSICOLORSET(ANSI_COLOR_RED);
	printf("Mode");
	ANSICOLORRESET;
	printf(":\n");
	printf("\tDecision on ");
	ANSICOLORSET(ANSI_COLOR_RED);
	printf("SDN-Controller");
	ANSICOLORRESET;
	printf(" (AI follows Messages from Controller)\n");

	CREATE_PROGRAM_PATH(*args);
	ABSINT_READ_CONFIG_FILE;

	struct AbsintInterfaces ifcollect;
	ifcollect.neighbours.dnstart=NULL;
	uintptr_t ifps;//InterfacePointerSize. The size of the pointers. Used for incrementing/decrementing and thus wrapping through the lists.
			//In most cases not really needed. The compiler actually ensures, that increments are done with the right size.
			//If you do a +1 on a (char *) on a 64-Bit System, than the (char *) has a sizeof 8-Byte and thus the compiler
			//calculates the increment of +1 as +8
	err = getInterfacesAndAdresses(&(ifcollect.ifc),&ifps,&(ifcollect.ifacestart),&(ifcollect.ifmacstart),&(ifcollect.ifaddrstart));
	struct AbsintInterfacesWLAN wifcollect;
	wifcollect.wlanc=0;
	wifcollect.wlanidx=NULL;
	select_AI_wlan_dev(&wifcollect,&ifcollect,sktctr,cmd);

	char* ai_wlan_dev_namep[2];
	unsigned int ai_wlan_dev_idx[2];
	/* Outside of a channel-switch event the variable
	 * AI_WLAN_SWITCH_DEV_NEW holds the currently used WLAN-Dev
	 * During an channel-switch process this one - the until now used Dev -
	 * gets written into AI_WLAN_SWITCH_DEV_OLD
	 * The other one from now on stands into AI_WLAN_SWITCH_DEV_NEW. The AI
	 * switches to this Dev and we start over
	 */

	/*
	ifcollect.ifpeerstart = malloc(ifc*sizeof(char *));
	*(ifcollect.ifpeerstart)=malloc(ifc*sizeof(struct in_addr));
	memset(*(ifcollect.ifpeerstart),0,ifc*sizeof(struct in_addr));
	for(i=1;i<ifc;i++){
		*((ifcollect.ifpeerstart)+i)=*((ifcollect.ifpeerstart)+(i-1))+(uintptr_t)(sizeof(struct in_addr));
	}
	*/

	err = getPortPeer(&ifcollect);

	/*Now, after we do have the whole Interface Information:
	* Konzept: Synchronisierung via Semaphores (Blocking) mit je einem Server bzw. ClientListening Thread im Hintergrund für
	*  - Verbindung mit anderen AI
	*  - Verbindung mit Controller
	* Für den Moment: Annahme, dass nur ein AI Partner pro AI (also immer Paare)
	* Kann erweitert werden auf unbestimmte Partner Anzahl
	* Bsp: eth0-eth3&wlan0 Verbunden mit AI Partner 1; wlan1-wlan2,eth4 verbunden mit AI Partner 2.
	*
	* Nachrichtenaustausch der Threads via Shared Memory:
	* Eintrag 1 enthält Nachrichtenlänge
	* Eintrag 2 enthält Nachrichtenherkunft (AI oder Controller Thread)
	* Eintrag 3: DirtyChar. Zeigt, ob letzte Nachricht von Main ausgelesen wurde.
	* 			Threads schreiben '1' rein, wenn Nachricht geschrieben.
	* 			Main schreibt '0' rein, wenn Nachricht ausgelesen wurde.
	* 			Threads überschreiben ShMem nur, wenn DirtyChar == '0';
	*
	* Main Thread hier hängt an Sem1 geblockt solange nichts zu tun ist.
	* Wenn ein Thread etwas empfängt reserviert er Sem2, prüft DirtyChar und beschreibt ShMem, gibt Sem2 wieder frei.
	* Nach Schreiben einer Nachricht signalisiert ein Thread Sem1 und läuft selbst wieder blocked gegen sein Socket.
	* Main läuft nun los und will Sem2 für ShMem Zugriff
	* Main rennt einmal durch ihren Kram, gibt Sem2 wieder frei, setzt Sem1 runter und springt hoch, zurück vor Sem1 Anforderung
	* Dort wird Main wieder geblockt.
	*
	*/

	ABSINT_INTER_THREAD_COM_SHMEM_AND_SYNCH_SETUP;


	struct in_addr aipaddress;

//	AIPAddrSrcSwitchAgainWithCfg:
	switch(aiconnect.AIPartnerAddrSrc){
	case INTER_AI_ADDR_SRC_AUTO:
		InterAIConnectAuto:
		memcpy(&aipaddress,*(ifcollect.neighbours.dnstart),sizeof(struct in_addr));
		break;
	case INTER_AI_ADDR_SRC_CFG:
		printf("    I use the AI Partner Address Source like specified in the ");
		ANSICOLORSET(ANSI_COLOR_GREEN);
		printf("CFG-File");
		ANSICOLORRESET;
		printf(".\n");
		aipaddress=aipaddresscfg;
//		goto AIPAddrSrcSwitchAgainWithCfg;
		break;
	case INTER_AI_ADDR_SRC_CMDLINE:
		inet_pton(AF_INET,*argstart,&aipaddress);
		break;
	default:
		ANSICOLORSET(ANSI_COLOR_YELLOW);
		printf("WARNING:");
		ANSICOLORRESET;
		printf(" Couldn't properly switch the Source for the AI Partner Address.\n");
		printf("\tHence using the automatic Approach. Trying to Resolve the Destination Address by myself.\n");
		goto InterAIConnectAuto;
		break;
	}

	AICONNECTSWITCHAGAINWITHCFG:
	switch(aiconnect.clientOrServer){
	case INTER_AI_CLSERV_CFG:
		printf("    I use the Inter AI Connection Mode from the ");
		ANSICOLORSET(ANSI_COLOR_GREEN);
		printf("CFG-File");
		ANSICOLORRESET;
		printf(".\n");
		aiconnect.clientOrServer=aiconnectmodecfg.clientOrServer;
		goto AICONNECTSWITCHAGAINWITHCFG;
		break;
	case INTER_AI_CLSERV_AUTO:
		//Do this automatic detection:
		//Try to connect to an AI Server
		//If there is one and you could connect: nice, we finished
		//If there is no server: Set one up.
		//Does only properly work if the AI start temporary separated.
		//On concurrent startup of AIs this very probably fails.
		printf("<-------------------------------------->\n");
		printf("<-- Doing Automatic Connection Mode Detection\n");
		printf("<-------------------------------------->\n");
		break;
	case INTER_AI_CLSERV_SERVER://Set up a Server for other AIs to connect to
		printf("<-------------------------------------->\n");
		printf("<-- Starting AI Communication Server\n");
		printf("<-------------------------------------->\n");
		ABSINT_INET_SOCKET_SERVER_START_THREAD;
		break;
	case INTER_AI_CLSERV_CLIENT://Connect to a Server
		printf("<-------------------------------------->\n");
		printf("<-- Working as AI Client - Connecting to AI Server\n");
		printf("<-------------------------------------->\n");
		ABSINT_INET_SOCKET_CLIENT_START_THREAD;
		break;
	default:
		ANSICOLORSET(ANSI_COLOR_YELLOW);
		printf("WARNING:");
		ANSICOLORRESET;
		printf(" Couldn't properly switch the Inter AI Connection Mode.\n");
		printf("\tHence using the automatic Approach.\n");
		break;
	}
	ANSICOLORSET(ANSI_COLOR_YELLOW);
	printf("NOTE: ");
	ANSICOLORRESET;
	printf("Currently there is no Inter-AI-Communication/Connection.\n\tIt's just prepared for future needs, if any.\n\n");

//	ABSINT_THREAD_START_STATION_MONITOR;

	ABSINT_INET_SOCKET_CONTROLLER_COM_START_THREAD;

	if(wifcollect.wlanc>=2){
		for(i=0;i<(wifcollect.wlanc);i++){
			WPA_SUPPLICANT_CONF_SETUP((ifcollect.ifacestart)[(wifcollect.wlanidx)[i]]);
		}

		AI_WLAN_SWITCH_DEV_NAMEP_NEW=(ifcollect.ifacestart)[(wifcollect.wlanidx)[0]];
		AI_WLAN_SWITCH_DEV_NAMEP_OLD=(ifcollect.ifacestart)[(wifcollect.wlanidx)[1]];
	}

	/* Parse die Argumente nach "wlandevs" nacheinander.
	 * Prüfe dabei immer ob das aktuelle Argument ein neues Kommandozeilen-Argument ist
	 * Falls nicht: Es ist ein weiteres WLAN-Device.
	 * ---------------------------------
	 * For now just quick and easy: Read out and finished.
	 * If there comes an extension with additional cmd-line arguments, then do it nice, like mentioned above
	 * and exclude the read wlandevs from the argstart array.
	 */
	for(i=0;i<argc;i++){
		if(strcmp(argstart[i],"wlandevs")==0){
			if(argc-(i+1)<2){
				printfc(red,"ERROR");printf(": Passed to few WLAN-Devices after wlandevs!\n");
				exit(1);
			}
			int j;
			i++;
			for(j=0;j<wifcollect.wlanc;j++){
				if(strcmp(argstart[i],(ifcollect.ifacestart)[(wifcollect.wlanidx)[j]])==0){
					AI_WLAN_SWITCH_DEV_NAMEP_NEW=(ifcollect.ifacestart)[(wifcollect.wlanidx)[j]];
					goto FirstPassedWLANDevExists;
				}
			}
			printfc(red,"ERROR");printf(": The first passed WLAN-Device after \"wlandevs\" doesn't exist!\n");
			exit(1);
			FirstPassedWLANDevExists:
			i++;
			for(j=0;j<wifcollect.wlanc;j++){
				if(strcmp(argstart[i],(ifcollect.ifacestart)[(wifcollect.wlanidx)[j]])==0){
					AI_WLAN_SWITCH_DEV_NAMEP_OLD=(ifcollect.ifacestart)[(wifcollect.wlanidx)[j]];
					goto SecondPassedWLANDevExists;
				}
			}
			printfc(red,"ERROR");printf(": The second passed WLAN-Device after \"wlandevs\" doesn't exist!\n");
			exit(1);
			SecondPassedWLANDevExists:
			break;
		}
	}

	if(wifcollect.wlanc<2){
		printfc(red,"ERROR");printf(": Not enough WLAN-Devices existent! EXITING!!!\n");
		exit(NETWORK_ERR_RARE_INTERFACES);
	}

	AI_WLAN_SWITCH_DEV_IDX_NEW=if_nametoindex(AI_WLAN_SWITCH_DEV_NAMEP_NEW);
	AI_WLAN_SWITCH_DEV_IDX_OLD=if_nametoindex(AI_WLAN_SWITCH_DEV_NAMEP_OLD);

	printf("NOTE: Creating >>Seamless Handover WLAN-Connection<< with Devices:\n");
	printf("\t%s (Index %d)\n\t%s (Index %d)\n",AI_WLAN_SWITCH_DEV_NAMEP_NEW,AI_WLAN_SWITCH_DEV_IDX_NEW,AI_WLAN_SWITCH_DEV_NAMEP_OLD,AI_WLAN_SWITCH_DEV_IDX_OLD);

	//Resetting Device (Getting sure, that it is 'up')
	err=secure_device_is_up(AI_WLAN_SWITCH_DEV_NAMEP_NEW);
	err=secure_device_is_up(AI_WLAN_SWITCH_DEV_NAMEP_OLD);
	//(Getting sure, that no wpa_supplicant is currently running; i.e. stop, if any)
	stop_adhoc_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_NEW,check_running_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_NEW));
	stop_adhoc_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_OLD,check_running_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_OLD));

	ABSINT_THREAD_START_WLAN_MONITOR(AI_WLAN_SWITCH_DEV_NAMEP_OLD);


//	err=absint_establish(ifcollect.ifacestart,0);
//wpa_supplicant -B -i sta1-wlan0 -c /media/sf_AbstractInterface/ollerus/Debug/wlan/absint-wpa_supplicant-adhoc.conf -D nl80211,wext
//ifconfig s1-eth1 10.0.1.100/24 netmask 255.255.255.0 broadcast 10.0.1.255

	/*
	 * After all the Setups, Initializations and Thread Starting
	 * the Main doesn't have anything to do, until the first message arrives
	 * (Thats at least the Assumption at the current Design-Phase ;oP )
	 * So the Flow looks like
	 * sem_mainSynch init with 0 (don't allow to use it at Beginning)
	 * JumpMarker:
	 * sem_wait(mainSynch)
	 * sem_wait(ShMem)
	 * Do Message-Mux and Operations
	 * sem_signal(ShMem)
	 * goto JumpMarker
	 */

	NextRunThrough:
	sem_wait(&sem_mainsynch);
	sem_wait(&(shmem.sem_shmem));
	/* Critical Section
	 * Shared Memory Access, Message-Mux
	 */
	if(shmem.flags & FLAG_ABSINT_COMTHR_SHMEM_SEND){//Shouldn't be able to occur
		sem_post(&(shmem.sem_shmem));
		goto NextRunThrough;
	}else {
	}

	//TODO: Maybe something else, than the simple skip of a message, if the delivered device doesn't exist
	//TODO: If the Device isn't present: Send the controller a msg.
	//NOTE: Here isn't any "received" Device used. This was just an early idea and later on changed
		//The Device-Choice is taken by the AI itself (In interchange, see this DEV_..._NEW and DEV_..._OLD stuff)
	switch(shmem.type){
		char devrecv[IFNAMSIZ];
		char *temp;
			#define FSMMSGID (*((msgswitchid *)(shmem.ShMem)))
			#define PAFTFSMMSGID (((char *)(shmem.ShMem))+sizeof(msgswitchid))
	case AIC_CTAI_START_ADHOC:
		if((shmem.msgsize)==0){
			memset(devrecv,0,sizeof(devrecv));
			strcpy(devrecv,AI_WLAN_SWITCH_DEV_NAMEP_NEW);
		}else{
			memcpy(devrecv,shmem.ShMem,(sizeof(devrecv)<(shmem.msgsize))?sizeof(devrecv):(shmem.msgsize));
			devrecv[(shmem.msgsize)]='\0';
			if(!check_if_dev_exists(devrecv,&ifcollect)){
				printfc(yellow,"-> WARNING: ");printf("Received Msg with dev, which doesn't exist on this Platform.\n\t\"%s\"\tIgnoring Msg START-ADHOC...\n",devrecv);
				break;
			}
		}
		start_adhoc_wpa_supplicant(devrecv,check_running_wpa_supplicant(devrecv));
		err=get_freq_ssid(devrecv,&currentfreq,currentssid);
		break;
	case AIC_CTAI_STOP_ADHOC:
		if((shmem.msgsize)==0){
			memset(devrecv,0,sizeof(devrecv));
			strcpy(devrecv,AI_WLAN_SWITCH_DEV_NAMEP_NEW);
		}else{
			memcpy(devrecv,shmem.ShMem,(sizeof(devrecv)<(shmem.msgsize))?sizeof(devrecv):(shmem.msgsize));
			devrecv[(shmem.msgsize)]='\0';
			if(!check_if_dev_exists(devrecv,&ifcollect)){
				printfc(yellow,"-> WARNING: ");printf("Received Msg with dev, which doesn't exist on this Platform.\n\t\"%s\"\tIgnoring Msg STOP-ADHOC...\n",devrecv);
				break;
			}
		}
		stop_adhoc_wpa_supplicant(devrecv,check_running_wpa_supplicant(devrecv));
		break;
	case AIC_CTAI_SET_ADHOC_ESSID:
		;
		char ssidrecv[33];
		i=0;
		while((shmem.ShMem)[i]!='\0'){
			if(i>shmem.msgsize){//Ugh, this is bad
				goto FreeShMem;
			}
			ssidrecv[i]=shmem.ShMem[i];
			i++;
		}
		i++;
		if(i==shmem.msgsize){
			//No further information, i.e. no device passed
			//-> Take the Channel-Switch currently sending device
			memset(devrecv,0,sizeof(devrecv));
			strcpy(devrecv,AI_WLAN_SWITCH_DEV_NAMEP_NEW);
		}else{
			i++;
			if(i>shmem.msgsize){//Ugh, this is bad
				goto FreeShMem;
			}
			memcpy(devrecv,(shmem.ShMem)+i,(shmem.msgsize)-i);
			devrecv[(shmem.msgsize)-i]=0;
			if(!check_if_dev_exists(devrecv,&ifcollect)){
				printfc(yellow,"-> WARNING: ");printf("Received Msg with dev, which doesn't exist on this Platform.\n\t\"%s\"\tIgnoring Msg SET-ESSID...\n",devrecv);
				break;
			}
		}
		#ifdef DEVELOPMENT_MODE
			printfc(yellow,"Received ");printf("AIC_CTAI_SET_ADHOC_ESSID\n");
			printf("SSID: %s| Device: %s\n",ssidrecv,devrecv);
		#endif
		set_adhoc_essid_wpa_supplicant(devrecv,ssidrecv,check_running_wpa_supplicant(devrecv));
		err=get_freq_ssid(devrecv,&currentfreq,currentssid);
		break;
	case AIC_CTAI_SET_ADHOC_FREQ:
		;
		int freq=(int)(*((msgfreq*)(shmem.ShMem)));
		if((shmem.msgsize)==sizeof(msgfreq)){
			//No device delivered: Take FSM Device
			memset(devrecv,0,sizeof(devrecv));
			strcpy(devrecv,AI_WLAN_SWITCH_DEV_NAMEP_NEW);
		}else{
	//		memset(devrecv,0,sizeof(devrecv));
			memcpy(devrecv,(char*)((msgfreq*)(shmem.ShMem)+1),(sizeof(devrecv)<((shmem.msgsize)-sizeof(msgfreq)))?sizeof(devrecv):((shmem.msgsize)-sizeof(msgfreq)));
			devrecv[(shmem.msgsize)-sizeof(msgfreq)]='\0';
	//		depr(1,"freq %d | devrecv %s",freq,devrecv);
			if(!check_if_dev_exists(devrecv,&ifcollect)){
				printfc(yellow,"-> WARNING: ");printf("Received Msg with dev, which doesn't exist on this Platform.\n\t\"%s\"\tIgnoring Msg SET-FREQ...\n",devrecv);
				break;
			}
		}
		err=check_running_wpa_supplicant(devrecv);
		set_adhoc_freq_wpa_supplicant(devrecv,freq,err);
		err=get_freq_ssid(devrecv,&currentfreq,currentssid);
		break;
	case AIC_CTAI_INQUIRE_IFACE:
//		free(shmem.ShMem);
//		shmem.ShMem=NULL;
		shmem.flags=FLAG_ABSINT_COMTHR_SHMEM_NEW|FLAG_ABSINT_COMTHR_SHMEM_SEND;
//		shmem.msgsize=0;
//		shmem.type=AIC_CTAI_INQUIRE_IFACE; //Just let it stay like it already is
		sem_post(&sem_send);
		sem_post(&(shmem.sem_shmem));
		goto NextRunThrough;
		break;
	case AIC_CTAI_INQUIRE_RSSI:
//		free(shmem.ShMem);
//		shmem.ShMem=NULL;
		shmem.flags=FLAG_ABSINT_COMTHR_SHMEM_NEW|FLAG_ABSINT_COMTHR_SHMEM_SEND;
//		shmem.msgsize=0;
//		shmem.type=AIC_CTAI_INQUIRE_RSSI; //Just let it stay like it already is
		sem_post(&sem_send);
		sem_post(&(shmem.sem_shmem));
		goto NextRunThrough;
		break;
	case AIC_WLAN_SWITCH_FSM_CTAI_START:
		//switchfsm.state=CHANSWITCH_FSM_STATE_ENTER;
		switchfsm.instanceid=*((msgswitchid *)(shmem.ShMem));//The ID, delivered from the Controller
		int newfreq;
		newfreq=*((msgfreq *)((msgswitchid *)(shmem.ShMem)+1));
		err=shmem.msgsize-sizeof(msgswitchid)-sizeof(msgfreq);
		if(err<=0){
			printfc(red,"ERROR");printf(": Delivered ChanSwitchStart-Msg without SSID. Very bad...\n\tHence ignoring this completely...\n");
			break;
		}
		temp=malloc(err+1);
		memcpy(temp,((char *)((msgfreq *)((msgswitchid *)(shmem.ShMem)+1)+1)),err);
		temp[err]='\0';
		if(shmem.ShMem){
			free(shmem.ShMem);
			shmem.ShMem=NULL;
		}
		AI_WLAN_SWITCH_DEV_EXCHANGE
		/* From now on, the two variables
		 * AI_WLAN_SWITCH_DEV_NAMEP_NEW &
		 * AI_WLAN_SWITCH_DEV_IDX_NEW
		 * are holding the WLAN-Device to which we want to switch
		 */
		//Directly prepare the next msg passing to the sending thread, instead of first cleaning the Shmem
		shmem.ShMem=malloc(sizeof(msgswitchid)+sizeof(unsigned int));
		shmem.flags=FLAG_ABSINT_COMTHR_SHMEM_NEW | FLAG_ABSINT_COMTHR_SHMEM_SEND;
		shmem.msgsize=sizeof(msgswitchid)+sizeof(unsigned int);
		*((msgswitchid *)(shmem.ShMem))=switchfsm.instanceid;
		*((unsigned int *)(((msgswitchid *)(shmem.ShMem))+1))=AI_WLAN_SWITCH_DEV_IDX_NEW;
		shmem.type=AIC_WLAN_SWITCH_FSM_AITC_READY_TO_SWITCH;
		ABSINT_THREAD_STOP_WLAN_MONITOR;
		printfc(red,"ERROR FROM JOIN: %d\n",err);
//		raise(SIGKILL_WLANMON);
//		pthread_join(sigkill_wlanmon_stuff.thread_wlan_mon,NULL);
		err=check_running_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_NEW);
		set_adhoc_freq_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_NEW,newfreq,err);
		set_adhoc_essid_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_NEW,temp,err);
		start_adhoc_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_NEW,err);
		free(temp);
		sem_post(&sem_send);
		sem_post(&(shmem.sem_shmem));
		currentTimeoutID++;
		ABSINT_THREAD_START_TIMEOUT(ABSINT_SWITCH_MODE_TIMEOUT_S,ABSINT_SWITCH_MODE_TIMEOUT_NS,AIC_INTERNAI_TIMEOUT_R,currentTimeoutID);
		switchfsm.state=CHANSWITCH_FSM_STATE_READY_TO_SWITCH;
			#ifdef DEVELOPMENT_MODE
		printfc(blue,"\tState");printf(": ReadyToSwitch\n");
			#endif
		goto NextRunThrough;
		break;
	case AIC_WLAN_SWITCH_FSM_CTAI_SWITCH_DONE:
		if((switchfsm.state!=CHANSWITCH_FSM_STATE_READY_TO_SWITCH)&&(switchfsm.state!=CHANSWITCH_FSM_STATE_READY_TIMEOUT)){
			break;
		}
		if(switchfsm.instanceid!=FSMMSGID){
			break;
		}
		if(shmem.ShMem){
			free(shmem.ShMem);
			shmem.ShMem=NULL;
		}
		shmem.ShMem=malloc(sizeof(msgswitchid));
		shmem.flags=FLAG_ABSINT_COMTHR_SHMEM_NEW | FLAG_ABSINT_COMTHR_SHMEM_SEND;
		shmem.msgsize=sizeof(msgswitchid);
		*((msgswitchid *)(shmem.ShMem))=switchfsm.instanceid;
		shmem.type=AIC_WLAN_SWITCH_FSM_AITC_SWITCH_DONE_ACK;
		sem_post(&sem_send);
		sem_post(&(shmem.sem_shmem));
		currentTimeoutID++;
		ABSINT_THREAD_START_TIMEOUT(ABSINT_SWITCH_MODE_TIMEOUT_S,ABSINT_SWITCH_MODE_TIMEOUT_NS,AIC_INTERNAI_TIMEOUT_D,currentTimeoutID);
		switchfsm.state=CHANSWITCH_FSM_STATE_SWITCH_DONE;
			#ifdef DEVELOPMENT_MODE
		printfc(blue,"\tState");printf(": SwitchDone\n");
			#endif
		goto NextRunThrough;
		break;
	case AIC_WLAN_SWITCH_FSM_CTAI_FINISH:
		if(switchfsm.state!=CHANSWITCH_FSM_STATE_SWITCH_DONE){
			if(switchfsm.state!=CHANSWITCH_FSM_STATE_DONE_TIMEOUT){
				break;
			}
		}
		if(switchfsm.instanceid!=FSMMSGID){
			break;
		}
		if(shmem.ShMem){
			free(shmem.ShMem);
			shmem.ShMem=NULL;
		}
		stop_adhoc_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_OLD,check_running_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_OLD));
		shmem.ShMem=malloc(sizeof(msgswitchid));
		shmem.flags=FLAG_ABSINT_COMTHR_SHMEM_NEW | FLAG_ABSINT_COMTHR_SHMEM_SEND;
		shmem.msgsize=sizeof(msgswitchid);
		*((msgswitchid *)(shmem.ShMem))=switchfsm.instanceid;
		shmem.type=AIC_WLAN_SWITCH_FSM_AITC_FINISH_ACK;
		sem_post(&sem_send);
		sem_post(&(shmem.sem_shmem));
		ABSINT_THREAD_START_WLAN_MONITOR(AI_WLAN_SWITCH_DEV_NAMEP_OLD);
		err=get_freq_ssid(AI_WLAN_SWITCH_DEV_NAMEP_NEW,&currentfreq,currentssid);
		switchfsm.state=CHANSWITCH_FSM_STATE_NONE;
		break;
	case AIC_WLAN_SWITCH_FSM_CTAI_STAY_READY:
		if(switchfsm.state!=CHANSWITCH_FSM_STATE_READY_TIMEOUT){
			break;
		}
		if(switchfsm.instanceid!=FSMMSGID){
			break;
		}
		if(shmem.ShMem){
			free(shmem.ShMem);
			shmem.ShMem=NULL;
		}
		shmem.ShMem=malloc(sizeof(msgswitchid));
		shmem.flags=FLAG_ABSINT_COMTHR_SHMEM_NEW | FLAG_ABSINT_COMTHR_SHMEM_SEND;
		shmem.msgsize=sizeof(msgswitchid);
		*((msgswitchid *)(shmem.ShMem))=switchfsm.instanceid;
		shmem.type=AIC_WLAN_SWITCH_FSM_AITC_STAY_READY_ACK;
		sem_post(&sem_send);
		sem_post(&(shmem.sem_shmem));
		currentTimeoutID++;
		ABSINT_THREAD_START_TIMEOUT(ABSINT_SWITCH_MODE_TIMEOUT_S,ABSINT_SWITCH_MODE_TIMEOUT_NS,AIC_INTERNAI_TIMEOUT_R,currentTimeoutID);
		switchfsm.state=CHANSWITCH_FSM_STATE_READY_TO_SWITCH;
			#ifdef DEVELOPMENT_MODE
		printfc(blue,"\tState");printf(": ReadyToSwitch\n");
			#endif
		goto NextRunThrough;
		break;
	case AIC_WLAN_SWITCH_FSM_CTAI_STAY_DONE:
		if(switchfsm.state!=CHANSWITCH_FSM_STATE_DONE_TIMEOUT){
			break;
		}
		if(switchfsm.instanceid!=FSMMSGID){
			break;
		}
		if(shmem.ShMem){
			free(shmem.ShMem);
			shmem.ShMem=NULL;
		}
		shmem.ShMem=malloc(sizeof(msgswitchid));
		shmem.flags=FLAG_ABSINT_COMTHR_SHMEM_NEW | FLAG_ABSINT_COMTHR_SHMEM_SEND;
		shmem.msgsize=sizeof(msgswitchid);
		*((msgswitchid *)(shmem.ShMem))=switchfsm.instanceid;
		shmem.type=AIC_WLAN_SWITCH_FSM_AITC_STAY_DONE_ACK;
		sem_post(&sem_send);
		sem_post(&(shmem.sem_shmem));
		currentTimeoutID++;
		ABSINT_THREAD_START_TIMEOUT(ABSINT_SWITCH_MODE_TIMEOUT_S,ABSINT_SWITCH_MODE_TIMEOUT_NS,AIC_INTERNAI_TIMEOUT_D,currentTimeoutID);
		switchfsm.state=CHANSWITCH_FSM_STATE_SWITCH_DONE;
			#ifdef DEVELOPMENT_MODE
		printfc(blue,"\tState");printf(": SwitchDone\n");
			#endif
		goto NextRunThrough;
		break;
	case AIC_INTERNAI_TIMEOUT_R:
		if(switchfsm.state!=CHANSWITCH_FSM_STATE_READY_TO_SWITCH){
			break;
		}
		if(currentTimeoutID!=shmem.msgsize){
			break;
		}
		shmem.ShMem=malloc(sizeof(msgswitchid));
		shmem.flags=FLAG_ABSINT_COMTHR_SHMEM_NEW | FLAG_ABSINT_COMTHR_SHMEM_SEND;
		shmem.msgsize=sizeof(msgswitchid);
		*((msgswitchid *)(shmem.ShMem))=switchfsm.instanceid;
		shmem.type=AIC_WLAN_SWITCH_FSM_AITC_READY_TIMEOUT;
		sem_post(&sem_send);
		sem_post(&(shmem.sem_shmem));
		currentTimeoutID++;
		ABSINT_THREAD_START_TIMEOUT(2*ABSINT_SWITCH_MODE_TIMEOUT_S,2*ABSINT_SWITCH_MODE_TIMEOUT_NS,AIC_INTERNAI_TIMEOUT_2nd,currentTimeoutID);
		switchfsm.state=CHANSWITCH_FSM_STATE_READY_TIMEOUT;
			#ifdef DEVELOPMENT_MODE
		printfc(blue,"\tState");printf(": ReadyTimeout\n");
			#endif
		goto NextRunThrough;
		break;
	case AIC_INTERNAI_TIMEOUT_D:
		if(switchfsm.state!=CHANSWITCH_FSM_STATE_SWITCH_DONE){
			break;
		}
		if(currentTimeoutID!=shmem.msgsize){
			break;
		}
		shmem.ShMem=malloc(sizeof(msgswitchid));
		shmem.flags=FLAG_ABSINT_COMTHR_SHMEM_NEW | FLAG_ABSINT_COMTHR_SHMEM_SEND;
		shmem.msgsize=sizeof(msgswitchid);
		*((msgswitchid *)(shmem.ShMem))=switchfsm.instanceid;
		shmem.type=AIC_WLAN_SWITCH_FSM_AITC_DONE_TIMEOUT;
		sem_post(&sem_send);
		sem_post(&(shmem.sem_shmem));
		currentTimeoutID++;
		ABSINT_THREAD_START_TIMEOUT(2*ABSINT_SWITCH_MODE_TIMEOUT_S,2*ABSINT_SWITCH_MODE_TIMEOUT_NS,AIC_INTERNAI_TIMEOUT_2nd,currentTimeoutID);
		switchfsm.state=CHANSWITCH_FSM_STATE_DONE_TIMEOUT;
			#ifdef DEVELOPMENT_MODE
		printfc(blue,"\tState");printf(": DoneTimeout\n");
			#endif
		goto NextRunThrough;
		break;
	case AIC_INTERNAI_TIMEOUT_2nd:
		switch(switchfsm.state){
						#define FinalTimeoutHandler \
							stop_adhoc_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_NEW,check_running_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_NEW)); \
							ABSINT_THREAD_START_WLAN_MONITOR(AI_WLAN_SWITCH_DEV_NAMEP_NEW); \
							AI_WLAN_SWITCH_DEV_EXCHANGE
		case CHANSWITCH_FSM_STATE_READY_TIMEOUT:
			if(currentTimeoutID!=shmem.msgsize){
				goto FreeShMem;
			}
			shmem.ShMem=malloc(sizeof(msgswitchid));
			shmem.flags=FLAG_ABSINT_COMTHR_SHMEM_NEW | FLAG_ABSINT_COMTHR_SHMEM_SEND;
			shmem.msgsize=sizeof(msgswitchid);
			*((msgswitchid *)(shmem.ShMem))=switchfsm.instanceid;
			shmem.type=AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_R;
			sem_post(&sem_send);
			sem_post(&(shmem.sem_shmem));
			FinalTimeoutHandler
			err=get_freq_ssid(AI_WLAN_SWITCH_DEV_NAMEP_NEW,&currentfreq,currentssid);
			break;
		case CHANSWITCH_FSM_STATE_DONE_TIMEOUT:
			if(currentTimeoutID!=shmem.msgsize){
				goto FreeShMem;
			}
			shmem.ShMem=malloc(sizeof(msgswitchid));
			shmem.flags=FLAG_ABSINT_COMTHR_SHMEM_NEW | FLAG_ABSINT_COMTHR_SHMEM_SEND;
			shmem.msgsize=sizeof(msgswitchid);
			*((msgswitchid *)(shmem.ShMem))=switchfsm.instanceid;
			shmem.type=AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_D;
			sem_post(&sem_send);
			sem_post(&(shmem.sem_shmem));
			FinalTimeoutHandler
			err=get_freq_ssid(AI_WLAN_SWITCH_DEV_NAMEP_NEW,&currentfreq,currentssid);
			break;
		default:
			goto FreeShMem;
			break;
						#undef FinalTimeoutHandler
		}
		switchfsm.state=CHANSWITCH_FSM_STATE_NONE;
			#ifdef DEVELOPMENT_MODE
		printfc(red,"\tState");printf(": NONE\n");printf("\tExiting SwitchMode due to 2nd Timeout!!!\n");
			#endif
		err=get_freq_ssid(AI_WLAN_SWITCH_DEV_NAMEP_NEW,&currentfreq,currentssid);
		goto NextRunThrough;
		break;
	default:
		break;
			#undef FSMMSGID
			#undef PAFTFSMMSGID
	}
	FreeShMem:

	if(shmem.ShMem){
		free(shmem.ShMem);
		shmem.ShMem=NULL;
	}
	shmem.flags=0;
	shmem.msgsize=0;
	shmem.type=0;
	sem_post(&(shmem.sem_shmem));
	goto NextRunThrough;

	pthread_exit(NULL);
	return err;
}



int absint_auto_autonom(int argc, char **argstart,struct nl80211_state *sktctr,struct CommandContainer *cmd,struct aiconnectmode aiconnect){
	int err = 0;
	printf("\nStarting Abstract Interface...\n");
	printf("Going into >>non-stop operation<< with PHYsical Interface Switch ");
	ANSICOLORSET(ANSI_COLOR_RED);
	printf("Mode");
	ANSICOLORRESET;
	printf(":\n");
	printf("\tDecision ");
	ANSICOLORSET(ANSI_COLOR_RED);
	printf("autonomous");
	ANSICOLORRESET;
	printf(" on AI (AI doesn't communicate with SDN-Controller)\n");
	return err;
}



int absint(int argc, char **argstart,struct nl80211_state *sktctr,struct CommandContainer *cmd){
	system("clear");
	#ifndef MPTCP_OFF
	system("sudo sysctl net.mptcp.mptcp_path_manager=fullmesh");
	#endif
	printf("\n>*< Abstract Interface called >*<\n");
	//The Arguments are here BEHIND the "absint"
	int err = 0;
	int loopcnt;
	struct aiconnectmode aicm;
	aicm.clientOrServer=INTER_AI_CLSERV_AUTO;
	aicm.AIPartnerAddrSrc=INTER_AI_ADDR_SRC_AUTO;
	switch (argc) {
	case 0:
	//Do here the "non-stop operation" ("Dauerbetrieb")
	//Do the "non-stop operation" also on the corresponding string compare down there
		printf("\nYou didn't gave me any Argument more after the \"absint\"\nThis means that i'll go into >>non-stop operation<<\n\tMode: SDN-Controller-driven\n");
		err=absint_auto_controller(argc,argstart,sktctr,cmd,aicm);
		return err;
		break;
	default:
		if (argc>5){
			printf("\nHm, you maybe passed to many Arguments for the Abstract Interface...\nNevertheless, i'll continue with the few, that i'm able to process.\n\t(That are 3 more after the \"absint\", except the \"useports\" command, which can handle unlimited)\nThat means i use the: %s %s\nand bypass the:",*argstart,*(argstart+1));
			for(loopcnt=2;loopcnt<argc;loopcnt++){
				printf(" %s",*(argstart+loopcnt));
			}
			printf("\n\n");
		} else if (argc<0) {
			//then the passed Number of Arguments is negative. Mustn't occur, How could it? Really bad...
			printf("\nNegative Number of Command Line Arguments in Abstract Interface! o.O\nHow could this occur?\nGratulation, you obviously found a Bug -.-\n\n");
			return MAIN_ERR_BAD_CMDLINE;
		}//No else here. Cause this means we have the right Number of args (and expect they are valid ._.)
		//So no additional output necessary. Just progress further.
		break;
	}

	//Now do the Command Line Multiplex
	//Consider: Do the string compare for the "auto", this means the >>non-stop operation<<, at last.
	//This Way gives a liiittle bit runtime improvement for the "single Operation" Program Calls.
	//They are executed once, cause one single Run-Through the Program, with Program Termination.
	//Hence they should be executed immediately
	//For the >>non-stop operations<< a small delay at Startup won't carry weight
	//I mean, they are running like forever... Nobody hurries at such a Startup ;oP
		//REMEMBER: We secured with the above, that the Number of args is in the right range (Between 1-2).
		//But for specific Commands we could have tighter demands. More precisely:
		//For example: Some Commands need exactly 2 args (1 isn't enough). But we could still have only 1...
	if (strcmp(*(argstart), "establish") == 0){
		if (argc<2){
			printf("\nAbsInt: To few Arguments on \"establish\"\n\tWhich Interface shall i set up?\n");
			return MAIN_ERR_BAD_CMDLINE;
		} else {
			//The Operations to establish a new MPTCP Connection.
			if((argc>2)&&(strcmp(*(argstart+2), "hard") == 0)){//Additional to set up the interface by hardware (Bringing up an Interface from down state)
				err=absint_establish(argstart+1,1);
			}else{
				err=absint_establish(argstart+1,0);
			}
		}
	} else if (strcmp(*(argstart), "cut") == 0){
		if (argc<2){
			printf("\nAbsInt: To few Arguments on \"cut\"\n\tWhich Interface shall i cease?\n");
			return MAIN_ERR_BAD_CMDLINE;
		} else {
			//The Operations to destroy a MPTCP Connection on an Interface.
			if((argc>2)&&(strcmp(*(argstart+2), "hard") == 0)){//Additional to set up the interface by hardware (Bringing up an Interface from down state)
				err=absint_cut(argstart+1,1);
			}else{
				err=absint_cut(argstart+1,0);
			}
		}
	} else if (strcmp(*(argstart), "useports") == 0){
		if (argc<2){
			printf("\nAbsInt: To few Arguments on \"useports\"\n\tWhich Ports should be used for Communication from now on?\n");
			return MAIN_ERR_BAD_CMDLINE;
		} else {
			//From now on the Hardware-Plattform uses the Ports for communication, which are given as command-line prompts
			//The "switch" from the current used should work without interruption of maybe currently running transmissions
			unsigned int ifc;//The Number of all available Interfaces
			char **ifacestart;
			char **ifmacstart;
			char **ifaddrstart;
			uintptr_t ifps;//InterfacePointerSize. The size of the pointers. Used for incrementing/decrementing and thus wrapping through the lists.
					//In most cases not really needed. The compiler actually ensures, that increments are done with the right size.
					//If you do a +1 on a (char *) on a 64-Bit System, than the (char *) has a sizeof 8-Byte and thus the compiler
					//calculates the increment of +1 as +8
			err = getInterfacesAndAdresses(&ifc,&ifps,&ifacestart,&ifmacstart,&ifaddrstart);
			err=absint_useports(argc-1,argstart+1,ifc,ifacestart);
		}
	} else if (strcmp(*(argstart), "netman") == 0){
		//Network Management
		//The Device for the "netman" is passed (as common) BEFORE the absint
		//	like "ollerus dev INTERFACENAME absint ..."
		if (argc<2){
			printf("\nAbsInt: To few Arguments on \"netman\"\n\t<adhoc | >\n");
			return MAIN_ERR_BAD_CMDLINE;
		}
		argc--;
		argstart++;
		if (strcmp(*(argstart), "adhoc") == 0){
			if (argc<2){
				printf("\nAbsInt: To few Arguments on \"netman adhoc\"\n\t<start | set | >\n");
				return MAIN_ERR_BAD_CMDLINE;
			}
			argc--;
			argstart++;
			if (strcmp(*(argstart), "start") == 0){
				start_adhoc_wpa_supplicant(WhatWeWant.interfacename,check_running_wpa_supplicant(WhatWeWant.interfacename));
			}else if (strcmp(*(argstart), "set") == 0){
				if (argc<2){
					printf("\nAbsInt: To few Arguments on \"netman adhoc set\"\n\t<freq | chan | essid>\n");
					return MAIN_ERR_BAD_CMDLINE;
				}
				argc--;
				argstart++;
				if (strcmp(*(argstart), "freq") == 0){
					char *endpoint;
					int freq;
					freq = (int)strtol(*(argstart+1), &endpoint, 10);
					if (*endpoint){
						printf("\nAbsInt: Not a valid Number on \"netman adhoc set freq\"\n");
						return MAIN_ERR_BAD_CMDLINE;
					}
					set_adhoc_freq_wpa_supplicant(WhatWeWant.interfacename,freq,check_running_wpa_supplicant(WhatWeWant.interfacename));
				}else if (strcmp(*(argstart), "chan") == 0){
					char *endpoint;
					int chan;
					chan = (int)strtol(*(argstart+1), &endpoint, 10);
					if (*endpoint){
						printf("\nAbsInt: Not a valid Number on \"netman adhoc set chan\"\n");
						return MAIN_ERR_BAD_CMDLINE;
					}
					enum nl80211_band band;
					band = chan <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
					int freq;
					freq = ieee80211_channel_to_frequency(chan, band);
					set_adhoc_freq_wpa_supplicant(WhatWeWant.interfacename,freq,1);
				}else if (strcmp(*(argstart), "essid") == 0){
					set_adhoc_essid_wpa_supplicant(WhatWeWant.interfacename,*(argstart+1),check_running_wpa_supplicant(WhatWeWant.interfacename));
				}
			}
		} else{
			printf("\nAbsInt: Invalid Command on \"netman\"\n");
			return MAIN_ERR_BAD_CMDLINE;
		}
	} else if (strcmp(*(argstart), "auto") == 0){
		int argcpasstoabsint;
		char **argpasstoabsint;
		if (argc<2){
			printf("\nAbsInt: To few Arguments on \"auto\"\n\tIn which Decision Mode am i supposed to start the\n\t>>non-stop operation<< of the Abstract Interface?\n");
			printf("Supported Modes:\n");
			printf("auto controller - The SDN-Controller takes the Decision of the used PHYsical Interfaces / to switch the currently used Interfaces\n");
			printf("auto autonom - The AI takes the Decision by itself and doesn't communicate with a controller (at least for this purpose... Let's see...\n");
			return MAIN_ERR_BAD_CMDLINE;
		} else {
			argcpasstoabsint=argc-1;
			argpasstoabsint=argstart+1;
			aicm.clientOrServer=INTER_AI_CLSERV_CFG;
			aicm.AIPartnerAddrSrc=INTER_AI_ADDR_SRC_CFG;
			/* First Switch the additional Command Line Arguments
			 * for the Connection between two AI's
			 * - Witch one works as Server / Client
			 * - How to get the Destination Address to connect to, if i am a Client
			 * REMARK: This Information can also be stored in the config-File
			 * But if you start the program with matching command lines, then the
			 * command line arguments have higher priority
			 * Additional you are able to explicitly give the command line "cfg"
			 * to force a read-out from the cfg-File (however, the effect is the same
			 * like giving not any argument for this...)
			 */
			//First: The one to determine if i am a server or client
			if(argc>2){
				if (strcmp(*(argstart+2), "cfg") == 0){
					aicm.clientOrServer=INTER_AI_CLSERV_CFG;
				} else if (strcmp(*(argstart+2), "auto") == 0){
					aicm.clientOrServer=INTER_AI_CLSERV_AUTO;
				} else if (strcmp(*(argstart+2), "server") == 0){
					aicm.clientOrServer=INTER_AI_CLSERV_SERVER;
				} else if (strcmp(*(argstart+2), "client") == 0){
					aicm.clientOrServer=INTER_AI_CLSERV_CLIENT;
				} else{
					aicm.clientOrServer=INTER_AI_CLSERV_CFG;
				}
			}
			/* Then: From where to get the Destination Address
			 * INFO: If i am a Client, then i'll try to connect to a server at this address
			 * If i am a Server, than i restrict the acception of connection requests to this address.
			 * (at least in theory, this would have to be changed in the server_setup_macro
			 * 		-> it isn't done like this for now. Currently the wildcard address is hardcoded)
			 * This Information can only be passed in combination and after the client/server switch
			 * If you want to pass the connection Address for the Inter AI Connection by Command Line
			 * then it comes straight after the Address_source Option (i.e. the second command line argument
			 * after the read-out source for the connection mode)
			 */
			if(argc>3){
				if (strcmp(*(argstart+3), "cfg") == 0){
					aicm.AIPartnerAddrSrc=INTER_AI_ADDR_SRC_CFG;
				} else if (strcmp(*(argstart+3), "auto") == 0){
					aicm.AIPartnerAddrSrc=INTER_AI_ADDR_SRC_AUTO;
				} else if (strcmp(*(argstart+3), "cmdline") == 0){
					printf("test\n");
					if(argc>4){
						argcpasstoabsint=argc-4;
						argpasstoabsint=argstart+4;
					}else{
						printf("AbsInt: To few Arguments on \"absint auto [controller|autonom] <aiconnectmode> <AIAdressSrc>\"\nYou said i shall take the AI Adress from the Command Line\nBut you didn't gave me an Address...");
					}
					aicm.AIPartnerAddrSrc=INTER_AI_ADDR_SRC_CMDLINE;
				} else{
					aicm.AIPartnerAddrSrc=INTER_AI_ADDR_SRC_CFG;
				}
			}
			argc--;
			argstart++;
			if (strcmp(*(argstart), "controller") == 0){
				err=absint_auto_controller(argcpasstoabsint,argpasstoabsint,sktctr,cmd,aicm);
			} else if (strcmp(*(argstart), "autonom") == 0){
				err=absint_auto_autonom(argcpasstoabsint,argpasstoabsint,sktctr,cmd,aicm);
			}
		}
	} else if (strcmp(*(argstart), "debug") == 0){
		if (argc<2){
			printf("\nAbsInt: To few Arguments on Functions from \"debug\"\n");
			return MAIN_ERR_BAD_CMDLINE;
		} else {
			argc--;
			argstart++;
			if (strcmp(*(argstart), "mptcpinit") == 0){

			} else if (strcmp(*(argstart), "poparp") == 0){
				unsigned int ifc;//The Number of all available Interfaces
				char **ifacestart;
				char **ifmacstart;
				char **ifaddrstart;
				char **ifpeerstart;
				uintptr_t ifps;//InterfacePointerSize. The size of the pointers. Used for incrementing/decrementing and thus wrapping through the lists.
						//In most cases not really needed. The compiler actually ensures, that increments are done with the right size.
						//If you do a +1 on a (char *) on a 64-Bit System, than the (char *) has a sizeof 8-Byte and thus the compiler
						//calculates the increment of +1 as +8
				err = getInterfacesAndAdresses(&ifc,&ifps,&ifacestart,&ifmacstart,&ifaddrstart);

				err=populateARP(ifc,ifaddrstart);
			} else if (strcmp(*(argstart), "readarp") == 0){
				struct AbsintInterfaces ifcollect;
				ifcollect.neighbours.dnstart=NULL;
				uintptr_t ifps;
				err = getInterfacesAndAdresses(&(ifcollect.ifc),&ifps,&(ifcollect.ifacestart),&(ifcollect.ifmacstart),&(ifcollect.ifaddrstart));

				err=getPortPeer(&ifcollect);
			} else if (strcmp(*(argstart), "getrealtime") == 0){
				double realtime;
				printf("Sizeof Double: %d\n",sizeof(double));
				struct timespec waittime;
				puts("");
				realtime=getRealTime();
				printf("RealTime: %f",realtime);
				printf("   | Debug Bytes: %X | %X | %X | %X | %X | %X | %X | %X\n",
						*((unsigned char *)(&realtime)),
						*(((unsigned char*)(&realtime))+1),
						*(((unsigned char*)(&realtime))+2),
						*(((unsigned char*)(&realtime))+3),
						*(((unsigned char*)(&realtime))+4),
						*(((unsigned char*)(&realtime))+5),
						*(((unsigned char*)(&realtime))+6),
						*(((unsigned char*)(&realtime))+7));
				printf("\t\tWait 1 sec...\n");
				waittime.tv_sec=1;
				waittime.tv_nsec=0;
				do {
					err = nanosleep(&waittime, &waittime);
				} while (err<0);
				realtime=getRealTime();
				printf("RealTime: %lf\n",realtime);
				printf("\t\tWait 0.5 sec...\n");
				waittime.tv_sec=0;
				waittime.tv_nsec=500000000;
				do {
					err = nanosleep(&waittime, &waittime);
				} while (err<0);
				realtime=getRealTime();
				printf("RealTime: %lf\n",realtime);
				printf("\t\tWait 2 sec...\n");
				waittime.tv_sec=2;
				waittime.tv_nsec=0;
				do {
					err = nanosleep(&waittime, &waittime);
				} while (err<0);
				realtime=getRealTime();
				printf("RealTime: %lf\n",realtime);
				printf("\t\tWait 0.5 sec...\n");
				waittime.tv_sec=0;
				waittime.tv_nsec=500000000;
				do {
					err = nanosleep(&waittime, &waittime);
				} while (err<0);
				realtime=getRealTime();
				printf("RealTime: %lf\n",realtime);
				printf("\t\tWait 5 sec...\n");
				waittime.tv_sec=5;
				waittime.tv_nsec=0;
				do {
					err = nanosleep(&waittime, &waittime);
				} while (err<0);
				realtime=getRealTime();
				printf("RealTime: %lf\n",realtime);
			} else if (strcmp(*(argstart), "sdnbydns") == 0){
				CREATE_PROGRAM_PATH(*args);
				ABSINT_READ_CONFIG_FILE;
				puts("");
				printfc(yellow,"--> Starting DNS Stuff\n");
				printfc(red,"\t");
				if(sdnctrldns==NULL){
					printfc(red,"Ctrl Address");printf(" directly delivered as ");printfc(red,"IP4");
					puts("");
				}else{
					printf("I shall ascertain the ");printfc(red,"Ctrl Address");printf(" via ");printfc(red,"DNS");
					puts("");
					#ifdef DEBUG
					printfc(red,"DEBUG: ");
					printf("Memory Size of SDN Ctrl DNS-Address: ");
					print_malloc_size(sdnctrldns);
					puts("");
					#endif
					ABSINT_INET_SOCKET_CONTROLLER_COM_DNS_LOOKUP;
				}
				puts("");
				char ControllerIP4[16]; \
				inet_ntop(AF_INET,&controlleraddress,ControllerIP4,sizeof(ControllerIP4)); \
				printf("So the (resulting) IP4 SDN Ctrl Address is: %s\n",ControllerIP4);
			} else {
				printf("Invalid Command after \"absint debug\"\n");
				return MAIN_ERR_BAD_CMDLINE;
			}
		}
	} else if (strcmp(*(argstart), "controller") == 0){
		argc--;
		argstart++;
		err=absint_controller(argc,argstart,sktctr,cmd);
	} else if (strcmp(*(argstart), "wlanmonitor") == 0){
		argc--;
		argstart++;

		printf("\"ollerus absint wlanmonitor\" called.\n");
		puts("");
		printfc(gray,"NOTE: ");printf("Usage:\n");
		printf("\t\"wlanmonitor 2GHz\": Monitors the 2.4GHz Spectrum. Channel Distance specified in the cfg-File.\n");
		printf("\t\"wlanmonitor 5GHz [optional further spec]\": Run \"ollerus absint wlanmonitor 5Gz\" to get more detailed Info at startup.\n");
		printf("\t\"wlanmonitor chan [channel-list]\": Specify a list of channels to observe. Separated with \",\" (without Blanks/whitspaces).\n\t\tBsp.: wlanmonitor chan 1,5,36\n");
		puts("");


		CREATE_PROGRAM_PATH(*args);
		ABSINT_READ_CONFIG_FILE;

		struct AbsintInterfaces ifcollect;
		ifcollect.neighbours.dnstart=NULL;
		uintptr_t ifps;
		err = getInterfacesAndAdresses(&(ifcollect.ifc),&ifps,&(ifcollect.ifacestart),&(ifcollect.ifmacstart),&(ifcollect.ifaddrstart));
		struct AbsintInterfacesWLAN wifcollect;
		wifcollect.wlanc=0;
		wifcollect.wlanidx=NULL;
		select_AI_wlan_dev(&wifcollect,&ifcollect,sktctr,cmd);

		char* ai_wlan_dev_namep[2];
		unsigned int ai_wlan_dev_idx[2];

		if(wifcollect.wlanc>=1){
			AI_WLAN_SWITCH_DEV_NAMEP_OLD=(ifcollect.ifacestart)[(wifcollect.wlanidx)[0]];
		}

		/* Parse die Argumente nach "wlandevs" nacheinander.
		 * Prüfe dabei immer ob das aktuelle Argument ein neues Kommandozeilen-Argument ist
		 * Falls nicht: Es ist ein weiteres WLAN-Device.
		 * ---------------------------------
		 * For now just quick and easy: Read out and finished.
		 * If there comes an extension with additional cmd-line arguments, then do it nice, like mentioned above
		 * and exclude the read wlandevs from the argstart array.
		 */
		int i;
		for(i=0;i<argc;i++){
			if(strcmp(argstart[i],"wlandevs")==0){
				if(argc-(i+1)<1){
					printfc(red,"ERROR");printf(": Passed to few WLAN-Devices after wlandevs!\n");
					exit(1);
				}
				int j;
				i++;
				for(j=0;j<wifcollect.wlanc;j++){
					if(strcmp(argstart[i],(ifcollect.ifacestart)[(wifcollect.wlanidx)[j]])==0){
						AI_WLAN_SWITCH_DEV_NAMEP_OLD=(ifcollect.ifacestart)[(wifcollect.wlanidx)[j]];
						goto FirstPassedWLANDevExists;
					}
				}
				printfc(red,"ERROR");printf(": The passed WLAN-Device after \"wlandevs\" doesn't exist!\n");
				exit(1);
				FirstPassedWLANDevExists:
				break;
			}
		}

		//TODO: Delete
//		#ifdef DEVELOPMENT_MODE
//		goto DevMonStart;
//		#endif

		if(wifcollect.wlanc<1){
			printfc(red,"ERROR");printf(": Not enough WLAN-Devices existent! EXITING!!!\n");
			exit(NETWORK_ERR_RARE_INTERFACES);
		}

		AI_WLAN_SWITCH_DEV_IDX_OLD=if_nametoindex(AI_WLAN_SWITCH_DEV_NAMEP_OLD);

		//Resetting Device (Getting sure, that it is 'up')
		err=secure_device_is_up(AI_WLAN_SWITCH_DEV_NAMEP_OLD);
		//(Getting sure, that no wpa_supplicant is currently running; i.e. stop, if any)
		stop_adhoc_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_OLD,check_running_wpa_supplicant(AI_WLAN_SWITCH_DEV_NAMEP_OLD));
		//Bit of setting the Device
						//ifconfig s1-eth1 10.0.1.100/24 netmask 255.255.255.0 broadcast 10.0.1.255
		#define IFCONFIG_CMD_PART1 "ifconfig"
		#define IFCONFIG_CMD_IP_ADDR "192.0.0.250"
		#define IFCONFIG_CMD_PART2 "netmask "
		#define IFCONFIG_CMD_NETMASK "255.255.255.0"
		#define IFCONFIG_CMD_PART3 "broadcast"
		#define IFCONFIG_CMD_BROADCAST_ADDR "192.0.0.255"
		char ipcmd[sizeof(IFCONFIG_CMD_PART1)+sizeof(AI_WLAN_SWITCH_DEV_NAMEP_OLD)+sizeof(IFCONFIG_CMD_IP_ADDR)];//Through the 'sizeof' comes a additional +1 for every string-part implicit usable for the succeeding Blank. The last +1 from its Null-terminating-Character is obviously used for the Null-Termination of the whole string
		snprintf(ipcmd,sizeof(ipcmd),"%s %s %s",IFCONFIG_CMD_PART1,AI_WLAN_SWITCH_DEV_NAMEP_OLD,IFCONFIG_CMD_IP_ADDR); //Only done once at startup, so performance of few cycles doesn't impact really hard: Just a quick snprintf instead of a "good" solution ;oP
		ipcmd[sizeof(ipcmd)]='\0';
		printf("IP-Address Setup, System-Call: %s\n",ipcmd);
		system(ipcmd);
		memset(ipcmd,0,sizeof(ipcmd));
		#undef IFCONFIG_CMD_PART1
		#undef IFCONFIG_CMD_IP_ADDR
		#undef IFCONFIG_CMD_PART2
		#undef IFCONFIG_CMD_NETMASK
		#undef IFCONFIG_CMD_PART3
		#undef IFCONFIG_CMD_BROADCAST_ADDR

		//TODO: Delete:
//		#ifdef DEVELOPMENT_MODE
//		DevMonStart:
//		;
//		static char testdev[] = "wlan0";
//		AI_WLAN_SWITCH_DEV_NAMEP_OLD=testdev;
//		#endif

		double timetomonitor=(double)WLANChanTrafficMonTime.tv_sec + (double)WLANChanTrafficMonTime.tv_nsec / 1000000000.0;
		absint_wlan_monitor_standalone(argc,argstart,AI_WLAN_SWITCH_DEV_NAMEP_OLD,timetomonitor,WLANChanTrafficMonChanDistance);
	} else {
		printf("Invalid Command after \"absint\"\n");
		return MAIN_ERR_BAD_CMDLINE;
	}

	printf("\nAbsInt: Going to Exit the AI with Error-Code: %d\n",err);
	return err;
}
