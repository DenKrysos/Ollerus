/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

//-------For BSD Sockets-------//
/*
 * Hinweis:
 * Beachten Sie, dass bei BSD-Systemen beim Kompilieren
 * immer die Headerdatei <sys/types.h>
 * noch vor <sys/socket.h> inkludiert werden muss,
 * sonst gibt es einen Fehler beim Kompilieren.
 */
#include "ollerus.h"

//#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <math.h>
//-------END BSD Sockets-------//
//-------For ioctl-------//
//#include <sys/types.h>
//#include <sys/socket.h>
#include </usr/include/linux/wireless.h>
#include </usr/include/linux/if_arp.h>
//#include </usr/include/linux/types.h>
#include <ifaddrs.h>
//#include <netinet/in.h>
//-------END ioctl-------//
//#include <libconfig.h>





#define PORTADAPTTXPOWCLIENT 24288
#define RECVBUFFSIZEADAPTTXPOW 20
#define ADAPTMSGTYPE_TXPOWER 0x00
#define ADAPTMSGTYPE_NOISE 0x01
//Values for direct approach. Center Value ist -71 dB for 54 MBit/s
#define MATO 0xBC //Hex for: -68 dB (2er Komplement)
#define MATU 0xB6 //Hex for: -74 dB (2er Komplement)
#define MATC 0xB9 //Hex for: -71 dB (2er Komplement)
//Values for relative approach
#define MATDIFFO 0x03
#define MATDIFFU 0x03
#define MATDIFFC -71
//SNR for 54 MBit/s at Access-Point (25 dBm)
#define SNRC 25
#define SNRDIFFO 3
#define SNRDIFFU 2

#define INDEX_CHAN36 0
#define INDEX_CHAN40 1
#define INDEX_CHAN44 2
#define INDEX_CHAN48 3
#define INDEX_CHAN52 4
#define INDEX_CHAN56 5
#define INDEX_CHAN60 6
#define INDEX_CHAN64 7
#define ARRAY_5GHZ_SIZE 8

#define INDEX_CHAN1 0
#define INDEX_CHAN2 1
#define INDEX_CHAN3 2
#define INDEX_CHAN4 3
#define INDEX_CHAN5 4
#define INDEX_CHAN6 5
#define INDEX_CHAN7 6
#define INDEX_CHAN8 7
#define INDEX_CHAN9 8
#define INDEX_CHAN10 9
#define INDEX_CHAN11 10
#define INDEX_CHAN12 11
#define INDEX_CHAN13 12
#define ARRAY_24GHZ_SIZE 13

#define SCAN_INTERVAL_TIME 5
#define SCAN_INTERVAL_RATE 4
#define HANDOVER_TIME 5



//Header Data for channel selection
struct ChannelLoadChain {
	unsigned int frequency;
	signed int noise;
	unsigned long long active;
	unsigned long long busy;
	unsigned long long receiving;
	unsigned long long transmitting;
	struct ChannelLoadChain *next;
};

struct ChannelLoad_start {
	unsigned int count;
	unsigned int currentcnt;
	struct ChannelLoadChain *start;
	struct ChannelLoadChain *current;
	int (*append) ();
};

struct channel_property {
	 int channel;
	 signed int noise;
	 double long load;
	 int count;
};

struct noiseThreadArgPassing {
	pthread_t *ownindex;
	struct nl80211_state *skt;
	struct CommandContainer *cmd;
	char *AP_scan;
	struct interfaces *int_names;
	struct bestchan	*setchan;
	struct bestchan *bestchan;
	pthread_t *hostapdthread;

};
struct bestchan{
	int band24;
	int band5;
	int active_interface;
	char AP_index;
};

struct interfaces{
	int wlan0;
	int wlan1;
	int wlan2;
};



//Functions for channel selection

int ChannelLoadChainAppend (struct ChannelLoadChain *tostore, struct ChannelLoad_start *self) {
	struct ChannelLoadChain *new = malloc(sizeof(struct ChannelLoadChain));
//Don't forget to free all this structs, if you do not need them anymore
//See function free_scan_chain
	memcpy(new, tostore, sizeof(struct ChannelLoadChain));
	if (self->count == 0) {
		self->start = new;
		self->count = 1;
		self->currentcnt = 1;
	} else {
		(*(self->current)).next = new;
		self->count++;
		self->currentcnt++;
	}
	self->current = new;

	return 0;
}


struct ChannelLoad_start *channelloadstart = NULL;

void setupChannelLoadChainStart() {

	channelloadstart = malloc(sizeof(struct ChannelLoad_start));
	memset(channelloadstart, 0, sizeof(struct ChannelLoad_start));
	channelloadstart->append = ChannelLoadChainAppend;
}

int free_channelload_chain (struct ChannelLoad_start *start) {
	if (!start)
		return STRUCT_ERR_NOT_EXIST;
	if (start->count < 1) {
		goto NoElementInChain;
	}
	if (start->count > 1) {
		if (!(start->start->next))
			return STRUCT_ERR_DMG;
		start->current = start->start->next;
		start->currentcnt = 1;
		while (start->currentcnt < start->count - 1) {
			free(start->start);
			if (!(start->current->next))
				return STRUCT_ERR_DMG;
			start->start = start->current;
			start->current = start->start->next;
			start->currentcnt++;
		}
		free(start->current);
	}
	free(start->start);

	NoElementInChain:
	free(start);
	channelloadstart = NULL;

	return 0;
}


int Do_CMD_GET_SURVEY_cb_communication_load(struct nl_msg *msg, void *arg)
{
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *genlhdr = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *survinf[NL80211_SURVEY_INFO_MAX + 1];
	static struct nla_policy survey_policy[NL80211_SURVEY_INFO_MAX + 1] = {
		[NL80211_SURVEY_INFO_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_SURVEY_INFO_NOISE] = { .type = NLA_U8 },
	};
	char dev[20];

	static struct ChannelLoadChain tostore;
	memset(&tostore, 0, sizeof(struct ChannelLoadChain));


	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(genlhdr, 0), genlmsg_attrlen(genlhdr, 0), NULL);

	if_indextoname(nla_get_u32(got_attr[NL80211_ATTR_IFINDEX]), dev);
//	printf("Survey data from %s\n", dev);

	if (!got_attr[NL80211_ATTR_SURVEY_INFO]) {
		fprintf(stderr, "survey data missing!\n");
		return NL_SKIP;
	}

	if (nla_parse_nested(survinf, NL80211_SURVEY_INFO_MAX, got_attr[NL80211_ATTR_SURVEY_INFO], survey_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	if (survinf[NL80211_SURVEY_INFO_FREQUENCY])
		tostore.frequency = (unsigned int)nla_get_u32(survinf[NL80211_SURVEY_INFO_FREQUENCY]);
	if (survinf[NL80211_SURVEY_INFO_NOISE])
		tostore.noise = (int8_t)(signed int)nla_get_u8(survinf[NL80211_SURVEY_INFO_NOISE]);
	if (survinf[NL80211_SURVEY_INFO_CHANNEL_TIME])
		tostore.active = (unsigned long long)nla_get_u64(survinf[NL80211_SURVEY_INFO_CHANNEL_TIME]);
	if (survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY])
		tostore.busy = (unsigned long long)nla_get_u64(survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY]);
//	if (survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY])
//		tostore.busy = (unsigned long long)nla_get_u64(survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY]);
	if (survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_RX])
		tostore.receiving = (unsigned long long)nla_get_u64(survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_RX]);
	if (survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_TX])
		tostore.transmitting = (unsigned long long)nla_get_u64(survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_TX]);

	channelloadstart->append (&tostore, channelloadstart);

	return NL_SKIP;
}

int channelloadchain_console_print (struct ChannelLoad_start *start, struct channel_property *load24, struct channel_property *load5,struct bestchan *setchan, char AP_scan) {
	if (!start)
		goto NoChain;
//	printf("<--------------------------------------->");
	if (start->count < 1) {
		printf("\nChain doesn't contain anything!\n");
		goto NoChainElement;
	}
	start->current = start->start;
	start->currentcnt = 1;
	long double calc1, calc2, load;
//printf("\nAP_index %i\n",setchan->AP_index);

	if(AP_scan==1){
		while (1) {
			if(start->current->busy!=0){


				calc1=((start->current->busy)-(start->current->transmitting));
				calc2=((start->current->active)-(start->current->transmitting));
				load= calc1/calc2;

			int channel = ieee80211_frequency_to_channel(start->current->frequency);

			if (channel == setchan->band24){
				int counter;
				for(counter=0;counter<ARRAY_24GHZ_SIZE;counter++){
					if(channel==load24[counter].channel){
						load24[counter].noise = start->current->noise;
						load24[counter].load = load;
						load24[counter].count = 1;

					}
				}
			}
			}
			if (start->currentcnt >= start->count)
						break;
					if (!(start->current->next))// && (start->currentcnt < start->count))
						return STRUCT_ERR_DMG;
					start->current = start->current->next;
					start->currentcnt++;

		}
	}

	else{
		while (1) {//(start->currentcnt <= start->count) {
			//Better to do the check inside the loop, to catch the right break-point
			printf("\n");
			printf("\tEntry #%d", start->currentcnt);
			printf("\n");

			printf("\n%i\n",start->current->frequency);
			printf("\n%d\n",start->current->noise);
			printf("\n%i\n",start->current->active);
			printf("\n%i\n",start->current->busy);
			printf("\n%i\n",start->current->receiving);
			printf("\n%i\n",start->current->transmitting);


			if (start->current->noise>(-50)){
				long double noise_fac, noise_w;
				noise_w=pow(10,start->current->noise/10);
				noise_fac=pow(2,noise_w);
printf("\n\n\n%Lf\t%Lf",noise_w,noise_fac);
				calc1=((start->current->busy)-(start->current->transmitting));
				calc2=((start->current->active)-(start->current->transmitting));
				load= (calc1/calc2)*noise_fac;
				printf("\n%Lf",load);
			}

			else if(start->current->busy!=0){


				calc1=((start->current->busy)-(start->current->transmitting));
				calc2=((start->current->active)-(start->current->transmitting));
				load= calc1/calc2;
			}

			else
				load=0;


				switch(start->current->frequency){
				case 2412:
					load24[0].channel= 1;
					load24[0].noise=start->current->noise;
					load24[0].load+=load;
					load24[0].count++;
		//			printf("\n\nload Kanal 1:%Lf\ncount:%i\n",load24[0].load,load24[0].count);
					break;
				case 2417:
					load24[1].channel= 2;
					load24[1].noise=start->current->noise;
					load24[1].load+=load;
					load24[1].count++;
		//			printf("\n\nload Kanal 2:%Lf\ncount:%i\n",load24[1].load,load24[1].count);
					break;
				case 2422:
					load24[2].channel= 3;
					load24[2].noise+=start->current->noise;
					load24[2].load=load;
					load24[2].count++;
		//			printf("\n\nload Kanal 3:%Lf\ncount:%i\n",load24[2].load,load24[2].count);
					break;
				case 2427:
					load24[3].channel= 4;
					load24[3].noise=start->current->noise;
					load24[3].load+=load;
					load24[3].count++;
		//			printf("\n\nload Kanal 4:%Lf\ncount:%i\n",load24[3].load,load24[3].count);
					break;
				case 2432:
					load24[4].channel= 5;
					load24[4].noise=start->current->noise;
					load24[4].load+=load;
					load24[4].count++;
		//			printf("\n\nload Kanal 5:%Lf\ncount:%i\n",load24[4].load,load24[4].count);
					break;
				case 2437:
					load24[5].channel= 6;
					load24[5].noise=start->current->noise;
					load24[5].load+=load;
					load24[5].count++;
		//			printf("\n\nload Kanal 6:%Lf\ncount:%i\n",load24[5].load,load24[5].count);
					break;
				case 2442:
					load24[6].channel= 7;
					load24[6].noise=start->current->noise;
					load24[6].load+=load;
					load24[6].count++;
		//			printf("\n\nload Kanal 7:%Lf\ncount:%i\n",load24[6].load,load24[6].count);
					break;
				case 2447:
					load24[7].channel= 8;
					load24[7].noise=start->current->noise;
					load24[7].load+=load;
					load24[7].count++;
		//			printf("\n\nload Kanal 8:%Lf\ncount:%i\n",load24[7].load,load24[7].count);
					break;
				case 2452:
					load24[8].channel= 9;
					load24[8].noise=start->current->noise;
					load24[8].load+=load;
					load24[8].count++;
		//			printf("\n\nload Kanal 9:%Lf\ncount:%i\n",load24[8].load,load24[8].count);
					break;
				case 2457:
					load24[9].channel= 10;
					load24[9].noise=start->current->noise;
					load24[9].load+=load;
					load24[9].count++;
		//			printf("\n\nload Kanal 10:%Lf\ncount:%i\n",load24[9].load,load24[9].count);
					break;
				case 2462:
					load24[10].channel= 11;
					load24[10].noise=start->current->noise;
					load24[10].load+=load;
					load24[10].count++;
		//			printf("\n\nload Kanal 11:%Lf\ncount:%i\n",load24[10].load,load24[10].count);
					break;
				case 2467:
					load24[11].channel= 12;
					load24[11].noise=start->current->noise;
					load24[11].load+=load;
					load24[11].count++;
		//			printf("\n\nload Kanal 12:%Lf\ncount:%i\n",load24[11].load,load24[11].count);
					break;
				case 2472:
					load24[12].channel= 13;
					load24[12].noise=start->current->noise;
					load24[12].load+=load;
					load24[12].count++;
		//			printf("\n\nload Kanal 13:%Lf\ncount:%i\n",load24[12].load,load24[12].count);
					break;
				case 5180:
					load5[0].channel=36;
					load5[0].noise=start->current->noise;
					load5[0].load+=load;
					load5[0].count++;
		//			printf("\n\nload Kanal 36:%Lf\ncount:%i\n",load5[0].load,load5[0].count);
					break;
				case 5200:
					load5[1].channel=40;
					load5[1].noise=start->current->noise;
					load5[1].load+=load;
					load5[1].count++;
		//			printf("\n\nload Kanal 40:%Lf\ncount:%i\n",load5[1].load,load5[1].count);
					break;
				case 5220:
					load5[2].channel=44;
					load5[2].noise=start->current->noise;
					load5[2].load+=load;
					load5[2].count++;
		//			printf("\n\nload Kanal 44:%Lf\ncount:%i\n",load5[2].load,load5[2].count);
					break;
				case 5240:
					load5[3].channel=48;
					load5[3].noise=start->current->noise;
					load5[3].load+=load;
					load5[3].count++;
		//			printf("\n\nload Kanal 48:%Lf\ncount:%i\n",load5[3].load,load5[3].count);
					break;
				case 5260:
					load5[4].channel=52;
					load5[4].noise=start->current->noise;
					load5[4].load+=load;
					load5[4].count++;
		//			printf("\n\nload Kanal 52:%Lf\ncount:%i\n",load5[4].load,load5[4].count);
					break;
				case 5280:
					load5[5].channel=56;
					load5[5].noise=start->current->noise;
					load5[5].load+=load;
					load5[5].count++;
		//			printf("\n\nload Kanal 56:%Lf\ncount:%i\n",load5[5].load,load5[5].count);
					break;
				case 5300:
					load5[6].channel=60;
					load5[6].noise=start->current->noise;
					load5[6].load+=load;
					load5[6].count++;
		//			printf("\n\nload Kanal 60:%Lf\ncount:%i\n",load5[6].load,load5[6].count);
					break;
				case 5320:
					load5[7].channel=64;
					load5[7].noise=start->current->noise;
					load5[7].load+=load;
					load5[7].count++;
		//			printf("\n\nload Kanal 64:%Lf\ncount:%i\n",load5[7].load,load5[7].count);
					break;
				default:
					fprintf(stderr,"Error in switch case of load scan");
					break;
				}

			if (start->currentcnt >= start->count)
				break;
			if (!(start->current->next))// && (start->currentcnt < start->count))
				return STRUCT_ERR_DMG;
			start->current = start->current->next;
			start->currentcnt++;

		}
	}
//	printf("<--------------------------------------->");
//		printf("\n");
		return 0;

		NoChainElement:
		printf("<--------------------------------------->");
		printf("\n");
		return STRUCT_ERR_INCOMPLETE;

		NoChain:
		return STRUCT_ERR_NOT_EXIST;
}


