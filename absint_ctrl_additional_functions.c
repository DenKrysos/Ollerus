/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */

#define NO_ABSINT_CTRL_ADDITIONAL_FUNCTIONS_C_FUNCTIONS

#include "ollerus_globalsettings.h"
#include "ollerus.h"
#include "ollerus_base.h"
#include "absint.h"
#include "absint_ctrl.h"
#include "absint_ctrl_sdn_ctrl_com.h"

#include "head/ollerus_extern_functions.h"




//IP Address - Index - Connection
void print_connected_ai(){

}




/* Prints the RSSIs of the connected AI on the given thread-index
 * If you pass '-1' (means (others=>1), i.e. every Bit==1 in an unsigned manner [equal to UINT_MAX])
 * it prints the RSSIs of every connected AI
 */
int print_rssi_of_ai_connections(int idx,pthread_t *all_threads,struct AICtrlConnectedAI **AIConnections){
	int err;err=0;
	int freq,chan;
	int i,j,k;
	puts("");
	if((idx==-1)||(idx==UINT_MAX)){
		printfc(green,"WLAN Neighbours of active AI Connections:\n");
	}else{
		printfc(green,"WLAN Neighbours of AI Connection ");
		printfc(cyan,"#%d",idx);printfc(green,":\n");
	}
	for(i=0;i<ABSINT_CTRL_THREADS_MAX;i++){
		if(all_threads[i]!=0) {
			printfc(cyan,"--> ");printf("Index: %d\n",i);
			for(j=0;j<13;j++){
				if(((*AIConnections)[i]).wneigh24[j].start){
					chan=j+1;
					freq = ieee80211_channel_to_frequency(chan, NL80211_BAND_2GHZ);// NL80211_BAND_5GHZ
					printfc(light_cyan,"   --> ");printf("Frequency: %d (Channel %d)\n",freq,chan);
					for(k=0;k<(((*AIConnections)[i]).wneigh24[j]).count;k++){
						printf("\tMAC: ");printMAC(((((*AIConnections)[i]).wneigh24[j]).start)[k].mac,6);
						printf(" | RSSI: %d dBm",((((*AIConnections)[i]).wneigh24[j]).start)[k].rssi);
						puts("");
					}
				}
			}
		}
	}
	puts("");
	return err;
}
int print_active_ai_connections(pthread_t *all_threads,struct AICtrlConnectedAI **AIConnections,struct AI_ChanSwitch_Couple **AICouples){
	int err;err=0;
	
	char IP4Print[16];
	int i,j;
	puts("");
	printfc(green,"Active AI Connections:\n");
	for(i=0;i<ABSINT_CTRL_THREADS_MAX;i++){
		if(all_threads[i]!=0) {
			printfc(cyan,"--> ");printf("Index: %d | DPID: %llu\n",i,((*AIConnections)[i]).dpid);
			if(AICouples){
				if(AICouples[i]){
					printf("\t\tCoupled: %d <-> %d\n",(AICouples[i])->AI1_thread_index,(AICouples[i])->AI2_thread_index);
				}else{
					printf("\t\tNot Coupled yet.\n");
				}
			}
			for(j=0;j<((*AIConnections)[i]).ifc;j++){
				printf("\t%s : ",((((*AIConnections)[i]).iface)[j]).iface);
//				if((uint32_t)(((((*AIConnections)[i]).iface)[j]).IP4) == 0){
//					printf("No IP4-Address assigned!\n");
//				}else {
					inet_ntop(AF_INET,&(((((*AIConnections)[i]).iface)[j]).IP4),IP4Print,sizeof(IP4Print));
					printf("%s",IP4Print);
//				}
				printf(" | Freq: %d MHz",((((*AIConnections)[i]).iface)[j]).freq);
				printf(" | SSID: %s",((((*AIConnections)[i]).iface)[j]).adhocssid);
//				printf(" | Bitrate: %d.%d Mbit/s",(((((*AIConnections)[i]).iface)[j]).bitrate)/10,(((((*AIConnections)[i]).iface)[j]).bitrate)%10);
				printf(" | Packet Error Rate: %d\%",((((*AIConnections)[i]).iface)[j]).packerrrateTX);
				puts("");
			}
		}
	}
	puts("");
	
	return err;
}
int print_dpids(struct DPID_Collector *DPIDs){
	//TODO
	int err=0;
	puts("");
	printfc(green,"In Hand DPIDs (#%d):\n",DPIDs->count);
	for(err=0;err<DPIDs->count;err++){
		printf("\t%llu\n",(DPIDs->dpid)[err]);
	}
	puts("");

	return err;
}