void lowload(struct channel_property *lo24,struct channel_property *lo5,struct bestchan *bc){

	FILE *datei;
	datei = fopen("optimizechannel.text", "a+");
	   if(NULL == datei) {
	      printf("\n\n\n\n\n\n\n\nKonnte Datei \"optimizechannel.txt\" nicht öffnen!\n");
	   }
	   if(datei != NULL)
	         printf("\n\n\nDatei erfolgreich geöffnet\n");


//choose between channel 1,6,11
	long double total_load1,total_load6,total_load11;
	total_load1=(lo24[0].load/lo24[0].count)+(lo24[1].load/lo24[1].count)+(lo24[2].load/lo24[2].count);
	total_load1=total_load1/3;

	total_load6=(lo24[3].load/lo24[3].count)+(lo24[4].load/lo24[4].count)+(lo24[5].load/lo24[5].count)+(lo24[6].load/lo24[6].count)+(lo24[7].load/lo24[7].count);
	total_load6=total_load6/5;

	total_load11=(lo24[8].load/lo24[8].count)+(lo24[9].load/lo24[9].count)+(lo24[10].load/lo24[10].count);
	total_load11=total_load11/3;

//the total load of channel 1,6 and 11
	printf("\n%Lf",total_load1);
	printf("\n%Lf",total_load6);
	printf("\n%Lf\n\n",total_load11);



	fprintf(datei,"%Lf\t",total_load1);
	fprintf(datei,"%Lf\t",total_load6);
	fprintf(datei,"%Lf\t",total_load11);



	if((total_load1<=total_load6)&&(total_load1<=total_load11)){
		bc->band24=lo24[0].channel;
	}
	else if((total_load6<=total_load1)&&(total_load6<=total_load11)){
		bc->band24=lo24[5].channel;
	}
	else if((total_load11<=total_load1)&&(total_load11<=total_load6)){
		bc->band24=lo24[10].channel;
	}

	long double  total_load36_64 [ARRAY_5GHZ_SIZE];
	long double	lowload5;
	bc->band5=lo5[0].channel;

	int i;
	for(i=0;i<ARRAY_5GHZ_SIZE;i++){
		total_load36_64[i]=lo5[i].load/lo5[i].count;
//		total load of channels in 5GHz band
//		printf("\n%Lf",total_load36_64[i]);
//		fprintf(datei,"%Lf\t",total_load36_64[i]);
		if(i==0){
			lowload5=total_load36_64[i];
			bc->band5=lo5[i].channel;
		}
		else if (lowload5>total_load36_64[i]){
			lowload5=total_load36_64[i];
			bc->band5=lo5[i].channel;
		}
	}

	fprintf(datei,"\n");
	printf("\n\n\n%i",bc->band24);
	printf("\n%i\n\n",bc->band5);
	fclose(datei);

}




//scanning channels for SNIR
void *loadscan(void*arg){

	//TODO:loadscan

	struct CommandContainer *cmd= ((struct noiseThreadArgPassing *)arg)->cmd;
	struct nl80211_state *skt = ((struct noiseThreadArgPassing *)arg)->skt;
	struct bestchan*bc = ((struct noiseThreadArgPassing *)arg)->bestchan;
	struct bestchan*setchan = ((struct noiseThreadArgPassing *)arg)->setchan;
	struct interfaces*interface_names = ((struct noiseThreadArgPassing *)arg)->int_names;
	pthread_t *hostapdthread =((struct noiseThreadArgPassing *)arg)->hostapdthread;

	int err,count,scan_count;
	struct channel_property  prop24GHz[ARRAY_24GHZ_SIZE];
	struct channel_property  prop5GHz[ARRAY_5GHZ_SIZE];
	char AP_scan=0;

	for(scan_count=0;scan_count<SCAN_INTERVAL_RATE;scan_count++){

		if(scan_count==0){
			for(count=0;count<ARRAY_24GHZ_SIZE;count++){
				prop24GHz[count].channel=0;
				prop24GHz[count].noise=0;
				prop24GHz[count].load=0;
				prop24GHz[count].count=0;
			}
			for(count=0;count<ARRAY_5GHZ_SIZE;count++){
				prop5GHz[count].channel=0;
				prop5GHz[count].noise=0;
				prop5GHz[count].load=0;
				prop5GHz[count].count=0;
			}
		}


		setupChannelLoadChainStart();
		cmd->cmd = NL80211_CMD_GET_SURVEY;
		prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
		cmd->nl_msg_flags = NLM_F_DUMP;
		cmd->callbackToUse = Do_CMD_GET_SURVEY_cb_communication_load;
		err = send_with_cmdContainer(skt, 0, 0, cmd);



		channelloadchain_console_print (channelloadstart, prop24GHz, prop5GHz, setchan, AP_scan);

		free_channelload_chain(channelloadstart);

		lowload(prop24GHz, prop5GHz, bc);

		struct timespec remainingdelay;
			remainingdelay.tv_sec =SCAN_INTERVAL_TIME;
			remainingdelay.tv_nsec = 0;
			do {
				err = nanosleep(&remainingdelay, &remainingdelay);
			} while (err<0);

	}


	setupChannelLoadChainStart();
	cmd->cmd = NL80211_CMD_GET_SURVEY;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &setchan->active_interface);
	cmd->nl_msg_flags = NLM_F_DUMP;
	cmd->callbackToUse = Do_CMD_GET_SURVEY_cb_communication_load;
	err = send_with_cmdContainer(skt, 0, 0, cmd);

	AP_scan=1;

	channelloadchain_console_print (channelloadstart, prop24GHz, prop5GHz, setchan, AP_scan);

	free_channelload_chain(channelloadstart);

	AP_scan=0;

	lowload(prop24GHz, prop5GHz, bc);


	if(bc->band24!=setchan->band24){
		setchan->band24=bc->band24;
		if(setchan->AP_index==0){
			setchan->AP_index=1;
			setchan->active_interface=interface_names->wlan2;
			printdat(setchan,&hostapdthread[1]);
			struct timespec remainingdelay;
			remainingdelay.tv_sec =HANDOVER_TIME;
			do {
				err = nanosleep(&remainingdelay, &remainingdelay);
			} while (err<0);
			kill_AP(0);
		}
		else{
			setchan->AP_index=0;
			setchan->active_interface=interface_names->wlan0;
			printdat(setchan,&hostapdthread[0]);
			struct timespec remainingdelay;
			remainingdelay.tv_sec =HANDOVER_TIME;
			do {
				err = nanosleep(&remainingdelay, &remainingdelay);
			} while (err<0);
			kill_AP(1);
		}
	}
	free(arg);

	return err;
}








enum adapttxpowermodes {
	adhoc,
	client,
	server,
};

sem_t atxpsem[THREADS_CLIENTS_MAX+1];//Semaphores for the thread IDs
//Use THREADS_CLIENTS_MAX as Index Semaphore for complete Array
//Use the numeric Indices to wait/signal single, specific ID holders
//I know, some uses of Semaphores in here are not urgend needed, but hey
//lets make it somehow consistent and oversecure


static int get_first_free_threads_index(pthread_t *threads_adapttxpow) {
//You could do some before-after checking, to see if some Thread-ID-holder
//has changed during looping and
//Read-Write-locks maybe could make something better
//Nevertheless, for now just nice and easy done with simple semaphores
//and ignore everything in this function here
	//<---------------------->
//OK, it's not optimal in a case like:
//All thread-ID holding places are full.
//Then the function here loops over some entry (let's say e.g. index 3)
//Now, during the loop, after it passed entry 3, the thread from index 3 terminates and
//cleans his entry to zero.
//So the loop ends and the function tells the caller that no place is free
//although place 3 got free during check
	//<---------------------->
//But to handle this in here makes it to inefficient and what you
//get for this isn't worth the price...
//In the end it should be handled in some way by the whole concept.
	int i;
	/* Semaphore not really needed. Only reads here...
	sem_wait(&atxpsem[THREADS_CLIENTS_MAX]);
	*/
	for(i=0;i<THREADS_CLIENTS_MAX;i++){
		if(threads_adapttxpow[i]==0) {
//			sem_post(&atxpsem[THREADS_CLIENTS_MAX]);
			return i;
		}
	}
//	sem_post(&atxpsem[THREADS_CLIENTS_MAX]);
	return -1;
}
struct serverListenArgPassing {
	int solid_s; //handy for putting socket descr. to thread
	int *bytes_recvd;
	sem_t *sem_bytes_recvd;
};
void *keep_txpower_min_server_each_listen (void* arg) {
	int s = ((struct serverListenArgPassing *)arg)->solid_s;
	int *bytes_recvd = ((struct serverListenArgPassing *)arg)->bytes_recvd;
	sem_t *sem_bytes_recvd = ((struct serverListenArgPassing *)arg)->sem_bytes_recvd;
	free(arg);
	int err;err=0;
	int pthreadoldcancelstate;
	int bytes_recvd_local;
	char msg_recvd[RECVBUFFSIZEADAPTTXPOW];

	while( (bytes_recvd_local = recv(s , msg_recvd , RECVBUFFSIZEADAPTTXPOW , 0)) > 0 )
    {
    	//Do whatever is needed with the received Information (if any...)

    }

//	err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
	sem_wait(sem_bytes_recvd);
	*bytes_recvd = bytes_recvd_local;
	sem_post(sem_bytes_recvd);
//	err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);

	return err;
}
struct getOnlyNoiseByFreqArgPass {
	int freq;
	signed char noise;
};
static int GET_ONLY_NOISE_BY_FREQ_cb(struct nl_msg* msg, void* arg) {
	struct getOnlyNoiseByFreqArgPass *freqctr = ((struct CallbackArgPass *)arg)->ArgPointer;
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *genlhdr = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *survinf[NL80211_SURVEY_INFO_MAX + 1];
	static struct nla_policy survey_policy[NL80211_SURVEY_INFO_MAX + 1] = {
		[NL80211_SURVEY_INFO_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_SURVEY_INFO_NOISE] = { .type = NLA_U8 },
	};
	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(genlhdr, 0), genlmsg_attrlen(genlhdr, 0), NULL);
	if (!got_attr[NL80211_ATTR_SURVEY_INFO]) {
		fprintf(stderr, "survey data missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(survinf, NL80211_SURVEY_INFO_MAX, got_attr[NL80211_ATTR_SURVEY_INFO], survey_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}
	if (survinf[NL80211_SURVEY_INFO_FREQUENCY]) {
		if(nla_get_u32(survinf[NL80211_SURVEY_INFO_FREQUENCY]) == freqctr->freq) {
			if (survinf[NL80211_SURVEY_INFO_NOISE])
				freqctr->noise = (int8_t)nla_get_u8(survinf[NL80211_SURVEY_INFO_NOISE]);
		}
	}
	return NL_SKIP;
}
static int getOnlyNoiseByFreq(void *freqcontainer, struct nl80211_state *sktctr, struct CommandContainer *cmd) {
	int err;
	cmd->cmd = NL80211_CMD_GET_SURVEY;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
//	prepareAttribute(&cmd, NL80211_ATTR_MAC, WhatWeWant.MAC_ConnectedTo);
	cmd->nl_msg_flags = NLM_F_DUMP;
	cmd->callbackToUse = GET_ONLY_NOISE_BY_FREQ_cb;
	cmd->callbackargpass = freqcontainer;
	err = send_with_cmdContainer(sktctr, 0, 0, cmd);

	return err;
}
struct adaptTxPowThreadArgPassing {
	int solid_s; //handy for putting socket descr. to thread
	struct CommandContainer *cmd;
	struct nl80211_state *sktctr;
	int thread_index; //to let the thread know on which index in the all holding array it stands
	pthread_t *all_threads_array; //Pass the array with references to all active threads. Could be handy somewhere...
};
void *keep_txpower_min_server_each (void* arg) {
//This communicates with its associated client and sets its txpower
//One instantiated as thread for each connected client.

//This cute guys are communicating with the wlan-clients

	int thread_index = (((struct adaptTxPowThreadArgPassing *)arg)->thread_index);
	struct CommandContainer *cmd = (((struct adaptTxPowThreadArgPassing *)arg)->cmd);
	struct nl80211_state *sktctr = (((struct adaptTxPowThreadArgPassing *)arg)->sktctr);
	printf("\t#%d Connection: Starting up.\n", thread_index);
	int s = ((struct adaptTxPowThreadArgPassing *)arg)->solid_s;
	pthread_t *threads_adapttxpow = (((struct adaptTxPowThreadArgPassing *)arg)->all_threads_array);
	//Free the arguments passing memory space
	free(arg);
	int err;

	struct getOnlyNoiseByFreqArgPass getnoiseargs;
	int bytes_recvd;
	char msg_recvd[RECVBUFFSIZEADAPTTXPOW];
	bytes_recvd = recv(s , msg_recvd , RECVBUFFSIZEADAPTTXPOW , 0);
	if (bytes_recvd > sizeof(int)) {
		fprintf(stderr, "Received Frequency Message exceeds size of allocated memory!");
		return MAIN_ERR_STD;
	}
	memcpy((&getnoiseargs.freq+(sizeof(int)-bytes_recvd)),msg_recvd,bytes_recvd);
	printf("\tThread #%d - WLAN Frequency: %d (Bytes of msg: %d)\n",thread_index,getnoiseargs.freq,bytes_recvd);

	signed int owntxpowi;
	signed char owntxpow;
	//TODO: Better without ioctl
//	ioctl_get_txpower(&owntxpowi);
	owntxpow=(char)owntxpowi;
	getOnlyNoiseByFreq(&getnoiseargs,sktctr,cmd);

	signed char msg_2b[2];
	msg_2b[0] = owntxpow;
	msg_2b[1] = getnoiseargs.noise;

	printf("\tThread #%d - Sending TX Power | Noise (on %d): %d | %d\n", thread_index, getnoiseargs.freq,owntxpow,getnoiseargs.noise);
//	char *msg;
//	err = senddetermined(s,msg,strlen(msg));
	err = senddetermined(s,msg_2b,2);

	sem_t sem_bytes_recvd;
	sem_init(&sem_bytes_recvd,0,1);
	char owntxpowold=owntxpow;
	char noiseold=getnoiseargs.noise;
	char noisetolerance=2;


	//Every Server Thread needs an own additional Thread - a listening Thread
	//The Server Thread itself does all the checking and sending stuff
	//The listening Thread blocks at recv messages from the port. Especially listens
	//for a '0' to see if the client closed the connection.
	pthread_t server_each_listen;
	struct serverListenArgPassing *pthreadArgPass;

	pthreadArgPass = malloc(sizeof(struct serverListenArgPassing));
	pthreadArgPass->solid_s = s;
	pthreadArgPass->bytes_recvd = &bytes_recvd;
	pthreadArgPass->sem_bytes_recvd = &sem_bytes_recvd;

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
	//secure detached-state for the threads over attributed creation
	if( (err=pthread_create(&server_each_listen, &tattr, keep_txpower_min_server_each_listen, (void*)pthreadArgPass)) < 0) {
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
	//Thread created

    while(1) {
    	//First check if the connection is still standing.
    	//The listening thread changes the bytes_recvd value to zero
    	//if the connection got closed from the client
		sem_wait(&sem_bytes_recvd);
		if(bytes_recvd == 0) {
			printf("\tThread #%d - Connection: Client disconnected\n", thread_index);
			fflush(stdout);
			sem_post(&sem_bytes_recvd);
			goto ConnectionClosed;
		} else if(bytes_recvd == -1) {
			fprintf(stderr, "\tThread #%d - Connection: recv failed\n", thread_index);
			sem_post(&sem_bytes_recvd);
			goto ConnectionClosed;
		}
		sem_post(&sem_bytes_recvd);
		//To remember:
		//The recv function delivers a Zero if other Site closed the connection

		//Get the Noise of the WLAN-Channel between me here and the client
		//and check if it has changed much
		//if yes, for sure, then send it to client
		getOnlyNoiseByFreq(&getnoiseargs,sktctr,cmd);
    	if((getnoiseargs.noise < (noiseold-noisetolerance)) || (getnoiseargs.noise > (noiseold+noisetolerance))) {
    		msg_2b[0] = ADAPTMSGTYPE_NOISE;
    		msg_2b[1] = getnoiseargs.noise;
    		printf(ANSI_COLOR_RED);
    		printf("   -->Thread #%d - Sending Noise: %d", thread_index, getnoiseargs.noise);
    		printf(ANSI_COLOR_RESET);
    		printf("\n");
    		err = senddetermined(s,msg_2b,2);
    		noiseold = getnoiseargs.noise;
    	}

    	//Watch for changes of the own TX Power. If it has changed: Send to Client
    	//TODO: Better without ioctl
//    	ioctl_get_txpower(&owntxpowi);
    	owntxpow=(char)owntxpowi;
    	if(owntxpow != owntxpowold) {
    		msg_2b[0] = ADAPTMSGTYPE_TXPOWER;
    		msg_2b[1] = owntxpow;
    		printf(ANSI_COLOR_RED);
    		printf("   -->Thread #%d - Sending TX Power: %d", thread_index, owntxpow);
    		printf(ANSI_COLOR_RESET);
    		printf("\n");
    		err = senddetermined(s,msg_2b,2);
    		owntxpowold=owntxpow;
    	}

    	//Wait a bit before next check
		struct timespec remainingdelay;
		remainingdelay.tv_sec = 10;
		remainingdelay.tv_nsec = 0;
		do {
			err = nanosleep(&remainingdelay, &remainingdelay);
		} while (err<0);

		//For sure it would be best if we could subscribe on the Kernel for an Event
		//which tells us that the TX Power has changed. But as far as i know (and at
		//the current Point of Time) this isn't possible...
    }
    ConnectionClosed:


	printf("\tThread #%d - Connection: Terminating.\n", thread_index);
	printf("\tReady for new Connection on Thread Place #%d. Awaiting...\n", thread_index);
	// Get the place in the thread-referencing pthread_t array free
	// to be used with another, new thread
	//Semaphore for whole array not necessary here, because we have a simple write here
	//(which is a atomic function...)
	//But in Combination with other nice things (e.g. )
	//a Mutex-handling here makes some other functions a bit more smooth
	//Semaphore for own array-entry makes things pretty better
	//Check comment in the thread-pseudo-main (keep_txpower_min_server)
			//For the listening thread nothing like this is necessary
			//Because this thread here can only come to this point, when the
			//listener is already terminated
//			pthread_cancel(server_each_listen);
	sem_wait(&atxpsem[thread_index]);
//	sem_wait(&atxpsem[THREADS_CLIENTS_MAX]);
	threads_adapttxpow[thread_index]=0;
//	sem_post(&atxpsem[THREADS_CLIENTS_MAX]);
	sem_post(&atxpsem[thread_index]);
//	pthread_exit(NULL);

	return err;
}
//For Access Point:
static int keep_txpower_min_server (struct nl80211_state *sktctr, struct CommandContainer *cmd, int argc, char **argv) {
//This handles to create a thread for each connected client.
//You could also do this with Combination of setpgrp() and fork()
//But i have choosen a multi-threaded concept.
	printf("\nWorking now as Access-Point.\n");

// Just to know: From the Communication Point of this also works as a server
// He waits for an incoming Transmission from any client to start its adaption.
// Then this here creates a socket and thread for each one.

	int s; /* connected socket descriptor */
	int ls; /* listen socket descriptor */
	struct hostent *hp; /* pointer to host info for remote host */
	struct servent *sp; /* pointer to service information */
	struct linger linger = {1,1};
		//allow lingering, graceful close;
		//used when setting SO_LINGER

	long timevar; //contains time returned by time()
	char *ctime(); //declare time formatting routine

	struct sockaddr_in myaddr_in; //for local socket address
	struct sockaddr_in peeraddr_in; //for peer socket address

	pthread_t threads_adapttxpow[THREADS_CLIENTS_MAX]; //to store the references to active connections
	//Don't forget the Mutex-handling on this thing, if the necessity comes in place for you!

	int err;
	int addrlen, i;
	char *msg;

	printf("\nGetting ready to listen for incoming connections...\n");
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in));
	memset(threads_adapttxpow, 0, sizeof(threads_adapttxpow));
	/* Set up address structure for the listen socket. */
	printf("\tsockaddr_in struct for port listening...\n");
	myaddr_in.sin_family = AF_INET;
	printf("\t...Socket Family to: %d\n", myaddr_in.sin_family);
	printf("\t\t-To Check->Should be: %d (AF_INET)\n", AF_INET);
	/* The server should listen on the wildcard address,
	* rather than its own internet address. This is
	* generally good practice for servers, because on
	* systems which are connected to more than one
	* network at once will be able to have one server
	* listening on all networks at once. Even when the
	* host is connected to only one network, this is good
	* practice, because it makes the server program more
	* portable.
	*/
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	printf("\t...Listening to wildcard-address: %d\n", myaddr_in.sin_addr.s_addr);

/* Beispiel um die Portnummer in der Datei
 * /etc/services
 * aus einem Eintrag wie
 * example 22375/tcp
 * auszulesen

			//Find the information for the “example” server
			//in order to get the needed port number.
			sp = getservbyname ("example", "tcp");
			if (sp == NULL) {
			fprintf(stderr, "%s: host not found", argv[0]);
			exit(1);
			}
			myaddr_in.sin_port= sp->s_port;
*/
	myaddr_in.sin_port = PORTADAPTTXPOWCLIENT;
	printf("\t...Port number: %d\n", myaddr_in.sin_port);

	printf("\tCreating listening socket...\n");
	ls = socket (AF_INET, SOCK_STREAM, 0);
	if (ls == -1) {
		perror(*argv);
		fprintf(stderr, "%s: unable to create socket\n" , *argv);
		exit(1);
	}
	printf("\t...Socket created!\n");
	/* Bind the listen address to the socket. */
	printf("\tBinding listen Address to socket...\n");
	if (bind(ls, &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address\n", argv[0]);
		exit(1);
	}
	printf("\t...bound!\n");
	/* Initiate the listen on the socket so remote users
	* can connect. The listen backlog is set to 5. 20
	*/
	printf("\tInitiating listening on socket...\n");
	if (listen(ls, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n",argv[0]);
		exit(1);
	}
	printf("...Initiation complete!\n");
	printf("\nNOTE: Maximum possible Threads are predefined: %d\n",THREADS_CLIENTS_MAX);
	printf("\t(Which determines the max number of parallel connected clients to adapt the Transmission Power)\n");
	printf("Initialize Semaphores...\n");
	for(i=0;i<=THREADS_CLIENTS_MAX;i++)
		sem_init(&atxpsem[i],0,1);

    addrlen = sizeof(struct sockaddr_in);

	printf("\nWaiting for incoming Connections...\n");
	struct adaptTxPowThreadArgPassing *pthreadArgPass;
	int firstfreethreadindex;
    while( s = accept(ls, (struct sockaddr *)&peeraddr_in, (socklen_t*)&addrlen) ) {
        if (s<0) {
            perror("accept failed");
            return 1;
        }
        puts("");
		puts("Incoming Connection accepted");

		//Following is a malloc. Necessary for passing the needed arguments to the newly
		//created threads. (Every thread needs his own arg passing mem space, so that this
		//isn't modified before the thread read it out... (vgl. Mutex)
		//So remember to free this space inside the thread, when the args are read out
		pthreadArgPass = malloc(sizeof(struct adaptTxPowThreadArgPassing));
		pthreadArgPass->solid_s = s;
		pthreadArgPass->cmd = cmd;
		pthreadArgPass->sktctr = sktctr;

		if((firstfreethreadindex=get_first_free_threads_index(threads_adapttxpow)) == -1) {
			//TODO: Real usable handling of this...
			//Insert something here like a single message to tell the client to try again later
			//or just a connection refuse and for this case the whole handling inside the client.
			printf("PROBLEM: No free Thread-Index present!\n");
			continue;
		}
		pthreadArgPass->thread_index = firstfreethreadindex;
		pthreadArgPass->all_threads_array = threads_adapttxpow;
		printf("Create thread with Index: %d\n", firstfreethreadindex);

		/* Alrighty, another complete re-write of thread-creation and all this
		 * change it to attribute-supported, straightforward-as-detached thread creation
		 * Hence comment all this here out
		 *
//		sem_wait(&atxpsem[THREADS_CLIENTS_MAX]);//Complete Block not needed for now. Maybe in the future...
		sem_wait(&atxpsem[firstfreethreadindex]);//Better block the thread-ID holder until the Thread, corresponding
		//to the contained ID was detached (a bit further down). vgl. the ...-each thread, for corresponding semaphore
		//Because the detach function needs the thread-ID (indeed...)
		//Think about the Problem:
		//Howsoever the thread terminated immediately after creation, before this main here runs to the detach-point
		//The Thread, like i created the whole management, cleaned the all-holding array (Removed his ID-entry to zero
		//to make place for a new one). Then the pthread_detach function can't get the thread-ID.
		//(NOTE: It's not the problem that the thread terminated. That would work like a charm, if the thread-ID
		//would still be present.
		if( pthread_create(&threads_adapttxpow[firstfreethreadindex], NULL, keep_txpower_min_server_each, (void*)pthreadArgPass) < 0) {
//			sem_post(&atxpsem[THREADS_CLIENTS_MAX]);//Complete Block not needed for now. Maybe in the future...
			sem_post(&atxpsem[firstfreethreadindex]);
			perror("could not create thread");
			return 1;
		}
//		sem_post(&atxpsem[THREADS_CLIENTS_MAX]);//Complete Block not needed for now. Maybe in the future...
		pthread_detach(threads_adapttxpow[firstfreethreadindex]);
		sem_post(&atxpsem[firstfreethreadindex]);

//		 //Better not to join... Secure that we not terminate before every thread with the
//		 //pthread_exit as last operation of this func here.
//            err = pthread_join(threads_adapttxpow[0], NULL);
//            if (err) {
//				 printf("ERROR; return code from pthread_join() is %d\n", err);
//				 exit(-1);
//			 }
//            puts("Handler assigned");
 *
 */

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
		if( (err=pthread_create(&threads_adapttxpow[firstfreethreadindex], &tattr, keep_txpower_min_server_each, (void*)pthreadArgPass)) < 0) {
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
    if (s<0) {
        perror("accept failed");
        return 1;
    }

	/* Last thing that main() should do */
	pthread_exit(NULL);
	return 0;
}




int quickTXPowerGet (char *txpowerdest, sem_t *sem_txpow) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	struct iwreq req = {
	.ifr_name = "wlan0",
	};
	memcpy(req.ifr_name,WhatWeWant.interfacename,5);

	// Getting Trasmission power in req
	if(ioctl(sockfd, SIOCGIWTXPOW, &req) == -1) {
		printf("\nError performing SIOCGIWTXPOW on [%s]\nCouldn't get the current Transmission Power!\n", WhatWeWant.interfacename);
		close(sockfd);
		return OPERATION_ERR_STD;
	}
	//Extract the power
	sem_wait(sem_txpow);
	*txpowerdest = (char)req.u.txpower.value;
	sem_post(sem_txpow);

	if (req.u.txpower.flags & IW_TXPOW_MWATT) {
		WhatWeWant.flags = WhatWeWant.flags | WLAN_CON_DATA_TXPOWER_MW;
		WhatWeWant.flags = WhatWeWant.flags & ~WLAN_CON_DATA_TXPOWER_DBM;
	} else {
		WhatWeWant.flags = WhatWeWant.flags & ~WLAN_CON_DATA_TXPOWER_MW;
		WhatWeWant.flags = WhatWeWant.flags | WLAN_CON_DATA_TXPOWER_DBM;
	}
	if (req.u.txpower.flags & IW_TXPOW_RELATIVE) {
		WhatWeWant.flags = WhatWeWant.flags & ~WLAN_CON_DATA_TXPOWER_MW;
		WhatWeWant.flags = WhatWeWant.flags & ~WLAN_CON_DATA_TXPOWER_DBM;
	}


	close(sockfd);

	return 0;
}
int quickTXPowerSet(struct nl80211_state *sktctr, struct CommandContainer *cmd,int powertoset) {
	int err=0;
	cmd->cmd = NL80211_CMD_SET_WIPHY;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	static enum nl80211_tx_power_setting txsettype;
	cmd->nl_msg_flags = 0;
	cmd->callbackToUse = NULL;
	txsettype = NL80211_TX_POWER_FIXED;
	static long power;
	power=(long)powertoset;
	prepareAttribute(cmd, NL80211_ATTR_WIPHY_TX_POWER_LEVEL, &power);
	prepareAttribute(cmd, NL80211_ATTR_WIPHY_TX_POWER_SETTING, &txsettype);
	err = send_with_cmdContainer(sktctr, 0, 0, cmd);
	if (err < 0) {
		printf("\tERROR: set txpower\n\tInvalid Value!\n\tKernel can't work with it!\n\tError: %d\n\tERRNO: %d\n\n",err,errno);
	} else {
		switch (err) {
		case 0:
			return 0;
			break;
		case MAIN_ERR_BAD_CMDLINE:
			printf("\tERROR: set txpower\n\tInvalid Value!\n\tCouldn't put it in a message!\n\n");
			return MAIN_ERR_BAD_CMDLINE;
			break;
		case MAIN_ERR_FEW_CMDS:
//				printf("\nNot enough Arguments after »set« passed\n\n");
			return MAIN_ERR_FEW_CMDS;
			break;
		default:
			fprintf(stderr, "\tERROR: Unsupported error-code from the set-txpower-function delivered!\n\n");
			return MAIN_ERR_STD;
			break;
		}
	}
	return err;
}
static int GET_ONLY_MAC_cb(struct nl_msg* msg, void* arg) {
		struct nlmsghdr *got_hdr = nlmsg_hdr(msg);
		struct nlattr *got_attr[NL80211_ATTR_MAX + 1];
		struct nlattr *got_bss[NL80211_BSS_MAX + 1];
		static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
				[NL80211_BSS_BSSID] = { }, };
		char device[IFNAMSIZ];
		if (got_hdr->nlmsg_type != expectedId) {
			return NL_STOP;
		}
		struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
		nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
				genlmsg_attrlen(gnlh, 0), NULL);
		if (!got_attr[NL80211_ATTR_BSS]) {
			fprintf(stderr, "bss info missing!\n");
			return NL_SKIP;
		}
		if (nla_parse_nested(got_bss, NL80211_BSS_MAX, got_attr[NL80211_ATTR_BSS],
				bss_policy)) {
			fprintf(stderr, "failed to parse nested attributes!\n");
			return NL_SKIP;
		}
		if (!got_bss[NL80211_BSS_BSSID]) {
	//		fprintf(stderr, "No BSSID delivered!\n");
			return NL_SKIP;
		}
		char seconddevice[sizeof(device)];
		if_indextoname(nla_get_u32(got_attr[NL80211_ATTR_IFINDEX]), device);
		snprintf(seconddevice, sizeof(device), "%s", device);
		snprintf(device, sizeof(device), "%s", WhatWeWant.interfacename);
		if (strcmp(device,seconddevice)) {
			fprintf(stderr, "Delivered Information is not about the enquired Interface!\n");
			return NL_SKIP;
		}
		/*
		 * Output
		 * Write the stuff in the Storage-struct
		 */
		if (got_bss[NL80211_BSS_BSSID]) {
			memset(WhatWeWant.MAC_ConnectedTo, 0, 20);
			memcpy(WhatWeWant.MAC_ConnectedTo, nla_data(got_bss[NL80211_BSS_BSSID]), 20);
		}
		return 0;
}
static int getOnlyMAC(struct nl80211_state *sktctr, struct CommandContainer *cmd) {
	int err;
	cmd->cmd = NL80211_CMD_GET_SCAN;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	cmd->nl_msg_flags = NLM_F_DUMP;
	cmd->callbackToUse = GET_ONLY_MAC_cb;
	err = send_with_cmdContainer(sktctr, 0, 0, cmd);
	return err;
}
signed char recsiglvl;
static int GET_ONLY_SIGLVL_cb(struct nl_msg* msg, void* arg) {
	struct nlmsghdr *got_hdr = nlmsg_hdr(msg);
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];
	struct nlattr *got_sta_info[NL80211_STA_INFO_MAX + 1];
	static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
		[NL80211_STA_INFO_SIGNAL] = { .type = NLA_U8 },
	};
	if (got_hdr->nlmsg_type != expectedId) {
		// what is this??
		return NL_STOP;
	}