/* Better not use this. Isn't finished and changed to a macro
 * (See DPID_REFRESH & DPID_REFRESH_AND_WAIT in "absint_ctrl_sdn_ctrl_com.h")
 */
char refresh_dpids(struct DPID_Collector *DPIDs){
	if(DPIDs->count!=0){
		free(DPIDs->dpid);
		DPIDs->dpid=NULL;
		DPIDs->count=0;
	}

	return 0;
}



char dpid_is_present(uint64_t mac,struct DPID_Collector *DPIDs){
	int i;
	for(i=0;i<DPIDs->count;i++){
//		int j;
//		puts("Values");
//		for(j=0;j<8;j++){
//			printf("delivered %02X | cmp %02X\n",*(((unsigned char *)(&mac))+j),*((unsigned char *)(&((DPIDs->dpid)[i]))+j));
//		}
//		puts("");
		if(mac==((DPIDs->dpid)[i]))
			return 1;
	}

	return 0;
}








int establish_wlan_topology_connections(struct AI_ChanSwitch_Couple **UnestablishedAIs,pthread_t *all_threads, int *index1, int *index2){
	int i;
	int err;err=1;
	for(i=0;i<ABSINT_CTRL_THREADS_MAX;i++){
		if(!(UnestablishedAIs[i])){
			//We have the first unassociated WLAN-Station.
			//Better check, if this exists. A AI could have disconnected. Then his index in the threads array is definitely cleaned
			//but maybe not in the AI_Couple Array.
			if(all_threads[i]!=0){
				*index1=i;
				break;
			}else{
				continue;
			}
		}
	}
	for(i+=1;i<ABSINT_CTRL_THREADS_MAX;i++){
		if(!(UnestablishedAIs[i])){
			//We have the second unassociated WLAN-Station.
			//For this we REALLY have to check the existence. It could be, that we do not have two
			//unassociated Stations, to let them connect. If only one is left unconnected, we have to wait for another
			//AI to connect with the controller
			if(all_threads[i]!=0){
				*index2=i;
				err=0;
				break;
			}else{
				continue;
			}
		}
	}
	if(err==0){
		UnestablishedAIs[*index1]=malloc(sizeof(struct AI_ChanSwitch_Couple));
		UnestablishedAIs[*index2]=UnestablishedAIs[*index1];
		UnestablishedAIs[*index1]->AI1_thread_index=*index1;
		UnestablishedAIs[*index1]->AI2_thread_index=*index2;
		UnestablishedAIs[*index1]->FSM=0;
		UnestablishedAIs[*index1]->fsmsock=0;
		sem_init(&(UnestablishedAIs[*index1]->sem_sock),0,1);
	}
	return err;
}



int analyse_wlanstatistics(int *newfreq, int currentfreq, double *stat, int statsize){
	int err=0;
	int i;
	int currentchan=ieee80211_frequency_to_channel(currentfreq);
	int currentidx=currentchan/WLAN_TRAFFICSTAT_CHAN_DIST;//I'd say compiler is clever enough to change it to right-shift, if =4 ;o)
	#define BORDER 1000//The Border to decide, if the current channel is good enough. If his stat lays over this value, the channel quality lasts and we won't switch
	#define OFFSET 500//The Offset, the new channel has to be better than the currently used. If the new channel isn't really better, then a switch wouldn't give us a noticable benefit.

	//First check, if current channel is good enough
	if(stat[currentidx]>BORDER)
		return 1;

	//Then, search for the best channel
//	double *min;
//	min=stat;
	int min;
	min=0;
	for(i=1;i<statsize;i++){
		if(stat[min]>stat[i])
			min=i;
	}
	//Best==Current?
	if(min==currentidx)
		return 2;

	//Is the best channel sufficiently better than the current?
	if(stat[min]<(stat[currentidx]+OFFSET))
		return 3;

	*newfreq=ieee80211_channel_to_frequency(min*WLAN_TRAFFICSTAT_CHAN_DIST, NL80211_BAND_2GHZ);// NL80211_BAND_5GHZ
	return 0;

	#undef BORDER
	#undef OFFSET
	return err;
}







#undef NO_ABSINT_CTRL_ADDITIONAL_FUNCTIONS_C_FUNCTIONS