//    struct genlmsghdr *gnlh = (struct genlmsghdr*) nlmsg_data(got_hdr);
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
	if (!got_attr[NL80211_ATTR_STA_INFO]) {
		fprintf(stderr, "sta stats missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(got_sta_info, NL80211_STA_INFO_MAX, got_attr[NL80211_ATTR_STA_INFO], stats_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}
	if (got_sta_info[NL80211_STA_INFO_SIGNAL]) {
//		WhatWeWant.stainf.SigLvl = (int8_t)nla_get_u8(got_sta_info[NL80211_STA_INFO_SIGNAL]);
		recsiglvl = (signed char)nla_get_u8(got_sta_info[NL80211_STA_INFO_SIGNAL]);
	}
	return 0;
}
static int getOnlySignalLvl(struct nl80211_state *sktctr, struct CommandContainer *cmd) {
	int err;
	cmd->cmd = NL80211_CMD_GET_STATION;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	prepareAttribute(cmd, NL80211_ATTR_MAC, WhatWeWant.MAC_ConnectedTo);
	cmd->nl_msg_flags = 0;
	cmd->callbackToUse = GET_ONLY_SIGLVL_cb;
	err = send_with_cmdContainer(sktctr, 0, 0, cmd);
	return err;
}
static int GET_ONLY_FREQ_cb(struct nl_msg* msg, void* arg) {
	struct nlmsghdr *got_hdr = nlmsg_hdr(msg);
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];
	struct nlattr *got_bss[NL80211_BSS_MAX + 1];
	static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
			[NL80211_BSS_TSF] = { .type = NLA_U64 },
			[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
			[NL80211_BSS_BSSID] = { },
			[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
			[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
			[NL80211_BSS_INFORMATION_ELEMENTS] = { },
			[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
			[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
			[NL80211_BSS_STATUS] = { .type = NLA_U32 }, };
	char device[IFNAMSIZ];
	if (got_hdr->nlmsg_type != expectedId) {
		return NL_STOP;
	}
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);

	if (!got_attr[NL80211_ATTR_BSS]) {
		fprintf(stderr, "bss info missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(got_bss, NL80211_BSS_MAX, got_attr[NL80211_ATTR_BSS],
			bss_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}
	if (!got_bss[NL80211_BSS_BSSID]) {
		return NL_SKIP;
	}
	if (!got_bss[NL80211_BSS_STATUS]) {
		return NL_SKIP;
	}
	char seconddevice[sizeof(device)];
	if_indextoname(nla_get_u32(got_attr[NL80211_ATTR_IFINDEX]), device);
	snprintf(seconddevice, sizeof(device), "%s", device);
	snprintf(device, sizeof(device), "%s", WhatWeWant.interfacename);
	if (strcmp(device,seconddevice)) {
		fprintf(stderr, "Delivered Information is not about the enquired Interface!\n");
		return NL_SKIP;
	}
// * Output *
	if (got_attr[NL80211_ATTR_IFTYPE])
		WhatWeWant.type = nla_get_u32(got_attr[NL80211_ATTR_IFTYPE]);
	if (got_attr[NL80211_ATTR_WIPHY_FREQ])
		WhatWeWant.frequency = nla_get_u32(got_attr[NL80211_ATTR_WIPHY_FREQ]);
	if (got_bss[NL80211_BSS_FREQUENCY])
		WhatWeWant.frequency = nla_get_u32(got_bss[NL80211_BSS_FREQUENCY]);
	return 0;
}
static int getOnlyFreq(struct nl80211_state *sktctr, struct CommandContainer *cmd) {
	int err;
	cmd->cmd = NL80211_CMD_GET_SCAN;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	cmd->nl_msg_flags = NLM_F_DUMP;
	cmd->callbackToUse = GET_ONLY_FREQ_cb;
	err = send_with_cmdContainer(sktctr, 0, 0, cmd);

	return err;
}
void extract_only_bitrate(struct StationInfo *dest, struct nlattr *bitrate_attr)//, char *bitrbuffer, int buflen)
{
	struct nlattr *got_bitrate[NL80211_RATE_INFO_MAX + 1];
	static struct nla_policy rate_policy[NL80211_RATE_INFO_MAX + 1] = {
		[NL80211_RATE_INFO_BITRATE] = { .type = NLA_U16 },
		[NL80211_RATE_INFO_BITRATE32] = { .type = NLA_U32 },
		[NL80211_RATE_INFO_MCS] = { .type = NLA_U8 },
		[NL80211_RATE_INFO_40_MHZ_WIDTH] = { .type = NLA_FLAG },
		[NL80211_RATE_INFO_SHORT_GI] = { .type = NLA_FLAG },
	};
	if (nla_parse_nested(got_bitrate, NL80211_RATE_INFO_MAX, bitrate_attr, rate_policy)) {
		fprintf(stderr, "failed to parse nested rate attributes!");
		return;
	}
	if (got_bitrate[NL80211_RATE_INFO_BITRATE32])
		dest->bitrate = nla_get_u32(got_bitrate[NL80211_RATE_INFO_BITRATE32]);
	else if (got_bitrate[NL80211_RATE_INFO_BITRATE])
		dest->bitrate = nla_get_u16(got_bitrate[NL80211_RATE_INFO_BITRATE]);
	(dest->bitrate) = (dest->bitrate)/10;
}
//int debuggothrough3 = 0;
static int GET_ONLY_BITRATE_cb(struct nl_msg* msg, void* arg) {
//	debuggothrough3++;
//	printf("Debug-Times through Do_CMD_GET_STATION_cb: %d\n\n",debuggothrough3);
	struct nlmsghdr *got_hdr = nlmsg_hdr(msg);
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];
	struct nlattr *got_sta_info[NL80211_STA_INFO_MAX + 1];
	struct nlattr *got_sta_bss[NL80211_STA_BSS_PARAM_MAX + 1];
	static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
		[NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
	};
	if (got_hdr->nlmsg_type != expectedId) {
		// what is this??
		return NL_STOP;
	}
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);
	if (!got_attr[NL80211_ATTR_STA_INFO]) {
		fprintf(stderr, "sta stats missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(got_sta_info, NL80211_STA_INFO_MAX, got_attr[NL80211_ATTR_STA_INFO], stats_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}
	// Output
	if (got_sta_info[NL80211_STA_INFO_TX_BITRATE]) {
		extract_only_bitrate(&(WhatWeWant.stainf), got_sta_info[NL80211_STA_INFO_TX_BITRATE]);
	}
	return 0;
}
static int getOnlyBitrate(struct nl80211_state *sktctr, struct CommandContainer *cmd) {
	//To Notice: This function doesn't store the Bitrate like it gets it form the Kernel
	//it instantanously applies a div 10.
	int err;
	cmd->cmd = NL80211_CMD_GET_STATION;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	prepareAttribute(cmd, NL80211_ATTR_MAC, WhatWeWant.MAC_ConnectedTo);
	cmd->nl_msg_flags = 0;
	cmd->callbackToUse = GET_ONLY_BITRATE_cb;
	err = send_with_cmdContainer(sktctr, 0, 0, cmd);

	return err;
}
//For Clients:
struct adaptTxPowClientThreadArgPassing {
	int solid_s; //handy for putting socket descr. to thread
	int *txpowerserver;
//	char *txpschanged;
	sem_t *sem_txps;
	int *bytes_recvd;
	sem_t *sem_bytes_recvd;
	int *noiseserver;
	sem_t *sem_noiseserver;
};
void *keep_txpower_min_client_listen (void* arg) {
	int s = ((struct adaptTxPowClientThreadArgPassing *)arg)->solid_s;
	char *txpowerserver = ((struct adaptTxPowClientThreadArgPassing *)arg)->txpowerserver;
//	char *txpschanged = ((struct adaptTxPowClientThreadArgPassing *)arg)->txpschanged;
	sem_t *sem_txps = ((struct adaptTxPowClientThreadArgPassing *)arg)->sem_txps;
	int *bytes_recvd = ((struct adaptTxPowClientThreadArgPassing *)arg)->bytes_recvd;
	sem_t *sem_bytes_recvd = ((struct adaptTxPowClientThreadArgPassing *)arg)->sem_bytes_recvd;
	char *noiseserver = ((struct adaptTxPowClientThreadArgPassing *)arg)->noiseserver;
	sem_t *sem_noiseserver = ((struct adaptTxPowClientThreadArgPassing *)arg)->sem_noiseserver;
	//Free the arguments passing memory space
	free(arg);
	int err;
	int pthreadoldcancelstate;
	char txpowerserverlocal;
	char noiseserverlocal;
	int bytes_recvd_local;
	char msg_recvd[RECVBUFFSIZEADAPTTXPOW];

    while( (bytes_recvd_local = recv(s , msg_recvd , RECVBUFFSIZEADAPTTXPOW , 0)) > 0 )
    {
    	msg_recvd[bytes_recvd_local]= '\0';
		printf(ANSI_COLOR_RED);
    	printf("\n-->Message from Server: %d Bytes.",bytes_recvd_local);
		printf(ANSI_COLOR_RESET);
		printf("\n");
    	switch (msg_recvd[0]) {
    	case ADAPTMSGTYPE_TXPOWER:
        	txpowerserverlocal=(signed int)msg_recvd[1];
    		printf(ANSI_COLOR_RED);
        	printf("-->Client-Thread: Received TX Power from Server: %d\n",txpowerserverlocal);
    		printf(ANSI_COLOR_RESET);
    		printf("\n");
        	err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
        	sem_wait(sem_txps);
        	*txpowerserver=txpowerserverlocal;
    //    	*txpschanged=1;
        	sem_post(sem_txps);
        	err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);
    		break;
    	case ADAPTMSGTYPE_NOISE:
    		noiseserverlocal=(signed int)msg_recvd[1];
    		printf(ANSI_COLOR_RED);
        	printf("-->Client-Thread: Received Noise from Server: %d\n",noiseserverlocal);
    		printf(ANSI_COLOR_RESET);
    		printf("\n");
        	err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
    		sem_wait(sem_noiseserver);
        	*noiseserver=noiseserverlocal;
    		sem_post(sem_noiseserver);
        	err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);
    		break;
    	default:
    		printf(ANSI_COLOR_RED);
        	printf("-->Client-Thread: I can't read the message o.O\n",txpowerserverlocal);
    		printf(ANSI_COLOR_RESET);
    		printf("\n");
    		break;
    	}
    }

	err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
    sem_wait(sem_bytes_recvd);
    *bytes_recvd=bytes_recvd_local;
    sem_post(sem_bytes_recvd);
	err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);

	return err;
}
static int keep_txpower_min_client (struct nl80211_state *sktctr, struct CommandContainer *cmd, int argc, char **argv, enum nl80211_iftype iftype, long txpowermax, char *cfg_file) {
	printf("\nWorking now as Client, whos transmission power is getting adapted...\n");
	//This sweetie is working as client from the communication point of view.
	//It will start the communication (and set process)
	//while the Parent-process on the WLAN-Access Point side is listening until
	//this cutie here will start the whole thing

	int err;
	int s; /* connected socket descriptor */
	struct hostent *hp; /* pointer to host info for remote host */
	struct servent *sp; /* pointer to service information */
	struct linger linger = {1,1};
		//allow lingering, graceful close;
		//used when setting SO_LINGER

	long timevar; //contains time returned by time()
	char *ctime(); //declare time formatting routine

	struct sockaddr_in myaddr_in; //for local socket address
	struct sockaddr_in peeraddr_in; //for peer socket address

	int addrlen,i;
	char buf[50]; //Try with max 50 bytes messages for now

	//First get the MAC Address of the associated Access-Point
	//Needed to get the received SignalLevel from the Kernel
	getOnlyMAC(sktctr, cmd);

// Some Kernel needs different Values for the Transmission Power Set
// i.e. To set the tx power to 20 dBm, some Kernel want to get 20, others want 2000
// thus ascertain the needed factor
	int txpowersetfactor=1;
	setTXPowerGetFactor(&txpowersetfactor,sktctr);

	printf("\nSetting up Connection to Access Point...\n");
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in));
	/* Set up the peer address to which we will connect. */
	printf("\tsockaddr_in struct for connection...\n");
	peeraddr_in.sin_family = AF_INET;
	printf("\t...Socket Family to: %d\n", peeraddr_in.sin_family);
	printf("\t\t-To Check->Should be: %d (AF_INET)\n", AF_INET);
	// Get the host information for the server / AP
	switch(iftype) {
	case NL80211_IFTYPE_ADHOC:
		;
		//A little bit more fancy. Slight change to get the »server«-address as against the normal station mode
		//TODO: Den IP Kram endlich mal ohne ioctl erledigen
//		get_access_point_local_ip(&peeraddr_in);
		//Now close one eye and don't look that close on this little workaround
		//Assume that the station, which runs in server mode ist the one, which startet the adhoc-net
		//So it has the IP-Address that ends on -.-.-.1
//		*(((char *)&(peeraddr_in.sin_addr.s_addr))+3)=1;
		break;
	//The next coming ones should all work the same. So just leave the break and let the cases run through until the default.
	case NL80211_IFTYPE_STATION:
		//The »Standart«-case. Normal station here, which communicates with an access point
//		break;
	case NL80211_IFTYPE_AP:
		//Should just work like the station
//		break;
	default:
		;
		//TODO: Den IP Kram endlich mal ohne ioctl erledigen
//		get_access_point_local_ip(&peeraddr_in);
		break;
	}
	char printaddrv4[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &(peeraddr_in.sin_addr), printaddrv4, INET_ADDRSTRLEN);
	printf("\t...Found (local) IP Address of AP: %s\n", printaddrv4);
	peeraddr_in.sin_port = PORTADAPTTXPOWCLIENT;
	printf("\t...Port number: %d\n", peeraddr_in.sin_port);
	/* Create the socket. */
	printf("\tCreating Socket...\n");
	s = socket (AF_INET, SOCK_STREAM, 0);
	if (s == -1) {
		perror(argv[0]);
		fprintf(stderr,"%s: unable to create socket\n", argv[0]);
		exit(1);
	}
	printf("\t...Socket created!\n");
	printf("\tNow connecting...\n");
	if (connect(s, &peeraddr_in, addrlen=sizeof(struct sockaddr_in)) ==-1) {
		perror(argv[0]);
		//TODO: For sure, to do this better you should involve the Netmask...
		printf("\n%s: ERROR: Couldn't connect to specified Peer Address.\nSweep the Subnet for running TX Power Adaption Server...\n",argv[0]);
		*(((char *)&(peeraddr_in.sin_addr.s_addr))+3)=0;
		while(connect(s, &peeraddr_in, addrlen=sizeof(struct sockaddr_in)) ==-1){
			perror(argv[0]);
			(*(((char *)&(peeraddr_in.sin_addr.s_addr))+3))++;
		}
//		exit(1);
	}
	inet_ntop(AF_INET, &(peeraddr_in.sin_addr), printaddrv4, INET_ADDRSTRLEN);
	printf("\nAfter all i could connect to remote:\n\t%s\n\n", printaddrv4);
	/* Since the connect call assigns a random address
	* to the local end of this connection, let's use
	* getsockname to see what it assigned. Note that
	* addrlen needs to be passed in as a pointer,
	* because getsockname returns the actual length
	* of the address.
	*/
//	printf("\t...connection established!\n");

	if (getsockname(s, &myaddr_in, &addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n",
		argv[0]);
		exit(1);
	}
	/* Print out a startup message for the user. */
	time(&timevar);
	/* The port number must be converted first to
	* host byte order before printing. On most hosts,
	* this is not necessary, but the ntohs() call is
	* included here so this program could easily be
	* ported to a host that does require it.
	*/
	printf("Connected to %s on port %u at %s\n","TODO",ntohs(myaddr_in.sin_port),ctime(&timevar));
	printf("\n");





	//Connection established. Now do the real stuff...
//	signed char mato = MATO;
//	signed char matu = MATU;
//	signed char matc = MATC;
	//Reading the Target Values out of cfg-File
	signed int matdiffo;
	signed int matdiffu;
	signed int matc;
	signed int snrdiffo;
	signed int snrdiffu;
	signed int snrc;
    FILE *cfgf;
    cfgf = fopen(cfg_file, "r");
    if (!cfgf) {
    	switch(errno) {
    		case EACCES:
    			printf("\tERROR: Couldn't open cfg-File!\n\t\tReason:Permission denied!\n");
    		break;
    		case ENOENT:
    			printf("\tERROR: Cfg-File doesn't exist! Shouldn't occur at this point...\n");
    		break;
    		default:
    		  	fprintf(stderr, "Ups, seems like we've encountered a case, which isn't caught yet :o(");
    		   	return MAIN_ERR_FUNC_INCOMPLETE;
    		break;
    	}
    } else {
    	char buf[100];
    	memset(buf,0,sizeof(buf));
    	while(1) {
    		int loopcnt;
    		int readval;
			if ((readval = getc(cfgf)) == EOF) {
			 printf("\t...Read completely through config-file.\n");//End of file reached
			 break;
			}
			buf[0]=(char)readval;
			if (buf[0] == '#') {
				for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
					if (((buf[loopcnt] = getc(cfgf)) == '=') || (buf[loopcnt] == EOF)) {
						buf[loopcnt]='\0';
						break;
					}
				}
			}
			if(strcmp(buf,"adapttargetrss")==0){
				for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
					if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) {
						buf[loopcnt]='\0';
						break;
					}
				}
				char *endpoint;
				matc = (char)strtol(buf, &endpoint, 10);
				if (*endpoint) {
					printf("ERROR: Unsupported Format in cfg-File at:\n\tDeviceMaxTXPower\n");
					return MAIN_ERR_BAD_CMDLINE;
				}
			} else
			if(strcmp(buf,"adapttargetrssupperdiff")==0){
				for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
					if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) {
						buf[loopcnt]='\0';
						break;
					}
				}
				char *endpoint;
				matdiffo = (char)strtol(buf, &endpoint, 10);
				if (*endpoint) {
					printf("ERROR: Unsupported Format in cfg-File at:\n\tDeviceMaxTXPower\n");
					return MAIN_ERR_BAD_CMDLINE;
				}
			} else
			if(strcmp(buf,"adapttargetrsslowerdiff")==0){
				for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
					if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) {
						buf[loopcnt]='\0';
						break;
					}
				}
				char *endpoint;
				matdiffu = (char)strtol(buf, &endpoint, 10);
				if (*endpoint) {
					printf("ERROR: Unsupported Format in cfg-File at:\n\tDeviceMaxTXPower\n");
					return MAIN_ERR_BAD_CMDLINE;
				}
			} else
			if(strcmp(buf,"adapttargetsnr")==0){
				for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
					if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) {
						buf[loopcnt]='\0';
						break;
					}
				}
				char *endpoint;
				snrc = (char)strtol(buf, &endpoint, 10);
				if (*endpoint) {
					printf("ERROR: Unsupported Format in cfg-File at:\n\tDeviceMaxTXPower\n");
					return MAIN_ERR_BAD_CMDLINE;
				}
			} else
			if(strcmp(buf,"adapttargetsnrupperdiff")==0){
				for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
					if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) {
						buf[loopcnt]='\0';
						break;
					}
				}
				char *endpoint;
				snrdiffo = (char)strtol(buf, &endpoint, 10);
				if (*endpoint) {
					printf("ERROR: Unsupported Format in cfg-File at:\n\tDeviceMaxTXPower\n");
					return MAIN_ERR_BAD_CMDLINE;
				}
			} else
			if(strcmp(buf,"adapttargetsnrlowerdiff")==0){
				for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
					if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) {
						buf[loopcnt]='\0';
						break;
					}
				}
				char *endpoint;
				snrdiffu = (char)strtol(buf, &endpoint, 10);
				if (*endpoint) {
					printf("ERROR: Unsupported Format in cfg-File at:\n\tDeviceMaxTXPower\n");
					return MAIN_ERR_BAD_CMDLINE;
				}
			}
    	}
    }
    fclose(cfgf);
//	signed int matdiffo = MATDIFFO;
//	signed int matdiffu = MATDIFFU;
//	signed int matc = MATDIFFC;
//	signed int snrdiffo = SNRDIFFO;
//	signed int snrdiffu = SNRDIFFU;
//	signed int snrc = SNRC;
	signed int mato = matc+matdiffo;
	signed int matu = matc-matdiffu;
	signed int snro = snrc+snrdiffo;
	signed int snru = snrc-snrdiffu;

	signed int txpowerserver;
	signed int txpowdrop;
	signed int rssatserver;
	signed int noiseserver;
	signed int snrserver;
	signed int txpowtoset;
	signed int txpowtosetsnr;
	char owntxpow;
	sem_t sem_noiseserver;
	sem_init(&sem_noiseserver,0,1);
	sem_t sem_owntxpow;
	sem_init(&sem_owntxpow,0,1);
//	char txpschanged=0;
	sem_t sem_txps;
	sem_init(&sem_txps,0,1);
	int bytes_recvd=1;
	sem_t sem_bytes_recvd;
	sem_init(&sem_bytes_recvd,0,1);

	getOnlyFreq(sktctr, cmd);
	printf("\nClient: Sending on Frequency: %d\n",WhatWeWant.frequency);
	err = senddetermined(s,&(WhatWeWant.frequency),sizeof(int));

	char msg_recvd[RECVBUFFSIZEADAPTTXPOW];
	bytes_recvd = recv(s , msg_recvd , RECVBUFFSIZEADAPTTXPOW , 0);
	msg_recvd[bytes_recvd]= '\0';
	txpowerserver=(signed int)msg_recvd[0];
	noiseserver=(signed int)msg_recvd[1];
	printf("\nClient: Received from Server: (Bytes of msg: %d)\n\tTX Power: %d\n\tNoise: %d\n",bytes_recvd,txpowerserver,noiseserver);

	//Start the listening Thread
	//it works like a Data Producer, while the Process (this function here) works like a
	//Data Consumer.
	//The Thread receives the message, if the Server sends one with its TXPower
	//The Server does this, if he changed his TXPower.
	//Then the Thread writes this in a shared Variable an also writes in another shared variable
	//that the TXPower from the Server has changed.

	pthread_t client_prod_thr;
	struct adaptTxPowClientThreadArgPassing *pthreadArgPass;

	pthreadArgPass = malloc(sizeof(struct adaptTxPowClientThreadArgPassing));
	pthreadArgPass->solid_s = s;
	pthreadArgPass->txpowerserver=&txpowerserver;
	pthreadArgPass->sem_txps=&sem_txps;
	pthreadArgPass->noiseserver=&noiseserver;
	pthreadArgPass->sem_noiseserver=&sem_noiseserver;
	pthreadArgPass->bytes_recvd=&bytes_recvd;
	pthreadArgPass->sem_bytes_recvd=&sem_bytes_recvd;

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
	//secure detached-state for the threads over attributed creation
	if( (err=pthread_create(&client_prod_thr, &tattr, keep_txpower_min_client_listen, (void*)pthreadArgPass)) < 0) {
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
	//Thread created. It runs and changes the txpowerserver
	//No more need to bother about it.
	//Now handle the Adaption of the own TX Power.
	
	
	//Creating Logging-File
	//Setting up first Line
	    FILE *logfatxp;
		char log_path_prefix[] = LOG_FILE_PATH_PREFIX_ADAPTTXPOWER;
		int log_path_leng = sizeof(log_path_prefix);
		//Remember: sizeof(string) also contains the end-of-string sign \0
		char log_file[log_path_leng+23];//Name like log_adapttxpower_2014-12-28_13:45:59.log
	if (logatxp & LOG_ATXP_BASIC) {
		time_t logdateraw;
		time(&logdateraw);
		struct tm *logdate;
		logdate = localtime(&logdateraw);
		//Calculate and set log_file_name
//		time_t temptime;
		memcpy(log_file,log_path_prefix,log_path_leng);
		//Write the Date and Time
		char *logwrt=log_file+log_path_leng-1;
		snprintf(logwrt, 24, "%04d-%02d-%02d_%02d:%02d:%02d.log",(logdate->tm_year)+1900,(logdate->tm_mon)+1,(logdate->tm_mday),(logdate->tm_hour),(logdate->tm_min),(logdate->tm_sec));
		//Write the File-Ending
//		log_file[log_path_leng+18]='.';
//		log_file[log_path_leng+19]='l';
//		log_file[log_path_leng+20]='o';
//		log_file[log_path_leng+21]='g';
//		log_file[log_path_leng+22]='\0';
	    logfatxp = fopen(log_file, "w");
	    rewind(logfatxp);
	    fflush(logfatxp);
		if(!logfatxp){
			fprintf(stderr, "\t\tERROR: Couldn't create new log-file!\n");
			return FILE_ERR_PERMISSION_DENIED;
		}
	    rewind(logfatxp);
	    fflush(logfatxp);
		fprintf(logfatxp, "RssServer\tTxPowerServer\tPathLoss\tTXPowerOwn\tRssOwnAtServer\tBitrate\n");
		fclose(logfatxp);
	}

	char nothingtoset=0;
	while(1){
		sem_wait(&sem_bytes_recvd);
		if(bytes_recvd == 0) {
			printf(ANSI_COLOR_BLUE);
	    	printf("<^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^>\n");
	    	printf("<______________________________________________________________>");
			printf(ANSI_COLOR_RESET);
			printf("\n");
			printf("\tConnection: Server disconnected!\n");
			fflush(stdout);
			sem_post(&sem_bytes_recvd);
			goto ConnectionClosedByServer;
		} else if(bytes_recvd == -1) {
			printf(ANSI_COLOR_BLUE);
	    	printf("<^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^>\n");
	    	printf("<______________________________________________________________>");
			printf(ANSI_COLOR_RESET);
			printf("\n");
			fprintf(stderr, "\tConnection: recv failed\n");
			sem_post(&sem_bytes_recvd);
			goto ConnectionClosedByServer;
		}
		sem_post(&sem_bytes_recvd);

		printf(ANSI_COLOR_BLUE);
    	printf("<||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||>\n");
    	printf("<||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||>");
		printf(ANSI_COLOR_RESET);
		printf("\n");

		//Calculate to what Value the RSS on the Access-Point would lead us to
    	getOnlySignalLvl(sktctr,cmd);
    	printf("RSS for Server: %d dBm (Sends with %d dBm)\n",recsiglvl,txpowerserver);
    	sem_wait(&sem_txps);
    	txpowdrop=txpowerserver-recsiglvl;
    	sem_post(&sem_txps);
    	printf("Calculated the Drop of Power: %d dBm\n",txpowdrop);
		quickTXPowerGet(&owntxpow,&sem_owntxpow);
    	printf("   Own current TX Power: ");
		printf(ANSI_COLOR_GREEN);
    	printf("%d",owntxpow);
		printf(ANSI_COLOR_RESET);
    	printf(" dBm\n");
		getOnlyBitrate(sktctr, cmd);
    	printf("   current Bitrate of Channel: ");
		printf(ANSI_COLOR_GREEN);
    	printf("%d",WhatWeWant.stainf.bitrate);
		printf(ANSI_COLOR_RESET);
    	printf(" MBit/s\n");
    	rssatserver=owntxpow-txpowdrop;
    	printf("Estimated (calculated) RSS at AP: ");
		printf(ANSI_COLOR_GREEN);
    	printf("%d",rssatserver);
		printf(ANSI_COLOR_RESET);
    	printf(" dBm\n");
    	printf("   Aim at [%d, %d] - If outside, set to: %d dBm\n",matu,mato,matc);
    	if(rssatserver>mato) {
    		//RSS to high, have to decrease
    		txpowtoset=matc+txpowdrop;
    		printf("RSS would set TX Power to %d dBm.\n",txpowtoset);
    	} else if(rssatserver>=matu) {
    		//RSS in Interval, nothing to do
    		printf("RSS thinks TX Power fits. Nothing to do by RSS.\n");
    		txpowtoset=0;
    		nothingtoset++;
    	} else {
    		//RSS to low, have to increase
    		txpowtoset=matc+txpowdrop;
    		printf("RSS would set TX Power to %d dBm.\n",txpowtoset);
    	}
    	printf(">-------------------------------------<\n");
		if (logatxp & LOG_ATXP_BASIC) {
		    logfatxp = fopen(log_file, "a");
			fprintf(logfatxp,"%d\t%d\t%d\t%d\t%d\t%d\n",recsiglvl,txpowerserver,txpowdrop,owntxpow,rssatserver,WhatWeWant.stainf.bitrate);
			fclose(logfatxp);
		}

    	//Now calculate the Value to what the SNR on the Server would us lead to
    	sem_wait(&sem_noiseserver);
    	snrserver=rssatserver-noiseserver;
    	sem_post(&sem_noiseserver);
    	printf(" | Calculated SNR on Server: ");
		printf(ANSI_COLOR_GREEN);
    	printf("%d",snrserver);
		printf(ANSI_COLOR_RESET);
    	printf(" dBm\n");
    	printf(" | \tAim at [%d, %d] - If outside, set to: %d dBm\n",snru,snro,snrc);
    	if(snrserver>snro) {
    		//SNR to high, have to decrease
    		txpowtosetsnr=owntxpow+(snrc-snrserver);
    		printf(" | SNR would set TX Power to %d dBm.\n",txpowtosetsnr);
    	} else if(snrserver>=snru) {
    		//RSS in Interval, nothing to do
    		printf(" | SNR thinks TX Power fits. Nothing to do by SNR.\n");
    		txpowtosetsnr=0;
    		nothingtoset++;
    	} else {
    		//TSS to low, have to increase
    		txpowtosetsnr=owntxpow+(snrc-snrserver);
    		printf(" | SNR would set TX Power to %d dBm.\n",txpowtosetsnr);
    	}
    	printf(">-------------------------------------<\n");

    	//And finally compare them two values and set
    	if (nothingtoset>=2) {
    		printf("Everything fits. Nothing to set.\n");
    		goto NothingToSet;
    	} else if (nothingtoset==0) {
    	//this means both approaches want to set
    	//so we set it to highest of these two values
			if(txpowtosetsnr>txpowtoset){
				printf("SNR leads to greater TX Power.\n");
				txpowtoset=txpowtosetsnr;
			} else {
				printf("RSS leads to greater TX Power.\n");
			}
    	} else {//equal to (nothingtoset==1)
    	//this means one of the two want to set
    	//here we have to take a closer look
    	//if it thinks the TX Power could be lower, than we can't do that
    	//because the other side is satisfied with the current value and told
    	//us concurrently with that, that it NEEDS this amount of Transmission Power.
    	//But if the changing Part wants to increase the TX Power, than
    	//we better should do it.
    		if(txpowtoset>owntxpow) {
    			printf("RSS needs to increase TX Power.\n");
    		} else if(txpowtosetsnr>owntxpow) {
    			printf("SNR needs to increase TX Power.\n");
    			txpowtoset=txpowtosetsnr;
    		} else {
    			//One wants do decrease, therefor do nothing...
    			if(txpowtosetsnr>txpowtoset){
    				printf("Do nothing, because only\nSNR wants to decrease TX Power.\n");
    				txpowtoset=txpowtosetsnr;
    			} else {
    				printf("Do nothing, because only\nRSS wants to decrease TX Power.\n");
    			}
    			goto NothingToSet;
    		}
    	}
    	printf(" Use the highest of both values\n");
    	printf("Calculated own TX Power to set: %d dBm\n",txpowtoset);
    	if(txpowtoset>txpowermax){
    		txpowtoset=txpowermax;
    		printf("Value would be to high. Maximum is %d dBm\n",txpowermax);
    		printf("So use the Maximum\n");
    	} else if(txpowtoset<1) {
    		txpowtoset=1;
    		printf("Value would be to low. Wouldn't make sense.\n");
    		printf("So use 1 dBm\n");
    	}
    	printf("Set own TX Power to: %d dBm\n",txpowtoset);
//    	txpowtoset=txpowtoset*txpowersetfactor;
//		quickTXPowerSet(sktctr,cmd,txpowtoset);
		quickTXPowerSet(sktctr,cmd,txpowtoset*txpowersetfactor);
		NothingToSet:
		nothingtoset=0;


    	//Time between Adaptions
		struct timespec remainingdelay;
		remainingdelay.tv_sec = 5;
		remainingdelay.tv_nsec = 0;
		do {
			err = nanosleep(&remainingdelay, &remainingdelay);
		} while (err<0);
	}








	//Final Connection Closure:
	CloseConnection:
	if (logatxp & LOG_ATXP_BASIC) {
		if (fclose(logfatxp)) {
			fprintf(stderr, "Log-File could not be closed successfully!");
		}
	}

	printf("\nCanceling the Listening Thread.\n");
	while(pthread_cancel(client_prod_thr) != 0) {}
	printf("\n");
	printf("Shutting down the connection...\n");
	/* Now, shutdown the connection for further sends.
	* This causes the server to receive an end-of-file
	* condition after receiving all the requests that
	* have just been sent, indicating that no further
	* requests will be sent.
	*/
	if (shutdown(s, 1) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to shutdown socket\n",argv[0]);
		exit(1);
	}
	/* Start receiving all the replys from the server.
	* This loop will terminate when the recv returns
	* zero, which is an end-of-file condition. This
	* will happen after the server has sent all of its
	* replies, and closed its end of the connection.
	*/
	//Not used for now
	//TODO: Check if used later on...
//	while (i = recv(s, buf, 50, 0)) {
//		if (i == -1) {
//			errout:
//			perror(argv[0]);
//			fprintf(stderr, "%s: error reading result\n",argv[0]);
//			exit(1);
//		}
//	}
	/* The reason this while loop exists is that there
	* is a remote possibility of the above recv returning
	* less than 50 bytes. This is because a recv returns
	* as soon as there is some data, and will not wait for
	* all of the requested data to arrive. Since 50 bytes
	* is relatively small compared to the allowed TCP
	* packet sizes, a partial receive is unlikely. If
	* this example had used 2048 bytes requests instead,
	* a partial receive would be far more likely.
	* This loop will keep receiving until all 50 bytes
	* have been received, thus guaranteeing that the
	* next recv at the top of the loop will
	* start at the begining of the next reply.
	*/
	time(&timevar);
	printf("All done at %s\n", ctime(&timevar));
	ConnectionClosedByServer:
	printf("Closing the socket...\n");
	close(s);
	printf("\t...Done!\n");


	/* Last thing that main() should do */
	pthread_exit(NULL);
	return 0;
}






int handleSON(struct nl80211_state *sktctr, int *argc, char **argstart, struct CommandContainer *cmd) {
	int err;
	char bypassquery=0;
	enum adapttxpowermodes adapttxpowermode;
	static long txpowermax;
	enum nl80211_iftype readiftype;
	char input;
	if (*argc < 2) {
		printf("To few arguments for:\n");
		//misuse the err as loop-counter.
		for(err=0;err<*argc;err++)
			printf(" %s", *(argstart+err));
		printf("\n");
		return MAIN_ERR_FEW_CMDS;
	}
	argstart++;
	if (strcmp(*(argstart), "adapttxpower") == 0) {
		printf("\nStarting to adapt the transmission power...\n");
		printf("\nSetting up Mode to operate...\n");
		printf("Requesting current infrastructure type from Kernel...\n");
		//Get infrastructure Mode from Kernel
		cmd->identifier = BYPASS_HANDLELINK_NL80211_CMD_GET_SCAN | BYPASS_HANDLELINK_NL80211_CMD_GET_STATION | BYPASS_HANDLELINK_NL80211_CMD_GET_SURVEY;
		err = handleLink(sktctr, argc, argstart, cmd);
		//determine which Transmission Power Adaption operation mode would match best to current iftype
	    switch(WhatWeWant.type) {
	    case NL80211_IFTYPE_UNSPECIFIED:
	    	break;
	    case NL80211_IFTYPE_ADHOC:
			adapttxpowermode = adhoc;
	    	break;
	    case NL80211_IFTYPE_STATION:
			adapttxpowermode = client;
	    	break;
	    case NL80211_IFTYPE_AP:
			adapttxpowermode = server;
	    	break;
	    case NL80211_IFTYPE_AP_VLAN:
	    	break;
	    case NL80211_IFTYPE_WDS:
	    	break;
	    case NL80211_IFTYPE_MONITOR:
	    	break;
	    case NL80211_IFTYPE_MESH_POINT:
	    	break;
	    case NL80211_IFTYPE_P2P_CLIENT:
	    	break;
	    case NL80211_IFTYPE_P2P_GO:
	    	break;
	    case NL80211_IFTYPE_P2P_DEVICE:
	    	break;
	    }
		printf("\tReading in the config-file...\n");
		char cfg_path_prefix[] = CFG_FILE_PATH_PREFIX_ADAPTTXPOWER;
		int cfg_path_leng = sizeof(cfg_path_prefix);
		char cfg_file[cfg_path_leng+9];//Designed with interfacenames of size 5 Bytes
						//Take account of this here and down there at the second memcpy
						//For sure you could do it with sizeof(), but how likely will this
						//differ from 5? So the immediate Value works a bit faster...
		//Remember the End-of-String Sign ('\0')...
		memcpy(cfg_file,cfg_path_prefix,cfg_path_leng);
//		cfg_file[cfg_path_leng-1]='w';
//		cfg_file[cfg_path_leng]='l';
//		cfg_file[cfg_path_leng+1]='a';
//		cfg_file[cfg_path_leng+2]='n';
//		cfg_file[cfg_path_leng+3]='0';
		memcpy((cfg_file+cfg_path_leng-1),(WhatWeWant.interfacename),5);
		cfg_file[cfg_path_leng+4]='.';
		cfg_file[cfg_path_leng+5]='c';
		cfg_file[cfg_path_leng+6]='f';
		cfg_file[cfg_path_leng+7]='g';
		cfg_file[cfg_path_leng+8]='\0';
	    FILE *cfgf;
	    cfgf = fopen(cfg_file, "r");
	    if (!cfgf) {
	    	switch(errno) {
	    		case EACCES:
	    			printf("\tERROR: Couldn't open cfg-File!\n\t\tReason:Permission denied!\n");
	    			printf("\tSetting up Mode while using the infrastructure mode read from Kernel...\n");
	    		break;
	    		case ENOENT:
	    			printf("\tERROR: Cfg-File doesn't exist!\n");
	    			printf("\tCreating new one:\n\t\tTherefore using infrastructure mode got from Kernel,\n\t\tputting in new cfg-File and\n\t\tsetting up current Mode with this...\n");
	    			printf("\t\t...Found working infrastructure Mode: %d\n", WhatWeWant.type);
	    		    cfgf = fopen(cfg_file, "w+");
	    		    if(!cfgf){
	    		    	printf("\t\tERROR: Couldn't create new cfg-file!\n");
	    		    	return FILE_ERR_PERMISSION_DENIED;
	    		    }
    		    	printf("\t\tSetting up config-file with:\n");
	    		    switch(WhatWeWant.type) {
	    		    case NL80211_IFTYPE_UNSPECIFIED:
	    		    	printf("\t\t\t#iftype=unspecified\n");
		    			fprintf(cfgf, "\n#iftype=unspecified");
		    			fprintf(cfgf, "\n");
		    			goto AdaptTXPowerSetupClient;
	    		    	break;
	    		    case NL80211_IFTYPE_ADHOC:
	    		    	printf("\t\t\t#iftype=adhoc\n");
		    			fprintf(cfgf, "\n#iftype=adhoc");
		    			AdaptTXPowerSetupAdhoc:
	    		    	printf("\t\t\t#adapttxpowermode=adhoc\n");
		    			fprintf(cfgf, "\n#adapttxpowermode=adhoc");
		    			fprintf(cfgf, "\n#bypassquery=false");
		    			fprintf(cfgf, "\n#DeviceMaxTXPower=20");
		    			fprintf(cfgf, "\n*INFO: Value in dBm");
		    			fprintf(cfgf, "\n*NOTE: 0.1 W = 20 dBm (2,4 GHz)");
		    			fprintf(cfgf, "\n*NOTE: 0.2 W = 23.01 dBm (5 GHz)");
		    			fprintf(cfgf, "\n*NOTE: 1 W = 30 dBm (5,5 GHz)");
		    			fprintf(cfgf, "\n#logadaption=false");
		    			fprintf(cfgf, "\n#adapttargetrss=%d", MATDIFFC);
		    			fprintf(cfgf, "\n#adapttargetrssupperdiff=%d", MATDIFFO);
		    			fprintf(cfgf, "\n#adapttargetrsslowerdiff=%d", MATDIFFU);
		    			fprintf(cfgf, "\n#adapttargetsnr=%d", SNRC);
		    			fprintf(cfgf, "\n#adapttargetsnrupperdiff=%d", SNRDIFFO);
		    			fprintf(cfgf, "\n#adapttargetsnrlowerdiff=%d", SNRDIFFU);
		    			fprintf(cfgf, "\n");
		    			adapttxpowermode = adhoc;
		    			txpowermax = 20;
	    		    	break;
	    		    case NL80211_IFTYPE_STATION:
	    		    	printf("\t\t\t#iftype=station\n");
		    			fprintf(cfgf, "\n#iftype=station");
		    			AdaptTXPowerSetupClient:
	    		    	printf("\t\t\t#adapttxpowermode=client\n");
		    			fprintf(cfgf, "\n#adapttxpowermode=client");
		    			fprintf(cfgf, "\n#bypassquery=false");
		    			fprintf(cfgf, "\n#DeviceMaxTXPower=20");
		    			fprintf(cfgf, "\n*INFO: Value in dBm");
		    			fprintf(cfgf, "\n*NOTE: 0.1 W = 20 dBm (2,4 GHz)");
		    			fprintf(cfgf, "\n*NOTE: 0.2 W = 23.01 dBm (5 GHz)");
		    			fprintf(cfgf, "\n*NOTE: 1 W = 30 dBm (5,5 GHz)");
		    			fprintf(cfgf, "\n#logadaption=false");
		    			fprintf(cfgf, "\n#adapttargetrss=%d", MATDIFFC);
		    			fprintf(cfgf, "\n#adapttargetrssupperdiff=%d", MATDIFFO);
		    			fprintf(cfgf, "\n#adapttargetrsslowerdiff=%d", MATDIFFU);
		    			fprintf(cfgf, "\n#adapttargetsnr=%d", SNRC);
		    			fprintf(cfgf, "\n#adapttargetsnrupperdiff=%d", SNRDIFFO);
		    			fprintf(cfgf, "\n#adapttargetsnrlowerdiff=%d", SNRDIFFU);
		    			fprintf(cfgf, "\n");
		    			adapttxpowermode = client;
		    			txpowermax = 20;
	    		    	break;
	    		    case NL80211_IFTYPE_AP:
	    		    	printf("\t\t\t#iftype=accesspoint\n");
		    			fprintf(cfgf, "\n#iftype=accesspoint");
		    			AdaptTXPowerSetupServer:
	    		    	printf("\t\t\t#adapttxpowermode=server\n");
		    			fprintf(cfgf, "\n#adapttxpowermode=server");
		    			fprintf(cfgf, "\n#bypassquery=false");
		    			fprintf(cfgf, "\n#DeviceMaxTXPower=20");
		    			fprintf(cfgf, "\n*INFO: Value in dBm");
		    			fprintf(cfgf, "\n*NOTE: 0.1 W = 20 dBm (2,4 GHz)");
		    			fprintf(cfgf, "\n*NOTE: 0.2 W = 23.01 dBm (5 GHz)");
		    			fprintf(cfgf, "\n*NOTE: 1 W = 30 dBm (5,5 GHz)");
		    			fprintf(cfgf, "\n#logadaption=false");
		    			fprintf(cfgf, "\n#adapttargetrss=%d", MATDIFFC);
		    			fprintf(cfgf, "\n#adapttargetrssupperdiff=%d", MATDIFFO);
		    			fprintf(cfgf, "\n#adapttargetrsslowerdiff=%d", MATDIFFU);
		    			fprintf(cfgf, "\n#adapttargetsnr=%d", SNRC);
		    			fprintf(cfgf, "\n#adapttargetsnrupperdiff=%d", SNRDIFFO);
		    			fprintf(cfgf, "\n#adapttargetsnrlowerdiff=%d", SNRDIFFU);
		    			fprintf(cfgf, "\n");
		    			adapttxpowermode = server;
		    			txpowermax = 20;
	    		    	break;
	    		    case NL80211_IFTYPE_AP_VLAN:
	    		    	printf("\t\t\t#iftype=apvlan\n");
		    			fprintf(cfgf, "\n#iftype=apvlan");
		    			fprintf(cfgf, "\n");
		    			goto AdaptTXPowerSetupServer;
	    		    	break;
	    		    case NL80211_IFTYPE_WDS:
	    		    	printf("\t\t\t#iftype=wds\n");
		    			fprintf(cfgf, "\n#iftype=wds");
		    			fprintf(cfgf, "\n");
		    			goto AdaptTXPowerSetupClient;
	    		    	break;
	    		    case NL80211_IFTYPE_MONITOR:
	    		    	printf("\t\t\t#iftype=monitor\n");
		    			fprintf(cfgf, "\n#iftype=monitor");
		    			fprintf(cfgf, "\n");
		    			goto AdaptTXPowerSetupClient;
	    		    	break;
	    		    case NL80211_IFTYPE_MESH_POINT:
	    		    	printf("\t\t\t#iftype=mesh\n");
		    			fprintf(cfgf, "\n#iftype=mesh");
		    			fprintf(cfgf, "\n");
		    			goto AdaptTXPowerSetupClient;
	    		    	break;
	    		    case NL80211_IFTYPE_P2P_CLIENT:
	    		    	printf("\t\t\t#iftype=p2pclient\n");
		    			fprintf(cfgf, "\n#iftype=p2pclient");
		    			fprintf(cfgf, "\n");
		    			goto AdaptTXPowerSetupClient;
	    		    	break;
	    		    case NL80211_IFTYPE_P2P_GO:
	    		    	printf("\t\t\t#iftype=p2pgo\n");
		    			fprintf(cfgf, "\n#iftype=p2pgo");
		    			fprintf(cfgf, "\n");
		    			goto AdaptTXPowerSetupClient;
	    		    	break;
	    		    case NL80211_IFTYPE_P2P_DEVICE:
	    		    	printf("\t\t\t#iftype=p2pdevice\n");
		    			fprintf(cfgf, "\n#iftype=p2pdevice");
		    			fprintf(cfgf, "\n");
		    			goto AdaptTXPowerSetupClient;
	    		    	break;
	    		    }
	    	    	fclose(cfgf);
	    			printf("\t\t...created cfg-file successfully!\n");
	    			break;
	    		    default:
	    		    	fprintf(stderr, "Ups, seems like we've encountered a case, which isn't caught yet :o(");
	    		    	return MAIN_ERR_FUNC_INCOMPLETE;
	    		    	break;
	    	}
	    } else {
	    	enum adapttxpowermodes readadapttxpowermode;
	    	char buf[100];
	    	memset(buf,0,sizeof(buf));
	    	while(1) {
	    		int loopcnt;
	    		int readval;
				if ((readval = getc(cfgf)) == EOF) {
				 printf("\t...Read completely through config-file.\n");//End of file reached
				 break;
				}
				buf[0]=(char)readval;
				if (buf[0] == '#') {
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
						if (((buf[loopcnt] = getc(cfgf)) == '=') || (buf[loopcnt] == EOF)) {
							buf[loopcnt]='\0';
							break;
						}
					}
				}
				if(strcmp(buf,"adapttxpowermode")==0){
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) {
							buf[loopcnt]='\0';
							break;
						}
					}
					if(strcmp(buf,"client")==0){
						readadapttxpowermode=client;
					} else
					if(strcmp(buf,"server")==0){
						readadapttxpowermode=server;
					} else
					if(strcmp(buf,"adhoc")==0){
						readadapttxpowermode=adhoc;
					} else {
						printf("\tERROR: cfg-file damaged! Invalid Entry at adapttxpowermode!\n");
					}
				} else
				if(strcmp(buf,"iftype")==0){
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) {
							buf[loopcnt]='\0';
							break;
						}
					}
					if(strcmp(buf,"station")==0){
						readiftype=NL80211_IFTYPE_STATION;
					} else
					if(strcmp(buf,"accesspoint")==0){
						readiftype=NL80211_IFTYPE_AP;
					} else
					if(strcmp(buf,"adhoc")==0){
						readiftype=NL80211_IFTYPE_ADHOC;
					} else
					if(strcmp(buf,"monitor")==0){
						readiftype=NL80211_IFTYPE_MONITOR;
					} else
					if(strcmp(buf,"apvlan")==0){
						readiftype=NL80211_IFTYPE_AP_VLAN;
					} else
					if(strcmp(buf,"wds")==0){
						readiftype=NL80211_IFTYPE_WDS;
					} else
					if(strcmp(buf,"mesh")==0){
						readiftype=NL80211_IFTYPE_MESH_POINT;
					} else
					if(strcmp(buf,"pspclient")==0){
						readiftype=NL80211_IFTYPE_P2P_CLIENT;
					} else
					if(strcmp(buf,"p2pgo")==0){
						readiftype=NL80211_IFTYPE_P2P_GO;
					} else
					if(strcmp(buf,"p2pdevice")==0){
						readiftype=NL80211_IFTYPE_P2P_DEVICE;
					} else
					if(strcmp(buf,"unspecified")==0){
						readiftype=NL80211_IFTYPE_UNSPECIFIED;
					} else {
						printf("\tERROR: cfg-file damaged! Invalid Entry at iftype!\n");
					}
				} else
				if(strcmp(buf,"bypassquery")==0){
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) {
							buf[loopcnt]='\0';
							break;
						}
					}
					if(strcmp(buf,"false")==0){

					} else
					if(strcmp(buf,"true")==0){
						bypassquery=1;
					}
				} else
				if(strcmp(buf,"DeviceMaxTXPower")==0){
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) {
							buf[loopcnt]='\0';
							break;
						}
					}
					char *endpoint;
					txpowermax = strtol(buf, &endpoint, 10);
					if (*endpoint) {
						printf("ERROR: Unsupported Format in cfg-File at:\n\tDeviceMaxTXPower\n");
						return MAIN_ERR_BAD_CMDLINE;
					}
					//Take account of that the Maximum TXPower of the Device here is gotten as a "long"
					//and is given to the client-routine as that.
					//So consider the Bitwidth of the executing Processor (and how the compiler handles "long")
					//to get sure, that the comparison with the actual TXPower to set is done smooth
					//and that this Value is passed right to nl80211 message, sent to the Kernel.
				} else
				if(strcmp(buf,"logadaption")==0){
					for(loopcnt=0;loopcnt<sizeof(buf);loopcnt++){
						if (((buf[loopcnt] = getc(cfgf)) == '\n') || (buf[loopcnt] == EOF)) {
							buf[loopcnt]='\0';
							break;
						}
					}
					if(strcmp(buf,"false")==0){

					} else
					if(strcmp(buf,"true")==0){
						logatxp=logatxp | LOG_ATXP_BASIC;
					}
				}
	    	}
	    	printf("\t...Read in Operation Mode to: %s\n", (readadapttxpowermode==client)?"client":((readadapttxpowermode==server)?"server":((readadapttxpowermode==adhoc)?"adhoc":"Ehm, something gone wrong...")));
	    	printf("\t...Read in IFTYPE: %d\n",readiftype);
	        if (fclose(cfgf)) {
	        	printf("NOTICE: Config-File couldn't be closed successfully!");
	        } else {
	        	printf("...config-file read successfully!\n");
	        }
	        printf("\nChecking consistency...\n");
	        if(readiftype!=WhatWeWant.type){
	        	//NOTE: Later iftype checks will work with the variable readiftype
	        	printf("WARNING: Infrastructure Mode, specified in config-file,\ndiffers from current working Interface Infrastructure type!\n");
	        	printf("\tCfg-File: %d\n",readiftype);
	        	printf("\tCurrent from Kernel: %d\n",WhatWeWant.type);
	        	if(bypassquery) {
	        		puts("NOTE: But cgf-File forced me to bypass further queries.");
	        		goto BypassIftypeQuery;
	        	}
	        	printf("Do you want me to force the config to change to current iftype?\n");
	        	printf("(y/n): ");
	        	char done = 0;
				while(!done){
					input = getchar();
					if(input=='\n')
						goto ConsTypeSkipNewLine;
					switch(input){
					case 'y':
			        	err=MAIN_ERR_CLEARED;
			        	readiftype=WhatWeWant.type;
						//TODO Do all the cfg-changing stuff... Write WhatWeWant.type in the cfg
						printf("\t(Not implemented yet...)\n");

			        	/* Example
			        	 *
			        	 */

//						#include <stdio.h>
//						#include <stdlib.h>
//						#include <sys/types.h>
//						#include <sys/stat.h>
//						#include <unistd.h>
//						#include <fcntl.h>
//						#include <sys/mman.h>
//
//						#define FILEPATH "/tmp/mmapped.bin"
//						#define NUMINTS  (1000)
//						#define FILESIZE (NUMINTS * sizeof(int)
//
//			            int i;
//			            int fd;
//			            int result;
//			            int *map;  /* mmapped array of int's */
//
//			            /* Open a file for writing.
//			             *  - Creating the file if it doesn't exist.
//			             *  - Truncating it to 0 size if it already exists. (not really needed)
//			             *
//			             * Note: "O_WRONLY" mode is not sufficient when mmaping.
//			             */
//			            fd = open(FILEPATH, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
//			            if (fd == -1) {
//			        	perror("Error opening file for writing");
//			        	exit(EXIT_FAILURE);
//			            }
//
//			            /* Stretch the file size to the size of the (mmapped) array of ints
//			             */
//			            result = lseek(fd, FILESIZE-1, SEEK_SET);
//			            if (result == -1) {
//			        	close(fd);
//			        	perror("Error calling lseek() to 'stretch' the file");
//			        	exit(EXIT_FAILURE);
//			            }
//
//			            /* Something needs to be written at the end of the file to
//			             * have the file actually have the new size.
//			             * Just writing an empty string at the current file position will do.
//			             *
//			             * Note:
//			             *  - The current position in the file is at the end of the stretched
//			             *    file due to the call to lseek().
//			             *  - An empty string is actually a single '\0' character, so a zero-byte
//			             *    will be written at the last byte of the file.
//			             */
//			            result = write(fd, "", 1);
//			            if (result != 1) {
//			        	close(fd);
//			        	perror("Error writing last byte of the file");
//			        	exit(EXIT_FAILURE);
//			            }
//
//			            /* Now the file is ready to be mmapped.
//			             */
//			            map = mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
//			            if (map == MAP_FAILED) {
//			        	close(fd);
//			        	perror("Error mmapping the file");
//			        	exit(EXIT_FAILURE);
//			            }
//
//			            /* Now write int's to the file as if it were memory (an array of ints).
//			             */
//			            for (i = 1; i <=NUMINTS; ++i) {
//			        	map[i] = 2 * i;
//			            }
//
//			            /* Don't forget to free the mmapped memory
//			             */
//			            if (munmap(map, FILESIZE) == -1) {
//			        	perror("Error un-mmapping the file");
//			        	/* Decide here whether to close(fd) and exit() or not. Depends... */
//			            }
//
//			            /* Un-mmaping doesn't close the file, so we still need to do that.
//			             */
//			            close(fd);

			            /*
			             * Example End
			             */
						done=1;
						break;
					case 'n':
						//simply do nothing. Jump out.
			        	err=MAIN_ERR_WARNING;
						goto ConsTypeNoCfgTypeChange;
						break;
					default:
						printf("PROBLEM: Invalid Input! You entered: %c!\n",input);
						printf("Another try (y/n): ");
						break;
					}
					ConsTypeSkipNewLine:
					;
				}
				ConsTypeNoCfgTypeChange:
				//Clear stdin buffer. Especially to remove this disturbing NewLine after last Input
				//fflush(stdin) or fseek(stdin,0,SEEK_END) or flushinp() doesn't really work that often -.-
				while(input!='\n' && input!=EOF)
					input=getchar();
	        	printf("\n");
	        }
	        if(err==MAIN_ERR_WARNING)
	        	printf("NOTICE-cfg-iftype: Kept differing settings in cfg-file.\nWorking now with settings from cfg-file...\n");
	        if(err==MAIN_ERR_CLEARED)
	        	printf("NOTICE-cfg-iftype: Config-File changed to current working status.\nWorking now with new settings...\n");
	        BypassIftypeQuery:
	        if(readadapttxpowermode!=adapttxpowermode){
	        	printf("\nWARNING: Operating Mode to adapt the Transsmission power,\nspecified in config-file, ndiffers from where the current\nworking Interface Infrastructure type would lead us to!\n");
	        	printf("\tCfg-File: %s\n",(readadapttxpowermode==client)?"client":((readadapttxpowermode==server)?"server":((readadapttxpowermode==adhoc)?"adhoc":"Ehm, something gone wrong...")));
	        	printf("\tCurrent from Kernel: %s\n",(adapttxpowermode==client)?"client":((adapttxpowermode==server)?"server":((adapttxpowermode==adhoc)?"adhoc":"Ehm, something gone wrong...")));
	        	if(bypassquery) {
	        		puts("NOTE: But cgf-File forced me to bypass further queries.");
	        		puts("\tHence now working with settings from cfg-File anyway.");
		        	adapttxpowermode=readadapttxpowermode;//Only if user doesn't force ollerus to change cfg... or bypass is active
	        		goto BypassOpModeQuery;
	        	}
	        	printf("Do you want me to force the config to change to current TX Power Adaption Mode?\n");
	        	printf("(y/n): ");
	        	char done = 0;
				while(!done){
					input = getchar();
					if(input=='\n')
						goto ConsTXPowModeSkipNewLine;
					switch(input){
					case 'y':
			        	err=MAIN_ERR_CLEARED;
						//TODO Do all the cfg-changing stuff... Write adapttxpowermode inside cfg-file
						printf("\t(Not implemented yet...)\n");
						done=1;
						break;
					case 'n':
			        	adapttxpowermode=readadapttxpowermode;//Only if user doesn't force ollerus to change cfg... or bypass is active
						//simply do nothing. Jump out.
			        	err=MAIN_ERR_WARNING;
						goto ConsTXPowModeNoCfgTypeChange;
						break;
					default:
						printf("PROBLEM: Invalid Input! You entered: %c!\n",input);
						printf("Another try (y/n): ");
						break;
					}
					ConsTXPowModeSkipNewLine:
					;
				}
				ConsTXPowModeNoCfgTypeChange:
				//Clear stdin buffer. Especially to remove this disturbing NewLine after last Input
				while(input!='\n' && input!=EOF)
					input=getchar();
	        	printf("\n");
	        }
	        if(err==MAIN_ERR_WARNING)
	        	printf("NOTICE-cfg-AdaptTXPowerMode: Kept differing settings in cfg-file.\nWorking now with settings from cfg-file...\n");
	        if(err==MAIN_ERR_CLEARED)
	        	printf("NOTICE-cfg-AdaptTXPowerMode: Config-File changed to current working status.\nWorking now with new settings...\n");
	    }
	    BypassOpModeQuery:
    	printf("\nMode-Setup finished.\n");
    	if(bypassquery) {
    		goto NoShortPause;
    	}
    	printf("Short Pause before everything starts...\nEnter any value to continue.\n");
    	input = getchar();
		//Clear stdin buffer. Especially to remove this disturbing NewLine after last Input
		while(input!='\n' && input!=EOF)
			input=getchar();
//    	printf("\n");
		NoShortPause:
    	switch (adapttxpowermode) {
    	case adhoc:
    		fprintf(stderr, "»Clean« Adhoc Mode not implemented yet.\nUse the »adapttxpowermode«-switched Client-Server-Mode\n\t- choosen with iftype=adhoc");
    		return MAIN_ERR_FUNC_INCOMPLETE;
    		break;
    	case client:
        	keep_txpower_min_client(sktctr, cmd, (*argc)++, argstart--, readiftype, txpowermax, cfg_file);
    		break;
    	case server:
    		keep_txpower_min_server(sktctr, cmd, (*argc)++, argstart--);
    		break;
    	default:
    		break;
    	}

		return 0;
	} else if (strcmp(*(argstart), "optimizechannel") == 0) {



//			kill_AP(0);
//			kill_AP(1);




		setupChannelLoadChainStart();
		cmd->cmd = NL80211_CMD_GET_SURVEY;
		prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
		cmd->nl_msg_flags = NLM_F_DUMP;
		cmd->callbackToUse = Do_CMD_GET_SURVEY_cb_communication_load;
		err = send_with_cmdContainer(sktctr, 0, 0, cmd);

		struct bestchan bc;
		struct bestchan setchan;
		setchan.AP_index=0;

		struct interfaces interface_names;
		interface_names.wlan0 = if_nametoindex("wlan0");
		interface_names.wlan1 = if_nametoindex("wlan1");
		interface_names.wlan2 = if_nametoindex("wlan2");


//		evtl. die size of mit der sizeof(channel_property )multiplizieren
		struct channel_property property24GHz [ARRAY_24GHZ_SIZE];
		struct channel_property property5GHz [ARRAY_5GHZ_SIZE];
		int count;
		for(count=0;count<ARRAY_24GHZ_SIZE;count++){
			property24GHz[count].channel=0;
			property24GHz[count].noise=0;
			property24GHz[count].load=0;
			property24GHz[count].count=0;
		}
		for(count=0;count<ARRAY_5GHZ_SIZE;count++){
			property5GHz[count].channel=0;
			property5GHz[count].noise=0;
			property5GHz[count].load=0;
			property5GHz[count].count=0;
		}

		channelloadchain_console_print (channelloadstart, property24GHz, property5GHz, &setchan, 0);

		free_channelload_chain(channelloadstart);

		lowload(property24GHz, property5GHz, &bc);


		setchan.band24=bc.band24;
		setchan.band5=bc.band5;
		setchan.active_interface=interface_names.wlan0;


//		TODO: Hier fehlt das Startup des ersten Acess Point anhand der Daten die im ersten SNIR Scan erfasst wurden


		pthread_t noiseThread;
		pthread_t hostapdthread[2];

 char test=0;
		printdat(&setchan,&hostapdthread[0]);

		struct timespec remainingdelay;
					remainingdelay.tv_sec =SCAN_INTERVAL_TIME;
					remainingdelay.tv_nsec = 0;
					do {
						err = nanosleep(&remainingdelay, &remainingdelay);
					} while (err<0);



//		pthread_cancel(hostapdthread[1]);



		struct noiseThreadArgPassing *pthreadArgPass;
				pthreadArgPass = malloc(sizeof(struct noiseThreadArgPassing));
				pthreadArgPass->ownindex=&noiseThread;
				pthreadArgPass->cmd=cmd;
				pthreadArgPass->skt=sktctr;
//				pthreadArgPass->ret1= property24GHz;
//				pthreadArgPass->ret2= property5GHz;
				pthreadArgPass->int_names=&interface_names;
				pthreadArgPass->setchan=&setchan;
				pthreadArgPass->bestchan=&bc;
				pthreadArgPass->hostapdthread=hostapdthread;
				pthread_attr_t tattr;
				/* initialized with default attributes */
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
				//secure detached-state for the threads over attributed creation
				if( (err=pthread_create(&noiseThread, &tattr, loadscan, (void*)pthreadArgPass)) < 0) {
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





//		pthread_t hostapdthread [2];
//
//
//
//
//		//	hier aufpassen vielleicht ohne array lösen
//				pthread_t hostapdthread [2];
//
//				struct loadold24 no24;
//				struct loadold5  no5;
//				struct loadold24 no24old;
//				struct loadold5 no5old;
//
//
//
//				int temp[30];
//				char argv[3];
//				char no1="set";
//				char no2="channel";
//				char no3="";
//				argv[0]=&no1;
//				argv[1]=&no2;
//				argv[2]=&no3;
//			//	int i = 0;
//				int tempcount = 0;
//				int chan = 1;
//				int chancount = 1;
//				int err;
//
//				while(tempcount<=1){
//						sprintf(no3,"%u",chan);
//				//here the channel is going to be set with the handleset function.
//						handleSet(sktctr, 3, argv,cmd);
//				//here the noise is received and written to the temp array;
//						cmd->cmd = NL80211_CMD_GET_SURVEY;
//						prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
//					//	prepareAttribute(&cmd, NL80211_ATTR_MAC, WhatWeWant.MAC_ConnectedTo);
//						cmd->nl_msg_flags = NLM_F_DUMP;
//						cmd->callbackToUse = Do_CMD_GET_NOISE_cb;
//						err = send_with_cmdContainer(sktctr, 0, NULL, cmd);
//
//				        temp[chancount]= WhatWeWant.noise;
//				        chancount++;
//						if(chan==64){
//							int i;
//							for (i=0 ; i<=12 ; i++){
//								no24.chan[i] += temp[i];
//							}
//							no24.count++;
//							for (i=13 ; i<=20 ; i++){
//								no5.chan[i-13] += temp[i];
//							}
//							no5.count++;
//							int chan = 1;
//							tempcount++;
//							chancount = 1;
//						}
//						else if (chan>=36)
//							chan+=4;
//						else if(chan==13)
//							chan=36;
//						else
//							chan++;
//				//		i++;
//						lowno(&no24,&no5,&bc);
//
//					}
//
//				printdat(bc.band24,1,hostapdthread);
//				printdat(bc.band5,0,hostapdthread);
//
//

//
//				printf("\nThe wireless channel wasn't ever choosen this smooth!\n");
//				//falls dies nich funktioniert obigen Thread anstatt Detached einen Joined Thread
//				pthread_exit(NULL);
//				return 0;
			}
//	kill_AP(0);
//	kill_AP(1);
	pthread_exit(NULL);

	return err;
}
