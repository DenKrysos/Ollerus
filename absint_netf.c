/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#define NO_ABSINT_NETF_C_FUNCTIONS

#include <sys/socket.h>
#include </usr/include/linux/wireless.h>
#include </usr/include/linux/if_arp.h>
#include <ifaddrs.h>
#include <netinet/in.h>
//#include <linux/if.h>
//#include <net/if.h>
//-------END ioctl-------//
#include "libnetlink.h"

#include "ollerus_globalsettings.h"
#include "ollerus.h"
#include "absint.h"
#include "absint_netf.h"
#include "ollerus.h"

#include "head/ollerus_extern_functions.h"

/* Network Functions from the Abstract Interface
 * Things like Network Discovery, Platform-Port-Readout...
 */



char check_running_wpa_supplicant(char *dev){
	/* Returns 1, if there is a running Daemon on the passed Interface
	 * Returns 0, if there is no running Daemon on the passed Interface
	 */
	int err;err=0;
	//2>&1: Redirect stderr to stdout stream
	char cmd[strlen("wpa_cli -i  list_networks 2>&1")+
				 strlen(dev)+1];
	snprintf(cmd,sizeof(cmd),"wpa_cli -i %s list_networks 2>&1",dev);
	FILE *fp;
	char buf[1024];
	char *rd;
	int i;i=0;

	fp = popen(cmd, "r");
	if (fp == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}
//	while (fgets(buf, sizeof(buf)-1, fp) != NULL) {
//	}
//	sleep(1);
	while ((buf[i]=fgetc(fp)) != EOF) {
		i++;
	}
	buf[i]='\0';
	pclose(fp);
	if(strstr(buf,"CURRENT")==NULL){
	#ifdef DEVELOPMENT_MODE
		printfc(yellow,"  ---> NOTE: ");printf("wpa_supplicant ");printfc(red,"NOT");printf(" running! | Answer-length %d\n",strlen(buf));
	#endif
		return 0;
	}else {
	#ifdef DEVELOPMENT_MODE
		printfc(yellow,"  ---> NOTE: ");printf("wpa_supplicant ");printfc(red,"currently");printf(" running! | Answer-length %d\n",strlen(buf));
	#endif
		return 1;
	}
}
//#define NO_WLAN_CONNECTION //Debuging Purpose
#ifdef NO_WLAN_CONNECTION
int start_adhoc_wpa_supplicant(char *dev,char running){return 0;};
void stop_adhoc_wpa_supplicant(char *dev,char running){};
#else
int start_adhoc_wpa_supplicant(char *dev,char running){
	int err;err=0;

	printf("\nStarting Ad-hoc Network on %s...\n",dev);
	CREATE_PROGRAM_PATH(*args);
	WPA_SUPPLICANT_CONF_SETUP(dev);
	printf("   ...with %s\n",wpasup_file);

	if(running){
		//Kill already running Daemon, if any exitsts | No Console Output: >/dev/null 2>&1
		char killcmd[strlen("wpa_cli -i  terminate >/dev/null 2>&1")+
					 strlen(dev)+1];
		snprintf(killcmd,sizeof(killcmd),"wpa_cli -i %s terminate >/dev/null 2>&1",dev);
		system(killcmd);
	}

	//Detection of Bad Termination of a previous wpa_supplicant Run
	printf("\tFirst check for existing run-file (would be still present,\n\t  if a previous run of wpa_supplicant wasn't properly terminated.)\n");
	char runfilepath[strlen("/run/wpa_supplicant/")+strlen(dev)+1];
	snprintf(runfilepath,sizeof(runfilepath),"/run/wpa_supplicant/%s",dev);
									/*
									FILE *runfile;
									runfile = fopen(runfilepath, "r+");
									if (!runfile) {
										switch(errno) {
											case ENOENT:
												goto NoFile;
												break;
											default:
												break;
										}
										goto NoFileClosure;
									}
									fclose(runfile);
									NoFileClosure:
									//Here we came, when there is a file, that we have to try to delete it.
									printfc(yellow,"   Warning: ");printf("There is a wpa_supplicant-run-file.\n\tTrying to delete %s\n",runfilepath);
									if(remove(runfilepath)==0){
										printfc(yellow,"   Successfully");printf(" deleted leftover run-file!\n");
									}else{
										printfc(red,"ERROR: ");printf("Couldn't delete File %s\n\t\tMaybe blocked by a running process?\n",runfilepath);
										return FILE_ERR_PERMISSION_DENIED;
									}
									//Jump to this Marker, when there is no such File
									NoFile:
									;
									*/
	//Do the Existence Check in another way... Nicer than this first approach
	if(access(runfilepath,F_OK)!=-1){
		//Here we came, when there is a file, that we have to try to delete it.
		printfc(yellow,"   Warning: ");printf("There is a wpa_supplicant-run-file.\n\tTrying to delete %s\n",runfilepath);
		if(remove(runfilepath)==0){
			printfc(yellow,"   Successfully");printf(" deleted leftover run-file!\n");
		}else{
			printfc(red,"ERROR: ");printf("Couldn't delete File %s\n\t\tMaybe blocked by a running process?\n",runfilepath);
			return FILE_ERR_PERMISSION_DENIED;
		}
	}else {
	    // file doesn't exist
	}

	char wpastartcmd[strlen("wpa_supplicant -B -i  -c  -D nl80211,wext")+
					 strlen(dev)+
					 strlen(wpasup_file)+1];
	snprintf(wpastartcmd,
			sizeof(wpastartcmd),
			"wpa_supplicant -B -i %s -c %s -D nl80211,wext",
			dev,wpasup_file);
	printf("  ...Calling: %s\n",wpastartcmd);
	system(wpastartcmd);

	puts("");
	return err;
}
void stop_adhoc_wpa_supplicant(char *dev,char running){
	//Kill running Daemon, if any exitsts | No Console Output: >/dev/null 2>&1
	if(running){
		char killcmd[strlen("wpa_cli -i  terminate >/dev/null 2>&1")+
					 strlen(dev)+1];
		snprintf(killcmd,sizeof(killcmd),"wpa_cli -i %s terminate >/dev/null 2>&1",dev);
		system(killcmd);
	}
	//Detection of Bad Termination of a previous wpa_supplicant Run
	char runfilepath[strlen("/run/wpa_supplicant/")+strlen(dev)+1];
	snprintf(runfilepath,sizeof(runfilepath),"/run/wpa_supplicant/%s",dev);
	if(access(runfilepath,F_OK)!=-1){
		//Here we came, when there is a file, that we have to try to delete it.
		if(remove(runfilepath)==0){
		}else{
		}
	}else {
	    // file doesn't exist
	}
}
#endif



//The Setting Functions either can just change the cfg file or also alter a running daemon
//Determined by the given running flag.
int set_adhoc_essid_wpa_supplicant(char *dev,char *ssid,char running){
	if(strlen(ssid)>32){
		printfc(red,"ERROR: ");printf("Delivered SSID for Change is longer than 32 characters!\n");
		return MAIN_ERR_BAD_CMDLINE;
	}
	int err;err=0;

	printf("\nSetting Ad-hoc ESSID on %s to \"%s\"...\n",dev,ssid);
	CREATE_PROGRAM_PATH(*args);
	WPA_SUPPLICANT_CONF_SETUP(dev);
	printf("   ...Changing File %s\n",wpasup_file);

							//	char wpastartcmd[strlen("wpa_cli -i  set")+
							//					 strlen(dev)+
							//					 strlen(ssid)+1];
							//	snprintf(wpastartcmd,
							//			sizeof(wpastartcmd),
							//			"wpa_supplicant -B -i %s -c %s -D nl80211,wext",
							//			dev,wpasup_file);
							//	printf("  ...Calling: %s\n",wpastartcmd);
							//	system(wpastartcmd);

    struct stat sbuf;
	int conffd;//Config-File File-Descriptor
	int pagesize;
	int maxc;//The maximum index for data-access of the mmap-file
	char *data;

	if((conffd=open(wpasup_file,O_RDWR))==-1){
		perror("Couldn't open wpa_supplicant conf file ");
		exit(1);
	}
    if (stat(wpasup_file, &sbuf) == -1) {
		perror("Couldn't stat wpa_supplicant conf file ");
        exit(1);
    }

	pagesize = getpagesize();

	if(sbuf.st_size<pagesize){
		maxc=sbuf.st_size;
	}else{
//		maxc=pagesize;
		maxc=sbuf.st_size;
		printfc(yellow,"\tLoud Warning: ");printf("File is larger than the pagesize!\n\t\tThis could need big amounts of memory or screw us up...\n");
		int div=(sbuf.st_size)/pagesize;
		if((sbuf.st_size)%pagesize)
			div++;
		pagesize=div*pagesize;
	}
//	long page_size_alternative = sysconf (_SC_PAGESIZE);
//	printfc(green,"pagesize %d | filesize %d | \n",pagesize,sbuf.st_size);

	if((data=mmap((caddr_t)NULL, pagesize, PROT_READ|PROT_WRITE, MAP_SHARED, conffd,0))==(caddr_t)(-1)){
//		fprintf(stderr,"Couldn't memory map wpa_supplicant conf file. ERRNO: %d\n",errno);
		perror("Couldn't memory map wpa_supplicant conf file ");
		exit(1);
	}

	char buf[128];
	int loopcnt,bufidx,i;
	bufidx=0;loopcnt=0;
	for(i=0;i<=maxc;i++){LoopStart:
			//		if ((buf[bufidx] = data[i]) == EOF) {
			//			break;
			//		}
		switch(buf[bufidx]=data[i]){
		case '\n':
			bufidx=0;
			i++;
			goto LoopStart;
			break;
		case '\t':
			i++;
			goto LoopStart;
			break;
		case '=':
			buf[bufidx]='\0';
			if(strcmp(buf,"ssid")==0){
				i++;
				goto WriteLoop;
			}
			break;
		default:
			break;
		}
		bufidx++;
	}
	goto ReachedEndOfFile;

	WriteLoop:
	buf[0]='"';
	memcpy(buf+1,ssid,strlen(ssid));
	buf[strlen(ssid)+1]='"';
	buf[strlen(ssid)+2]='\0';
	bufidx=0;
	for(i;i<maxc;i++){
		if(buf[bufidx]=='\0'){
			goto LineClearLoop;
		}
		if(data[i]=='\n'){
			goto ReachedEndOfFile;
		}
		data[i]=buf[bufidx];
		bufidx++;
	}
	goto ReachedEndOfFile;

	LineClearLoop:
	if(data[i]!='\n'){
		for(i;i<maxc;i++){
			if(data[i]=='\n'){
				goto Finished;
			}
			data[i]=' ';
		}
		goto ReachedEndOfFile;
	}

	Finished:
	munmap(data, pagesize);

	if(running){
		printf("   ..Reconfiguring running Ad-hoc Network.\n");
		char reconfcmd[strlen("wpa_cli -i  reconfigure")+strlen(dev)+1];
		snprintf(reconfcmd,sizeof(reconfcmd),"wpa_cli -i %s reconfigure",dev);
		system(reconfcmd);
	}

	puts("");
	return err;

	ReachedEndOfFile://ReachedEndOfFile is equal to CorruptedFile
	//Take into Account: You could also come here, when the file becomes larger than the pagesize...
	//TODO: Someday... If the filesize is larger than the pagesize (see the check higher there),
	//then we have to load the next page of the file and process through it...
	//Additionally this messy consideration, when the string we search for lies on the breakpoint
	//between the pages...
	munmap(data, pagesize);
	fprintf(stderr,"Corrupted WPA_Supplicant Ad-hoc conf File! Can't continue!\n");
	exit(1);

	puts("");
	return err;
}



int set_adhoc_freq_wpa_supplicant(char *dev,int freq,char running){
	int err;err=0;

	int chan=ieee80211_frequency_to_channel(freq);
	printf("\nSetting Ad-hoc Frequency on %s to \"%d (Channel %d)\"...\n",dev,freq,chan);
	CREATE_PROGRAM_PATH(*args);
	WPA_SUPPLICANT_CONF_SETUP(dev);
	printf("   ...Changing File %s\n",wpasup_file);

    struct stat sbuf;
	int conffd;//Config-File File-Descriptor
	int pagesize;
	int maxc;//The maximum index for data-access of the mmap-file
	char *data;

	if((conffd=open(wpasup_file,O_RDWR))==-1){
		perror("Couldn't open wpa_supplicant conf file ");
		exit(1);
	}
    if (stat(wpasup_file, &sbuf) == -1) {
		perror("Couldn't stat wpa_supplicant conf file ");
        exit(1);
    }

	pagesize = getpagesize();

	if(sbuf.st_size<pagesize){
		maxc=sbuf.st_size;
	}else{
//		maxc=pagesize;
		maxc=sbuf.st_size;
		printfc(yellow,"\tLoud Warning: ");printf("File is larger than the pagesize!\n\t\tThis could need big amounts of memory or screw us up...\n");
		int div=(sbuf.st_size)/pagesize;
		if((sbuf.st_size)%pagesize)
			div++;
		pagesize=div*pagesize;
	}
//	long page_size_alternative = sysconf (_SC_PAGESIZE);
//	printfc(green,"pagesize %d | filesize %d | \n",pagesize,sbuf.st_size);

	if((data=mmap((caddr_t)NULL, pagesize, PROT_READ|PROT_WRITE, MAP_SHARED, conffd,0))==(caddr_t)(-1)){
//		fprintf(stderr,"Couldn't memory map wpa_supplicant conf file. ERRNO: %d\n",errno);
		perror("Couldn't memory map wpa_supplicant conf file ");
		exit(1);
	}

	char buf[128];
	int loopcnt,bufidx,i;
	bufidx=0;loopcnt=0;
	for(i=0;i<=maxc;i++){LoopStart:
			//		if ((buf[bufidx] = data[i]) == EOF) {
			//			break;
			//		}
		switch(buf[bufidx]=data[i]){
		case '\n':
			bufidx=0;
			i++;
			goto LoopStart;
			break;
		case '\t':
			i++;
			goto LoopStart;
			break;
		case '=':
			buf[bufidx]='\0';
			if(strcmp(buf,"frequency")==0){
				i++;
				goto WriteLoop;
			}
			break;
		default:
			break;
		}
		bufidx++;
	}
	goto ReachedEndOfFile;

	WriteLoop:
	snprintf(buf,sizeof(buf),"%d",freq);
	bufidx=0;
	for(i;i<maxc;i++){
		if(buf[bufidx]=='\0'){
			goto LineClearLoop;
		}
		if(data[i]=='\n'){
			goto ReachedEndOfFile;
		}
		data[i]=buf[bufidx];
		bufidx++;
	}
	goto ReachedEndOfFile;

	LineClearLoop:
	if(data[i]!='\n'){
		for(i;i<maxc;i++){
			if(data[i]=='\n'){
				goto Finished;
			}
			data[i]=' ';
		}
		goto ReachedEndOfFile;
	}

	Finished:
	munmap(data, pagesize);

	if(running){
		printf("   ..Reconfiguring running Ad-hoc Network.\n");
		char reconfcmd[strlen("wpa_cli -i  reconfigure")+strlen(dev)+1];
		snprintf(reconfcmd,sizeof(reconfcmd),"wpa_cli -i %s reconfigure",dev);
		system(reconfcmd);
	}

	puts("");
	return err;

	ReachedEndOfFile://ReachedEndOfFile is equal to CorruptedFile
	//Take into Account: You could also come here, when the file becomes larger than the pagesize...
	//TODO: Someday... If the filesize is larger than the pagesize (see the check higher there),
	//then we have to load the next page of the file and process through it...
	//Additionally this messy consideration, when the string we search for lies on the breakpoint
	//between the pages...
	munmap(data, pagesize);
	fprintf(stderr,"Corrupted WPA_Supplicant Ad-hoc conf File! Can't continue!\n");
	exit(1);
}




static int iface_mode_cb(struct nl_msg* msg, void* arg) {
	struct nlmsghdr *got_hdr = nlmsg_hdr(msg);
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];
	uint32_t *ifTypeFreq=((struct CallbackArgPass *)arg)->ArgPointer;
	if (got_hdr->nlmsg_type != expectedId) {
		// what is this??
		return NL_STOP;
	}
	struct genlmsghdr *gnlh = nlmsg_data(got_hdr);
	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),genlmsg_attrlen(gnlh, 0), NULL);
	if (got_attr[NL80211_ATTR_IFTYPE])
		ifTypeFreq[0] = nla_get_u32(got_attr[NL80211_ATTR_IFTYPE]);
	if (got_attr[NL80211_ATTR_WIPHY_FREQ])
		ifTypeFreq[1] = nla_get_u32(got_attr[NL80211_ATTR_WIPHY_FREQ]);
	return 0;
}


static int get_freq_ssid_cb(struct nl_msg* msg, void* arg) {
	intptr_t *cbpass=((struct CallbackArgPass *)arg)->ArgPointer;
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
	if (got_hdr->nlmsg_type != (*(int *)(cbpass[0]))) {
		// what is this??
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
	if (got_attr[NL80211_ATTR_WIPHY_FREQ])
		*(int *)(cbpass[1]) = nla_get_u32(got_attr[NL80211_ATTR_WIPHY_FREQ]);
	if (got_bss[NL80211_BSS_FREQUENCY])
		*(int *)(cbpass[1]) = nla_get_u32(got_bss[NL80211_BSS_FREQUENCY]);
	/*
	 * Getting SSID:
	 * For Explanation, if someday, someone reads this crazy Stuff:
	 * The SSID stands on a Location where NL80211_BSS_INFORMATION_ELEMENTS leads us to.
	 * There is standing a whole Bunch of Bits and Bytes
	 * At the Beginning we can find the SSID
	 * So got_bss[NL80211_BSS_INFORMATION_ELEMENTS] gives us the all-containing struct out of our struct-array got_bss
	 * With nla_data we get the Payload (the actual Data) out of the struct.
	 * The Pointer, coming from nla_data points to the first Address of the Payload.
	 * On the second Address stands the length of the delivered SSID
	 * On the third Address starts the SSID-String, char-by-char
	 */
	if (got_bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
		char *SSIDPointer = nla_data(got_bss[NL80211_BSS_INFORMATION_ELEMENTS]);
		SSIDPointer++;
		unsigned char SSIDLength = *SSIDPointer;
		//To get sure everything is alright
		if ((SSIDLength <= 32) && (SSIDLength <= nla_len(got_bss[NL80211_BSS_INFORMATION_ELEMENTS])-2)) {
			memcpy((char *)(cbpass[2]), SSIDPointer + 1, SSIDLength);
		} else {
			printf(stderr,
					"Something wrong with SSID: Does not fit inside Payload!\n");
			return NL_SKIP;
		}
	} else {
		printf(stderr, "SSID not present!\n");
		return NL_SKIP;
	}
	return 0;
}
int get_freq_ssid(char *dev,int *freq, char *ssid){
	int err;err=0;
	static int ifId;
	ifId=if_nametoindex(dev);
	struct CommandContainer cmd;
	cmd.prepatt = NULL;
	struct nl80211_state sktctr; //contains the nl_socket
	err = nl80211_init_socket(&sktctr);
	int expId = sktctr.nl80211_id;
	intptr_t cbpass[3];
	cbpass[0]=&expId;
	cbpass[1]=freq;
	cbpass[2]=ssid;
	cmd.cmd = NL80211_CMD_GET_SCAN;
	cmd.callbackargpass=cbpass;
	prepareAttribute(&cmd, NL80211_ATTR_IFINDEX, &ifId);
	cmd.nl_msg_flags = NLM_F_DUMP;
	cmd.callbackToUse = get_freq_ssid_cb;
	err = send_with_cmdContainer(&sktctr, 0, NULL, &cmd);
	nl80211_cleanup_socket(&sktctr);
	return err;
}


int get_packeterrorrateTX(char *dev,uint16_t *packerrrate){
	int err;err=0;

	char bufIface[16384];
    struct ifinfomsg *recvdInfo;
    int iface_idx;
    struct rtattr *recvdRtaIface;
	struct nlmsghdr *recvdMsgIface;
	struct nlmsghdr *recvdMsgTemp;
    int attlen;
    char addrIP4[128];
	int bytes_recvd_iface;
	char delivered64;delivered64=0;
	uint16_t packerrrate64;

	struct {
	    struct nlmsghdr nlhdr;
	    struct ifinfomsg infomsg;
	} msg_iface;

	int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	memset(&msg_iface, 0, sizeof(msg_iface));
	msg_iface.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	msg_iface.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	msg_iface.nlhdr.nlmsg_type = RTM_GETLINK;
	msg_iface.infomsg.ifi_family = AF_UNSPEC;

	send(sock, &msg_iface, msg_iface.nlhdr.nlmsg_len, 0);

	bytes_recvd_iface= recv(sock, bufIface, sizeof(bufIface), 0);

	ProcessNextIfaceMsg:
	recvdMsgIface= (struct nlmsghdr *)bufIface;
	while NLMSG_OK(recvdMsgIface, bytes_recvd_iface) {
		recvdInfo = NLMSG_DATA(recvdMsgIface);

		recvdRtaIface = IFLA_RTA(recvdInfo);

		attlen = IFLA_PAYLOAD(recvdMsgIface);
		while RTA_OK(recvdRtaIface, attlen) {
			if (recvdRtaIface->rta_type == IFLA_IFNAME) {
	            if (strcmp(RTA_DATA(recvdRtaIface),dev) != 0) {
	            	break;//It's about the wrong device
	            }
	        }else if (recvdRtaIface->rta_type == IFLA_STATS) {
	        	if((((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->tx_packets)==0){
	        		*packerrrate=0;
	        	}else {
		            *packerrrate=(uint16_t)(((((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->tx_errors)*1000)/(((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->tx_packets));
	        	}
	            depr(32.1,"erronous packs %d | packets %d | RX ERR: %d | RX Packs: %d",((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->tx_errors,((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->tx_packets,((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->rx_errors,((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->rx_packets);
	            depr(32.2,"collisions %d | rx_missed_errors %d",((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->collisions,((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->rx_missed_errors);
	            depr(32.3,"tx_aborted_errors %d | tx_carrier_errors %d | tx_fifo_errors %d\ntx_heartbeat_errors %d | tx_window_errors %d",((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->tx_aborted_errors,((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->tx_carrier_errors,((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->tx_fifo_errors,((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->tx_heartbeat_errors,((struct rtnl_link_stats*)RTA_DATA(recvdRtaIface))->tx_window_errors);
	            depr(32.9,"pack err rate %d | dev %s",*packerrrate,dev);
	        }else if (recvdRtaIface->rta_type == IFLA_STATS64) {
	        	delivered64=1;
	        	if((((struct rtnl_link_stats64*)RTA_DATA(recvdRtaIface))->tx_packets)==0){
	        		packerrrate64=0;
	        	}else {
	        		packerrrate64=(uint16_t)(((((struct rtnl_link_stats64*)RTA_DATA(recvdRtaIface))->tx_errors)*1000)/(((struct rtnl_link_stats64*)RTA_DATA(recvdRtaIface))->tx_packets));
	        	}
	            depr(64.1,"erronous packs %d | packets %d | RX ERR: %d | RX Packs: %d",((struct rtnl_link_stats64*)RTA_DATA(recvdRtaIface))->tx_errors,((struct rtnl_link_stats64*)RTA_DATA(recvdRtaIface))->tx_packets,((struct rtnl_link_stats64*)RTA_DATA(recvdRtaIface))->rx_errors,((struct rtnl_link_stats64*)RTA_DATA(recvdRtaIface))->rx_packets);
	            depr(64.9,"pack err rate %d | dev %s",*packerrrate,dev);
	        }
			recvdRtaIface = RTA_NEXT(recvdRtaIface, attlen);
		}

		recvdMsgIface = NLMSG_NEXT(recvdMsgIface, bytes_recvd_iface);
	}
	LookAfterIfaceDoneMsg:
	bytes_recvd_iface=recv(sock,bufIface,sizeof(bufIface),MSG_DONTWAIT);
	if(bytes_recvd_iface>0){
		//Then we had another Message inside the socket.
		//Check if it was just the "DONE" Message. Nothing to do for them: Check if theres another
		//Message after this.
		//If it isn't a "DONE" Message, then process through it. I.e. jump back before the processing part
		recvdMsgTemp=(struct nlmsghdr *)bufIface;
		if(recvdMsgTemp->nlmsg_type == NLMSG_DONE){
			goto LookAfterIfaceDoneMsg;
		}else{goto ProcessNextIfaceMsg;}
	}//else: everything processed, socket is empty, nothing more to do for the last request. Go one with program

	if(delivered64)
		*packerrrate=packerrrate64;

	return err;
}





int printWLANInterfaces(struct AbsintInterfacesWLAN *wifcollect,struct AbsintInterfaces *ifcollect){
	int err;err=0;
	int i;
	for(i=0;i<wifcollect->wlanc;i++){
		printf("\t%s (IfIndex: %d)\n",(ifcollect->ifacestart)[(wifcollect->wlanidx)[i]],if_nametoindex((ifcollect->ifacestart)[(wifcollect->wlanidx)[i]]));
	}

	return err;
}
/* To check, if a Device is a WLAN-Interface we could do:
 * Send a NL80211 Request to the Kernel to get Interface Attributes, such as Infrastructure Type and Frequency
 * This is only successful, if the Dev is capable of NL80211.
 * After this Request we could check for either: Is a Type delivered, is a Freq delivered or just
 * the error-code from the send_with_cmdContainer Routine. The err should be only '0', if we passed a WLAN-Device
 */
int select_AI_wlan_dev(struct AbsintInterfacesWLAN *wifcollect,struct AbsintInterfaces *ifcollect,struct nl80211_state *sktctr,struct CommandContainer *cmd){
	int err;err=0;
	int i;
	static int ifId;
//	uint32_t ifTypeFreq[2];
	if(wifcollect->wlanidx){
		free(wifcollect->wlanidx);
		wifcollect->wlanidx=NULL;
	}
	wifcollect->wlanc=0;
	for(i=0;i<(ifcollect->ifc);i++){
						/*
						memset(ifTypeFreq,0,sizeof(ifTypeFreq));
						cmd->cmd = NL80211_CMD_GET_INTERFACE;
						cmd->callbackargpass=ifTypeFreq;
						ifId=if_nametoindex((ifcollect->ifacestart)[i]);
						prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifId);
						cmd->nl_msg_flags = 0;
						cmd->callbackToUse = iface_mode_cb;
						err = send_with_cmdContainer(sktctr, 0, NULL, cmd);
						printf("Err: %d | Type: %d | Freq: %d\n",err,ifTypeFreq[0],ifTypeFreq[1]);
						*/
		//Do the quick approach which the error!=0 check
		cmd->cmd = NL80211_CMD_GET_INTERFACE;
		ifId=if_nametoindex((ifcollect->ifacestart)[i]);
		prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifId);
		cmd->nl_msg_flags = 0;
		cmd->callbackToUse = NULL;
		err = send_with_cmdContainer(sktctr, 0, NULL, cmd);
		if(err==0){
			wifcollect->wlanc++;
			if((wifcollect->wlanc)==1){
				(wifcollect->wlanidx)=malloc( (wifcollect->wlanc)*sizeof(*(wifcollect->wlanidx)) );
			} else {
				(wifcollect->wlanidx)=realloc( (wifcollect->wlanidx),(wifcollect->wlanc)*sizeof(*(wifcollect->wlanidx)) );
			}
			(wifcollect->wlanidx)[(wifcollect->wlanc)-1]=i;
		}
	}
	printf("Thereof WLAN-Interfaces:\n");
	printWLANInterfaces(wifcollect,ifcollect);
	return 0;
}








//Kleine Vorlage. Lässt sich gut anpassen um via Netlink Informationen zu IP-Adressen und/oder
//Interfaces zu beziehen.
int netlinkAddressInterfaceVorlage(int argc, char **argstart){
	int err;

	// This is the address we want the interface name for,
	// expressed in dotted-quad format
	char * address_dq = "127.0.0.1";
	// Convert it to decimal format
	unsigned int address;
	inet_pton(AF_INET, address_dq, &address);

	char buf[16384];

	// Our first message will be a header followed by an address payload
	struct {
	    struct nlmsghdr nlhdr;
	    struct ifaddrmsg addrmsg;
	} msg;

	// Our second message will be a header followed by a link payload
	struct {
	    struct nlmsghdr nlhdr;
	    struct ifinfomsg infomsg;
	} msg2;

	struct nlmsghdr *retmsg;

	// Set up the netlink socket
	int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	// Fill in the message
	// NLM_F_REQUEST means we are asking the kernel for data
	// NLM_F_ROOT means provide all the addresses
	// RTM_GETADDR means we want address information
	// AF_INET means limit the response to ipv4 addresses
	memset(&msg, 0, sizeof(msg));
	msg.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	msg.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	msg.nlhdr.nlmsg_type = RTM_GETADDR;
	msg.addrmsg.ifa_family = AF_INET;

	// As above, but RTM_GETLINK means we want link information
	memset(&msg2, 0, sizeof(msg2));
	msg2.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	msg2.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	msg2.nlhdr.nlmsg_type = RTM_GETLINK;
	msg2.infomsg.ifi_family = AF_UNSPEC;

	// Send the first netlink message
	send(sock, &msg, msg.nlhdr.nlmsg_len, 0);

	int len;

	// Get the netlink reply
	len = recv(sock, buf, sizeof(buf), 0);

	retmsg = (struct nlmsghdr *)buf;

	// Loop through the reply messages (one for each address)
	// Each message has a ifaddrmsg structure in it, which
	// contains the prefix length as a member.  The ifaddrmsg
	// structure is followed by one or more rtattr structures,
	// some of which (should) contain raw addresses.
	while NLMSG_OK(retmsg, len) {

	    struct ifaddrmsg *retaddr;
	    retaddr = (struct ifaddrmsg *)NLMSG_DATA(retmsg);
	    int iface_idx = retaddr->ifa_index;

	    struct rtattr *retrta;
	    retrta = (struct rtattr *)IFA_RTA(retaddr);

	    int attlen;
	    attlen = IFA_PAYLOAD(retmsg);

	    char pradd[128];

	    // Loop through the routing information to look for the
	    // raw address.
	    while RTA_OK(retrta, attlen) {
	        if (retrta->rta_type == IFA_ADDRESS) {
	            // Found one -- is it the one we want?
	            unsigned int * tmp = RTA_DATA(retrta);
	            if (address == *tmp) {
	                // Yes!
	                inet_ntop(AF_INET, RTA_DATA(retrta), pradd, sizeof(pradd));
	                printf("Address %s ", pradd);
	                // Now we need to get the interface information
	                // First eat up the "DONE" message waiting for us
	                len = recv(sock, buf, sizeof(buf), 0);
	                // Send the second netlink message and get the reply
	                send(sock, &msg2, msg2.nlhdr.nlmsg_len, 0);
	                len = recv(sock, buf, sizeof(buf), 0);
	                retmsg = (struct nlmsghdr *)buf;
	                while NLMSG_OK(retmsg, len) {
	                    struct ifinfomsg *retinfo;
	                    retinfo = NLMSG_DATA(retmsg);
	                    if (retinfo->ifi_index == iface_idx) {
	                        retrta = IFLA_RTA(retinfo);
	                        attlen = IFLA_PAYLOAD(retmsg);
	                        char prname[128];
	                        // Loop through the routing information
	                        // to look for the interface name.
	                        while RTA_OK(retrta, attlen) {
	                            if (retrta->rta_type == IFLA_IFNAME) {
	                                strcpy(prname, RTA_DATA(retrta));
	                                printf("on %s\n", prname);
	                                exit(EXIT_SUCCESS);
	                            }
	                            retrta = RTA_NEXT(retrta, attlen);

	                        }
	                    }

	                    retmsg = NLMSG_NEXT(retmsg, len);
	                }

	            }
	        }
	        retrta = RTA_NEXT(retrta, attlen);

	    }

	    retmsg = NLMSG_NEXT(retmsg, len);
	}
	return err;
}











int printInterfacesAndAdresses(int argc, char **argstart){
	int err;
	err=0;

	//We do not care about the LocalPort. So we want to exclude it
	//Prepare it here for Skip.
	char *address_local = "127.0.0.1";
	unsigned int localport;
	inet_pton(AF_INET, address_local, &localport);

	char bufAddr[16384];
	char bufIface[16384];
    char ifacename[128];
    struct ifinfomsg *recvdInfo;
    struct ifaddrmsg *recvAddr;
    int iface_idx;
    struct rtattr *recvdRtaAddr;
    struct rtattr *recvdRtaIface;
	struct nlmsghdr *recvdMsgAddr;
	struct nlmsghdr *recvdMsgIface;
    int attlen,attlen2;
    char addrIP4[128];
    unsigned int *tmp;
	int bytes_recvd_addr,bytes_recvd_iface;

	struct {
	    struct nlmsghdr nlhdr;
	    struct ifaddrmsg addrmsg;
	} msg_addr;

	struct {
	    struct nlmsghdr nlhdr;
	    struct ifinfomsg infomsg;
	} msg_iface;


	int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	memset(&msg_addr, 0, sizeof(msg_addr));
	msg_addr.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	msg_addr.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	msg_addr.nlhdr.nlmsg_type = RTM_GETADDR;
	msg_addr.addrmsg.ifa_family = AF_INET;

	memset(&msg_iface, 0, sizeof(msg_iface));
	msg_iface.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	msg_iface.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	msg_iface.nlhdr.nlmsg_type = RTM_GETLINK;
	msg_iface.infomsg.ifi_family = AF_UNSPEC;

	send(sock, &msg_addr, msg_addr.nlhdr.nlmsg_len, 0);

	bytes_recvd_addr = recv(sock, bufAddr, sizeof(bufAddr), 0);

	recvdMsgAddr = (struct nlmsghdr *)bufAddr;

	while NLMSG_OK(recvdMsgAddr, bytes_recvd_addr) {
		recvAddr = (struct ifaddrmsg *)NLMSG_DATA(recvdMsgAddr);
	    iface_idx = recvAddr->ifa_index;

	    recvdRtaAddr = (struct rtattr *)IFA_RTA(recvAddr);

	    attlen = IFA_PAYLOAD(recvdMsgAddr);

	    while RTA_OK(recvdRtaAddr, attlen) {
	        if (recvdRtaAddr->rta_type == IFA_ADDRESS) {
	            tmp = RTA_DATA(recvdRtaAddr);
	            if (localport == *tmp) {
	            	goto ExcludeLocalPort;
	            }
				inet_ntop(AF_INET, RTA_DATA(recvdRtaAddr), addrIP4, sizeof(addrIP4));
				printf("Address %s ", addrIP4);
				bytes_recvd_iface = recv(sock, bufIface, sizeof(bufIface), 0);
				send(sock, &msg_iface, msg_iface.nlhdr.nlmsg_len, 0);
				bytes_recvd_iface = recv(sock, bufIface, sizeof(bufIface), 0);
				recvdMsgIface = (struct nlmsghdr *)bufIface;
				while NLMSG_OK(recvdMsgIface, bytes_recvd_iface) {
					recvdInfo = NLMSG_DATA(recvdMsgIface);
					if (recvdInfo->ifi_index == iface_idx) {
						recvdRtaIface = IFLA_RTA(recvdInfo);
						attlen2 = IFLA_PAYLOAD(recvdMsgIface);
						while RTA_OK(recvdRtaIface, attlen2) {
							if (recvdRtaIface->rta_type == IFLA_IFNAME) {
								strcpy(ifacename, RTA_DATA(recvdRtaIface));
								printf("on %s\n", ifacename);
							}
							recvdRtaIface = RTA_NEXT(recvdRtaIface, attlen2);

						}
					}

					recvdMsgIface = NLMSG_NEXT(recvdMsgIface, bytes_recvd_iface);
				}
	        }
	        ExcludeLocalPort:
	        recvdRtaAddr = RTA_NEXT(recvdRtaAddr, attlen);

	    }

	    recvdMsgAddr = NLMSG_NEXT(recvdMsgAddr, bytes_recvd_addr);
	}
	return err;
}







int getInterfacesAndAdresses(unsigned int *ifc,uintptr_t *ifps,char ***ifacestart,char ***ifmacstart,char ***ifaddrstart){
	int err;
	err=0;
	int i;
	char DonePartsFlags;DonePartsFlags=0;

	//We do not care about the LocalPort. So we want to exclude it
	//Prepare it here for Skip.
	char *localport = "lo";

	char bufAddr[16384];
	char bufIface[16384];
    struct ifinfomsg *recvdInfo;
    struct ifaddrmsg *recvAddr;
    int iface_idx;
    struct rtattr *recvdRtaAddr;
    struct rtattr *recvdRtaIface;
	struct nlmsghdr *recvdMsgAddr;
	struct nlmsghdr *recvdMsgIface;
	struct nlmsghdr *recvdMsgTemp;
    int attlen,attlen2;
    char addrIP4[128];
	int bytes_recvd_addr,bytes_recvd_iface;

	struct {
	    struct nlmsghdr nlhdr;
	    struct ifaddrmsg addrmsg;
	} msg_addr;

	struct {
	    struct nlmsghdr nlhdr;
	    struct ifinfomsg infomsg;
	} msg_iface;


	int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	int sock2 = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	memset(&msg_addr, 0, sizeof(msg_addr));
	msg_addr.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	msg_addr.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	msg_addr.nlhdr.nlmsg_type = RTM_GETADDR;
	msg_addr.addrmsg.ifa_family = AF_INET;

	memset(&msg_iface, 0, sizeof(msg_iface));
	msg_iface.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	msg_iface.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	msg_iface.nlhdr.nlmsg_type = RTM_GETLINK;
	msg_iface.infomsg.ifi_family = AF_UNSPEC;

	send(sock, &msg_iface, msg_iface.nlhdr.nlmsg_len, 0);

	bytes_recvd_iface= recv(sock, bufIface, sizeof(bufIface), 0);


	//Finally we want a nice looking result: An Array of Arrays, where the Adresses/Interfaces are stored
	//The "big" Array shouldn't have more entrys than we have Interfaces and the "small" Arrays should
	//just have the right size of its containing string.
	//We could do this either in one of two ways:
	//1. Go through everything like the "printInterfacesAndAdresses" function and store the stuff in some buffers. We would have to
	//allocate this buffers big enough, where we couldn't know how many Interfaces could come (theoretically could be unlimited...)
	//Like this we would only have to go once through all Netlink messages and than process the buffers
	//	(REMARK: Max size of an interfacename is 128 Byte)
	//2. Go through everything an calculate just the number of interfaces and the sizes of their names.
	//Than allocate appropriate memspaces and go a second trace through everything
	//Better: No big buffers. Worse: More Netlink messaging and processing
	//I choose Method 2

	//After all we have created (malloc, the memory should stay reserved until freed from me ;o) ):
	//- A Memory Area of concatenated strings of arbitrary lengths. This Area will be considered as Array on readout.
	//- An Array of char pointers (char *). Every pointer stores the address of one string from the MemArea.
	//- A two-level char pointer (char **). It points on the first Entry of the pointer Array)
	//- The Counter (int), which contains the number off all found Interfaces.
	//We deliver back:
	//- The counter
	//- The char **

	//First calculate the number and sizes
	//At this point here we have sent the Interface request message and will hold the answer in the Buffer bufiface.
	//This Netlink Request don't have to be resend.
	//But all this Interface message are so many, we do not want to hold them temporarily stored.
	//So resend a Netlink Request every Run-through
	unsigned int ifacecount;

	//Now: The Number and Size Calculation:
	//First go through: Only the Counter
	//Second calculates the sizes
	ifacecount=0;
	ProcessNextIfaceMsg0:
	recvdMsgIface= (struct nlmsghdr *)bufIface;
	while NLMSG_OK(recvdMsgIface, bytes_recvd_iface) {
		recvdInfo = NLMSG_DATA(recvdMsgIface);

		recvdRtaIface = IFLA_RTA(recvdInfo);

		attlen2 = IFLA_PAYLOAD(recvdMsgIface);
		while RTA_OK(recvdRtaIface, attlen2) {
			if (recvdRtaIface->rta_type == IFLA_IFNAME) {
	            if (strcmp(RTA_DATA(recvdRtaIface),localport) == 0) {
	            	//Do nothing. We won't count the local port
	            }else{ifacecount++;}
//	            printf("dev %s\n",RTA_DATA(recvdRtaIface));
	        }
			recvdRtaIface = RTA_NEXT(recvdRtaIface, attlen2);
		}

		recvdMsgIface = NLMSG_NEXT(recvdMsgIface, bytes_recvd_iface);
	}
	LookAfterIfaceDoneMsg0:
	bytes_recvd_iface=recv(sock,bufIface,sizeof(bufIface),MSG_DONTWAIT);
	if(bytes_recvd_iface>0){
		//Then we had another Message inside the socket.
		//Check if it was just the "DONE" Message. Nothing to do for them: Check if theres another
		//Message after this.
		//If it isn't a "DONE" Message, then process through it. I.e. jump back before the processing part
		recvdMsgTemp=(struct nlmsghdr *)bufIface;
		if(recvdMsgTemp->nlmsg_type == NLMSG_DONE){
			goto LookAfterIfaceDoneMsg0;
		}else{goto ProcessNextIfaceMsg0;}
	}//else: everything processed, socket is empty, nothing more to do for the last request. Go one with program
//	printf("Found %d Interfaces\n",ifacecount);
	*ifc=ifacecount;
	(*ifacestart) = malloc(ifacecount*sizeof(char *));
	(*ifmacstart) = malloc(ifacecount*sizeof(char *));
	(*ifaddrstart) = malloc(ifacecount*sizeof(char *));
//	*ifps=sizeof(char *)/8;//REMARK: Addresses and hence Additions to Pointers are considered Bytewise.
						//This means: If you add +1 to a Pointer it gets incremented by 8.
	*ifps=1;


	//We have reserved the Array to store the Pointers for the actual Iface and Address Infos in it.
	//But at this Run-through we misuse this Array to store the later on needed lengths in it.
	//For the actual Use we need the Arrays not until later and so we can save some Memory-Space.
	//Now calculating the sizes of the names
	memset(bufIface,0,sizeof(bufIface));
	send(sock, &msg_iface, msg_iface.nlhdr.nlmsg_len, 0);
	bytes_recvd_iface= recv(sock, bufIface, sizeof(bufIface), 0);



	unsigned char curIface;
	curIface=0;
	ProcessNextIfaceMsg1:
	recvdMsgIface = (struct nlmsghdr *)bufIface;
	while NLMSG_OK(recvdMsgIface, bytes_recvd_iface) {
		recvdInfo = NLMSG_DATA(recvdMsgIface);
		recvdRtaIface = IFLA_RTA(recvdInfo);
		attlen2 = IFLA_PAYLOAD(recvdMsgIface);
		while RTA_OK(recvdRtaIface, attlen2) {
			if (recvdRtaIface->rta_type == IFLA_IFNAME) {
				if (strcmp(RTA_DATA(recvdRtaIface),localport) == 0) {
					//Do nothing. We won't count the local port
				}else{
					*((*ifaddrstart)+(curIface*(*ifps)))=(char *)(strlen(RTA_DATA(recvdRtaIface))+1);//+1 because of the End-of-String Symbol \0
//					printf("len: %d\n",*((*ifaddrstart)+(curIface*(*ifps))));
					curIface++;
					if(do_debug >= 4){
						ANSICOLORSET(ANSI_COLOR_YELLOW);
						printf("\t\tDEBUG: strlen(Portname) -- Port-Address-Inquiry: %d\n",strlen(RTA_DATA(recvdRtaIface))+1);
						ANSICOLORRESET;
					}
				}
			}
			recvdRtaIface = RTA_NEXT(recvdRtaIface, attlen2);

		}

		recvdMsgIface = NLMSG_NEXT(recvdMsgIface, bytes_recvd_iface);
	}
	LookAfterIfaceDoneMsg1:
	bytes_recvd_iface=recv(sock,bufIface,sizeof(bufIface),MSG_DONTWAIT);
	if(bytes_recvd_iface>0){
		//Then we had another Message inside the socket.
		//Check if it was just the "DONE" Message. Nothing to do for them: Check if theres another
		//Message after this.
		//If it isn't a "DONE" Message, then process through it. I.e. jump back before the processing part
		recvdMsgTemp=(struct nlmsghdr *)bufIface;
		if(recvdMsgTemp->nlmsg_type == NLMSG_DONE){
			goto LookAfterIfaceDoneMsg1;
		}else{goto ProcessNextIfaceMsg1;}
	}//else: everything processed, socket is empty, nothing more to do for the last request. Go one with program

	//Now allocate the space for the Interface-Names and connect the Pointers
	curIface=0;
	for(i=0;i<ifacecount;i++){
		curIface+=(uintptr_t)(*((*ifaddrstart)+(i*(*ifps))));
	}
	*(*ifacestart)=malloc(curIface);
	for(i=1;i<ifacecount;i++){
		*((*ifacestart)+(i*(*ifps)))=(*((*ifacestart)+(i-1)*(*ifps)))+(uintptr_t)(*((*ifaddrstart)+(i-1)*(*ifps)));
	}


	//MAC-Addresses
	*(*ifmacstart)=malloc(ifacecount*6);//MAC-Address in numerical form needs 6 Bytes
	for(i=1;i<ifacecount;i++){
		*((*ifmacstart)+(i*(*ifps)))=*((*ifmacstart)+(i-1)*(*ifps))+(uintptr_t)(6);
	}
	memset(*(*ifmacstart),0,ifacecount*6);


	//The IP4 Addresses do have a defined size
	//So we allocate the space and connect the Pointers
	//Now, after the misuse of the memory space, we can use the address save space for its right purpose
	//and connect the pointers
	*(*ifaddrstart)=malloc(ifacecount*sizeof(struct in_addr));//Would be sizeof(struct in6_addr), if we would ask for IP6
	for(i=1;i<ifacecount;i++){
		*((*ifaddrstart)+(i*(*ifps)))=*((*ifaddrstart)+(i-1)*(*ifps))+(uintptr_t)(sizeof(struct in_addr));
	}
	memset(*(*ifaddrstart),0,ifacecount*sizeof(struct in_addr));



	//Consume the last "DONE" Message and all this other stuff, we don't need (if any...)
	while((bytes_recvd_iface=recv(sock,bufIface,sizeof(bufIface),MSG_DONTWAIT)) != -1){
		if(do_debug >= 4){
			ANSICOLORSET(ANSI_COLOR_YELLOW);
			printf("\t\tDEBUG: Bytes received Inter-Function -- Port-Address-Inquiry\n");
			printf("\t\t\tright after Port-Name-Size Extraction: %d\n",bytes_recvd_iface);
			ANSICOLORRESET;
		}
	}
	memset(bufIface,0,sizeof(bufIface));
	memset(bufAddr,0,sizeof(bufAddr));
	send(sock, &msg_iface, msg_iface.nlhdr.nlmsg_len, 0);
	bytes_recvd_iface= recv(sock, bufIface, sizeof(bufIface), 0);
	curIface=0;
	recvdMsgIface= (struct nlmsghdr *)bufIface;


	//Here comes the readout and storage of the Addresses and Interfaces.
	//So finally let's write the Addresses and Interfaces in the allocated Memory
	ProcessNextIfaceMsg2:
	recvdMsgIface = (struct nlmsghdr *)bufIface;
	while NLMSG_OK(recvdMsgIface, bytes_recvd_iface) {
		recvdInfo = NLMSG_DATA(recvdMsgIface);
	    iface_idx = recvdInfo->ifi_index;
		recvdRtaIface = IFLA_RTA(recvdInfo);
		attlen2 = IFLA_PAYLOAD(recvdMsgIface);
		while RTA_OK(recvdRtaIface, attlen2) {
			if (recvdRtaIface->rta_type == IFLA_IFNAME) {
				if (strcmp(RTA_DATA(recvdRtaIface),localport) == 0) {
					goto ExcludeLocalPort2;
				}
//				printf("Device: %s\n",RTA_DATA(recvdRtaIface));
				strcpy(*((*ifacestart)+curIface*(*ifps)), RTA_DATA(recvdRtaIface));
				FLAG_SET(DonePartsFlags,0x01);
			} else if (recvdRtaIface->rta_type == IFLA_ADDRESS) {
//				printf("MAC: ");
//				printMAC(RTA_DATA(recvdRtaIface),6);
//				printf("\n");
				memcpy(*((*ifmacstart)+curIface*(*ifps)),RTA_DATA(recvdRtaIface),6);
				FLAG_SET(DonePartsFlags,0x02);
			} else {
				//Uncomment for Debug Information (What else is delivered? With what Value?)
//				int tempint;tempint=*((int *)(RTA_DATA(recvdRtaIface)));
//				printf("\tType: %d | Value: %d\n",recvdRtaIface->rta_type,tempint);
			}
			recvdRtaIface = RTA_NEXT(recvdRtaIface, attlen2);
		}
		if((DonePartsFlags & 0x01) && (DonePartsFlags & 0x02)){
			DonePartsFlags=0;
		}else {
			//Than this was a peculiar Message/Interface delivered. Hm, skip it...
			DonePartsFlags=0;
			goto ExcludeLocalPort2;
		}
		send(sock2, &msg_addr, msg_addr.nlhdr.nlmsg_len, 0);
		bytes_recvd_addr= recv(sock2, bufAddr, sizeof(bufAddr), 0);
		ProcessNextAddrMsg2:
		recvdMsgAddr = (struct nlmsghdr *)bufAddr;
		while NLMSG_OK(recvdMsgAddr, bytes_recvd_addr) {
			recvAddr = (struct ifaddrmsg *)NLMSG_DATA(recvdMsgAddr);
    		if (recvAddr->ifa_index == iface_idx) {
				recvdRtaAddr = (struct rtattr *)IFA_RTA(recvAddr);
				attlen = IFA_PAYLOAD(recvdMsgAddr);
				while RTA_OK(recvdRtaAddr, attlen) {
					if (recvdRtaAddr->rta_type == IFA_ADDRESS) {
			            memcpy(*((*ifaddrstart)+curIface*(*ifps)),RTA_DATA(recvdRtaAddr),sizeof(struct in_addr));
					}
					recvdRtaAddr = RTA_NEXT(recvdRtaAddr, attlen);
				}
    		}
			recvdMsgAddr = NLMSG_NEXT(recvdMsgAddr, bytes_recvd_addr);
		}
		LookAfterAddrDoneMsg2:
		bytes_recvd_addr=recv(sock2,bufAddr,sizeof(bufAddr),MSG_DONTWAIT);
		if(bytes_recvd_addr>0){
			//Then we had another Message inside the socket.
			//Check if it was just the "DONE" Message. Nothing to do for them: Check if theres another
			//Message after this.
			//If it isn't a "DONE" Message, then process through it. I.e. jump back before the processing part
			recvdMsgTemp=(struct nlmsghdr *)bufAddr;
			if(recvdMsgTemp->nlmsg_type == NLMSG_DONE){
				goto LookAfterAddrDoneMsg2;
			}else{goto ProcessNextAddrMsg2;}
		}//else: everything processed, socket is empty, nothing more to do for the last request. Go one with program

		curIface++;
        ExcludeLocalPort2:
		recvdMsgIface = NLMSG_NEXT(recvdMsgIface, bytes_recvd_iface);
	}
	LookAfterIfaceDoneMsg2:
	bytes_recvd_iface=recv(sock,bufIface,sizeof(bufIface),MSG_DONTWAIT);
	if(bytes_recvd_iface>0){
		//Then we had another Message inside the socket.
		//Check if it was just the "DONE" Message. Nothing to do for them: Check if theres another
		//Message after this.
		//If it isn't a "DONE" Message, then process through it. I.e. jump back before the processing part
		recvdMsgTemp=(struct nlmsghdr *)bufIface;
		if(recvdMsgTemp->nlmsg_type == NLMSG_DONE){
			goto LookAfterIfaceDoneMsg2;
		}else{goto ProcessNextIfaceMsg2;}
	}//else: everything processed, socket is empty, nothing more to do for the last request. Go one with program

	close(sock);
	close(sock2);
	printf("Found following Interface <-> MAC <-> IP4-Address Pairs:\n");
	uint64_t tempmac;
	for(i=0;i<ifacecount;i++){
		printf("\t%s - ",*((*ifacestart)+i*(*ifps)));
		tempmac=0;
		memcpy(&tempmac,*((*ifmacstart)+i*(*ifps)),6);
		if(tempmac==0){
			printf("No Valid HW-Addr");
		}else{
			printMAC(*((*ifmacstart)+i*(*ifps)),6);
		}
		if(**((*ifaddrstart)+i*(*ifps))==0){
			printf(" - No IP4-Address assigned!\n");
		}else{
			inet_ntop(AF_INET,*((*ifaddrstart)+i*(*ifps)),addrIP4,sizeof(addrIP4));
			printf(" - %s\n",addrIP4);
		}
	}
	return err;
}
//Detects only Interfaces with configured, valid IP4 Address
//NOTE: Current Implementation only detects IP4 Addresses and hence only Ports, which have assigned an IP4 Address
//IP6 Add-on would be possible
int getConfiguredInterfacesAndAdresses(unsigned int *ifc,uintptr_t *ifps,char ***ifacestart,char ***ifmacstart,char ***ifaddrstart){
	int err;
	err=0;
	int i;
	char DonePartsFlags;DonePartsFlags=0;

	//We do not care about the LocalPort. So we want to exclude it
	//Prepare it here for Skip.
	char *address_local = "127.0.0.1";
	unsigned int localport;
	inet_pton(AF_INET, address_local, &localport);

	char bufAddr[16384];
	char bufIface[16384];
    struct ifinfomsg *recvdInfo;
    struct ifaddrmsg *recvAddr;
    int iface_idx;
    struct rtattr *recvdRtaAddr;
    struct rtattr *recvdRtaIface;
	struct nlmsghdr *recvdMsgAddr;
	struct nlmsghdr *recvdMsgIface;
	struct nlmsghdr *recvdMsgTemp;
    int attlen,attlen2;
    char addrIP4[128];
    unsigned int *tmp;
	int bytes_recvd_addr,bytes_recvd_iface;

	struct {
	    struct nlmsghdr nlhdr;
	    struct ifaddrmsg addrmsg;
	} msg_addr;

	struct {
	    struct nlmsghdr nlhdr;
	    struct ifinfomsg infomsg;
	} msg_iface;


	int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	memset(&msg_addr, 0, sizeof(msg_addr));
	msg_addr.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	msg_addr.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	msg_addr.nlhdr.nlmsg_type = RTM_GETADDR;
	msg_addr.addrmsg.ifa_family = AF_INET;

	memset(&msg_iface, 0, sizeof(msg_iface));
	msg_iface.nlhdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	msg_iface.nlhdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	msg_iface.nlhdr.nlmsg_type = RTM_GETLINK;
	msg_iface.infomsg.ifi_family = AF_UNSPEC;

	send(sock, &msg_addr, msg_addr.nlhdr.nlmsg_len, 0);

	bytes_recvd_addr = recv(sock, bufAddr, sizeof(bufAddr), 0);

	recvdMsgAddr = (struct nlmsghdr *)bufAddr;

    //And immediately consume the "DONE" message. Throw it in the Iface-Variables. Will be overwritten later on, when the buffer is needed...
	//I.e. consume everything, until socket is empty to clean up everything.
	while((bytes_recvd_iface=recv(sock,bufIface,sizeof(bufIface),MSG_DONTWAIT)) != -1){
		if(do_debug >= 4){
			ANSICOLORSET(ANSI_COLOR_YELLOW);
			printf("\t\tDEBUG: Bytes received Inter-Function -- Port-Address-Inquiry\n");
			printf("\t\t\tat count calculation: %d\n",bytes_recvd_iface);
			ANSICOLORRESET;
		}
	}

	//Finally we want a nice looking result: An Array of Arrays, where the Adresses/Interfaces are stored
	//The "big" Array shouldn't have more entrys than we have Interfaces and the "small" Arrays should
	//just have the right size of its containing string.
	//We could do this either in one of two ways:
	//1. Go through everything like the "printInterfacesAndAdresses" function and store the stuff in some buffers. We would have to
	//allocate this buffers big enough, where we couldn't know how many Interfaces could come (theoretically could be unlimited...)
	//Like this we would only have to go once through all Netlink messages and than process the buffers
	//	(REMARK: Max size of an interfacename is 128 Byte)
	//2. Go through everything an calculate just the number of interfaces and the sizes of their names.
	//Than allocate appropriate memspaces and go a second trace through everything
	//Better: No big buffers. Worse: More Netlink messaging and processing
	//I choose Method 2

	//After all we have created (malloc, the memory should stay reserved until freed from me ;o) ):
	//- A Memory Area of concatenated strings of arbitrary lengths. This Area will be considered as Array on readout.
	//- An Array of char pointers (char *). Every pointer stores the address of one string from the MemArea.
	//- A two-level char pointer (char **). It points on the first Entry of the pointer Array)
	//- The Counter (int), which contains the number off all found Interfaces.
	//We deliver back:
	//- The counter
	//- The char **

	//First calculate the number and sizes
	//At this point here we have sent the Address request message and will hold the answer in the Buffer bufAddr.
	//This Netlink Request don't have to be resend.
	//But all this Interface message are so many, we do not want to hold them temporarily stored.
	//So resend a Netlink Request every Run-through
	unsigned int ifacecount;

	//Now: The Number and Size Calculation:
	//First go through: Only the Counter
	//Second calculates the sizes
	ifacecount=0;
	while NLMSG_OK(recvdMsgAddr, bytes_recvd_addr) {
		recvAddr = (struct ifaddrmsg *)NLMSG_DATA(recvdMsgAddr);
	    iface_idx = recvAddr->ifa_index;

	    recvdRtaAddr = (struct rtattr *)IFA_RTA(recvAddr);

	    attlen = IFA_PAYLOAD(recvdMsgAddr);

	    while RTA_OK(recvdRtaAddr, attlen) {
	        if (recvdRtaAddr->rta_type == IFA_ADDRESS) {
	            tmp = RTA_DATA(recvdRtaAddr);
	            if (localport == *tmp) {
	            	//Do nothing. We won't count the local port
	            }else{ifacecount++;}
	        }
	        recvdRtaAddr = RTA_NEXT(recvdRtaAddr, attlen);

	    }

	    recvdMsgAddr = NLMSG_NEXT(recvdMsgAddr, bytes_recvd_addr);
	}
//	printf("Found %d Interfaces\n",ifacecount);
	*ifc=ifacecount;
	(*ifacestart) = malloc(ifacecount*sizeof(char *));
	(*ifmacstart) = malloc(ifacecount*sizeof(char *));
	(*ifaddrstart) = malloc(ifacecount*sizeof(char *));
//	*ifps=sizeof(char *)/8;//REMARK: Addresses and hence Additions to Pointers are considered Bytewise.
						//This means: If you add +1 to a Pointer it gets incremented by 8.
	*ifps=1;


	//We have reserved the Array to store the Pointers for the actual Iface and Address Infos in it.
	//But at this Run-through we misuse this Array to store the later on needed lengths in it.
	//For the actual Use we need the Arrays not until later and so we can save some Memory-Space.
	//Now calculating the sizes of the names
	memset(bufAddr,0,bytes_recvd_addr);
	send(sock, &msg_addr, msg_addr.nlhdr.nlmsg_len, 0);
	bytes_recvd_addr = recv(sock, bufAddr, sizeof(bufAddr), 0);
	recvdMsgAddr = (struct nlmsghdr *)bufAddr;

	//And again clean the socket (just to get sure, it is empty. You'll never know how forthcoming your specific Kernel is...
	//several spam you down with info-messages no one want ;oP )
	while((bytes_recvd_iface=recv(sock,bufIface,sizeof(bufIface),MSG_DONTWAIT)) > 0){
		if(do_debug >= 4){
			ANSICOLORSET(ANSI_COLOR_YELLOW);
			printf("\t\tDEBUG: Bytes received Inter-Function -- Port-Address-Inquiry\n");
			printf("\t\t\tat Port-Name-Size Extraction: %d\n",bytes_recvd_iface);
			ANSICOLORRESET;
		}
	}


	unsigned char curIface;
	curIface=0;
	while NLMSG_OK(recvdMsgAddr, bytes_recvd_addr) {
		recvAddr = (struct ifaddrmsg *)NLMSG_DATA(recvdMsgAddr);
	    iface_idx = recvAddr->ifa_index;

	    recvdRtaAddr = (struct rtattr *)IFA_RTA(recvAddr);

	    attlen = IFA_PAYLOAD(recvdMsgAddr);

	    while RTA_OK(recvdRtaAddr, attlen) {
	        if (recvdRtaAddr->rta_type == IFA_ADDRESS) {
	            tmp = RTA_DATA(recvdRtaAddr);
	            if (localport == *tmp) {
	            	goto ExcludeLocalPortLength;
	            }
				//Consumation of previous requested messages should be done at this point
	            //    either before the big while, or before the jump back.
				//Then send next request
				send(sock, &msg_iface, msg_iface.nlhdr.nlmsg_len, 0);
				bytes_recvd_iface = recv(sock, bufIface, sizeof(bufIface), 0);
				ProcessNextMsg:
				recvdMsgIface = (struct nlmsghdr *)bufIface;
				while NLMSG_OK(recvdMsgIface, bytes_recvd_iface) {
					recvdInfo = NLMSG_DATA(recvdMsgIface);
					if (recvdInfo->ifi_index == iface_idx) {
						recvdRtaIface = IFLA_RTA(recvdInfo);
						attlen2 = IFLA_PAYLOAD(recvdMsgIface);
						while RTA_OK(recvdRtaIface, attlen2) {
							if (recvdRtaIface->rta_type == IFLA_IFNAME) {
								*((*ifaddrstart)+(curIface*(*ifps)))=(char *)(strlen(RTA_DATA(recvdRtaIface))+1);//+1 because of the End-of-String Symbol \0
								curIface++;
								if(do_debug >= 4){
									ANSICOLORSET(ANSI_COLOR_YELLOW);
									printf("\t\tDEBUG: strlen(Portname) -- Port-Address-Inquiry: %d\n",strlen(RTA_DATA(recvdRtaIface))+1);
									ANSICOLORRESET;
								}
							}
							recvdRtaIface = RTA_NEXT(recvdRtaIface, attlen2);

						}
					}

					recvdMsgIface = NLMSG_NEXT(recvdMsgIface, bytes_recvd_iface);
				}
				LookAfterDoneMsg:
				bytes_recvd_iface=recv(sock,bufIface,sizeof(bufIface),MSG_DONTWAIT);
				if(bytes_recvd_iface>0){
					//Then we had another Message inside the socket.
					//Check if it was just the "DONE" Message. Nothing to do for them: Check if theres another
					//Message after this.
					//If it isn't a "DONE" Message, then process through it. I.e. jump back before the processing part
					recvdMsgTemp=(struct nlmsghdr *)bufIface;
					if(recvdMsgTemp->nlmsg_type == NLMSG_DONE){
						goto LookAfterDoneMsg;
					}else{goto ProcessNextMsg;}
				}//else: everything processed, socket is empty, nothing more to do for the last request. Go one with program
	        }
	        ExcludeLocalPortLength:
	        recvdRtaAddr = RTA_NEXT(recvdRtaAddr, attlen);

	    }

	    recvdMsgAddr = NLMSG_NEXT(recvdMsgAddr, bytes_recvd_addr);
	}

	//Now allocate the space for the Interface-Names and connect the Pointers
	curIface=0;
	for(i=0;i<ifacecount;i++){
		curIface+=(uintptr_t)(*((*ifaddrstart)+(i*(*ifps))));
	}
	*(*ifacestart)=malloc(curIface);
	for(i=1;i<ifacecount;i++){
		*((*ifacestart)+(i*(*ifps)))=(*((*ifacestart)+(i-1)*(*ifps)))+(uintptr_t)(*((*ifaddrstart)+(i-1)*(*ifps)));
	}


	//MAC-Addresses
	*(*ifmacstart)=malloc(ifacecount*6);//MAC-Address in numerical form needs 6 Bytes
	for(i=1;i<ifacecount;i++){
		*((*ifmacstart)+(i*(*ifps)))=*((*ifmacstart)+(i-1)*(*ifps))+(uintptr_t)(6);
	}


	//The IP4 Addresses do have a defined size
	//So we allocate the space and connect the Pointers
	//Now, after the misuse of the memory space, we can use the address save space for its right purpose
	//and connect the pointers
	*(*ifaddrstart)=malloc(ifacecount*sizeof(struct in_addr));//Would be sizeof(struct in6_addr), if we would ask for IP6
	for(i=1;i<ifacecount;i++){
		*((*ifaddrstart)+(i*(*ifps)))=*((*ifaddrstart)+(i-1)*(*ifps))+(uintptr_t)(sizeof(struct in_addr));
	}



	//Consume the last "DONE" Message and all this other stuff, we don't need (if any...)
	while((bytes_recvd_iface=recv(sock,bufIface,sizeof(bufIface),MSG_DONTWAIT)) != -1){
		if(do_debug >= 4){
			ANSICOLORSET(ANSI_COLOR_YELLOW);
			printf("\t\tDEBUG: Bytes received Inter-Function -- Port-Address-Inquiry\n");
			printf("\t\t\tright after Port-Name-Size Extraction: %d\n",bytes_recvd_iface);
			ANSICOLORRESET;
		}
	}
	memset(bufAddr,0,bytes_recvd_addr);
//	memset(bufAddr,0,sizeof(bufAddr));
	send(sock, &msg_addr, msg_addr.nlhdr.nlmsg_len, 0);
	bytes_recvd_addr = recv(sock, bufAddr, sizeof(bufAddr), 0);
	recvdMsgAddr = (struct nlmsghdr *)bufAddr;
	curIface=0;

	//And again, to get on the safe side. Clean up.
	while((bytes_recvd_iface=recv(sock,bufIface,sizeof(bufIface),MSG_DONTWAIT)) != -1){
		if(do_debug >= 4){
			ANSICOLORSET(ANSI_COLOR_YELLOW);
			printf("\t\tDEBUG: Bytes received Inter-Function -- Port-Address-Inquiry\n");
			printf("\t\t\tright before Array Filling: %d\n",bytes_recvd_iface);
			ANSICOLORRESET;
		}
	}

	//Here comes the readout and storage of the Addresses and Interfaces.
	//So finally let's write the Addresses and Interfaces in the allocated Memory
	while NLMSG_OK(recvdMsgAddr, bytes_recvd_addr) {
		recvAddr = (struct ifaddrmsg *)NLMSG_DATA(recvdMsgAddr);
	    iface_idx = recvAddr->ifa_index;

	    recvdRtaAddr = (struct rtattr *)IFA_RTA(recvAddr);

	    attlen = IFA_PAYLOAD(recvdMsgAddr);

	    while RTA_OK(recvdRtaAddr, attlen) {
	        if (recvdRtaAddr->rta_type == IFA_ADDRESS) {
	            tmp = RTA_DATA(recvdRtaAddr);
	            if (localport == *tmp) {
	            	goto ExcludeLocalPort;
	            }
	            memcpy(*((*ifaddrstart)+curIface*(*ifps)),RTA_DATA(recvdRtaAddr),sizeof(struct in_addr));
				//Consumation of previous requested messages should be done at this point
	            //    either before the big while, or before the jump back.
				//Then send next request
				send(sock, &msg_iface, msg_iface.nlhdr.nlmsg_len, 0);
				bytes_recvd_iface = recv(sock, bufIface, sizeof(bufIface), 0);
				ProcessNextMsg2:
				recvdMsgIface = (struct nlmsghdr *)bufIface;
				while NLMSG_OK(recvdMsgIface, bytes_recvd_iface) {
					recvdInfo = NLMSG_DATA(recvdMsgIface);
					if (recvdInfo->ifi_index == iface_idx) {
						recvdRtaIface = IFLA_RTA(recvdInfo);
						attlen2 = IFLA_PAYLOAD(recvdMsgIface);
						while RTA_OK(recvdRtaIface, attlen2) {
							if (recvdRtaIface->rta_type == IFLA_IFNAME) {
								strcpy(*((*ifacestart)+curIface*(*ifps)), RTA_DATA(recvdRtaIface));
								FLAG_SET(DonePartsFlags,0x01);
//								printf("Device: %s\n",RTA_DATA(recvdRtaIface));
							} else if (recvdRtaIface->rta_type == IFLA_ADDRESS) {
								memcpy(*((*ifmacstart)+curIface*(*ifps)),RTA_DATA(recvdRtaIface),6);
								FLAG_SET(DonePartsFlags,0x02);
							} else {
								//Uncomment for Debug Information (What else is delivered? With what Value?)
//								int tempint;tempint=*((int *)(RTA_DATA(recvdRtaIface)));
//								printf("\tType: %d | Value: %d\n",recvdRtaIface->rta_type,tempint);
							}
							if((DonePartsFlags & 0x01) && (DonePartsFlags & 0x02)){
								DonePartsFlags=0;
								curIface++;
							}
							recvdRtaIface = RTA_NEXT(recvdRtaIface, attlen2);

						}
					}
					else{
						recvdRtaIface = IFLA_RTA(recvdInfo);
						attlen2 = IFLA_PAYLOAD(recvdMsgIface);
						while RTA_OK(recvdRtaIface, attlen2) {
							if (recvdRtaIface->rta_type == IFLA_IFNAME) {
								printf("Other Devices: %s\n",RTA_DATA(recvdRtaIface));
							}
							recvdRtaIface = RTA_NEXT(recvdRtaIface, attlen2);

						}
					}

					recvdMsgIface = NLMSG_NEXT(recvdMsgIface, bytes_recvd_iface);
				}
				LookAfterDoneMsg2:
				bytes_recvd_iface=recv(sock,bufIface,sizeof(bufIface),MSG_DONTWAIT);
				if(bytes_recvd_iface>0){
					//Then we had another Message inside the socket.
					//Check if it was just the "DONE" Message. Nothing to do for them: Check if theres another
					//Message after this.
					//If it isn't a "DONE" Message, then process through it. I.e. jump back before the processing part
					recvdMsgTemp=(struct nlmsghdr *)bufIface;
					if(recvdMsgTemp->nlmsg_type == NLMSG_DONE){
						goto LookAfterDoneMsg2;
					}else{goto ProcessNextMsg2;}
				}//else: everything processed, socket is empty, nothing more to do for the last request. Go one with program
	        }
	        ExcludeLocalPort:
	        recvdRtaAddr = RTA_NEXT(recvdRtaAddr, attlen);

	    }

	    recvdMsgAddr = NLMSG_NEXT(recvdMsgAddr, bytes_recvd_addr);
	}
	close(sock);
	printf("Found following Interface <-> MAC <-> IP4-Address Pairs:\n");
	for(i=0;i<ifacecount;i++){
		printf("\t%s - ",*((*ifacestart)+i*(*ifps)));
		printMAC(*((*ifmacstart)+i*(*ifps)),6);
		inet_ntop(AF_INET,*((*ifaddrstart)+i*(*ifps)),addrIP4,sizeof(addrIP4));
		printf(" - %s\n",addrIP4);
	}
	return err;
}








/*
 * ND: Neighbour Discovery:
 */




//This gets a **char to a collection of the IP4 Addresses, that are assigned to the devices Interfaces
//From them it extracts the disjunct Subnets.
//In every Subnet the Function sweeps Pings over the whole Range of IPs
//		--> SEE: For now it sweeps only over the last 256 Addresses (i.e. after the third Point
//				---> X.X.X.0-255 )
//TODO:
//For sure we should take the netmask into account at some day...
//And also: Doublicated Subnets are not excluded. If the Station operates with multiple Addresses in the same Subnet,
//then this Subnet is completely sweeped for every Address.
//
//After this the ARP Neighbours are automagically present in the tables (=> rtnetlink.h)
int populateARP(unsigned int ifc,char **ifaddrstart){
	int err;err=0;
	int i,j;
	unsigned char myaddr;
	struct in_addr mycompleteaddr, nulladdr;
	memset(&nulladdr,0,sizeof(nulladdr));
	char *endpoint;
//	char addrPrint[16];//4*3 Numbers + 3 Dots + EndOfString '\0'
	char pingcmd[43];//"ping -c 1" + addrPrint[without '\0'] + Ausgabe-Kanal-Umleitung[>/dev/null 2>&1] + " &\0"
	memset(pingcmd,0,sizeof(pingcmd));
	snprintf(pingcmd,11,"ping -c 3 ");
	char *lastdot;

	for(i=0;i<ifc;i++){
		memcpy(&mycompleteaddr,*(ifaddrstart+i),sizeof(mycompleteaddr));
		if(memcmp(&mycompleteaddr,&nulladdr,sizeof(struct in_addr))==0){
			//Skip, if no proper address is assigned
			continue;
		}
		memset(pingcmd+10,0,sizeof(pingcmd)-10);
		inet_ntop(AF_INET, *(ifaddrstart+i), (pingcmd+10), sizeof(pingcmd)-5);
		//find the last Point in the Address-String.
		lastdot=pingcmd+sizeof(pingcmd)-3;//'-3' because we do not have to check the last character. The last character is always a number, the first one before that can be another number or the dot.
											//'-1' compensates the +sizeof() goes 1 above the bound of the string
												// (We do not use "strlen()" because this does many Operations. We anyway do a bit of a loop over the string
												// Because of this the number of Loop Iterations we would economize with strlen() doesn't really save Performance
												// with sizeof() we get a quick value and the Economization of the Operations from strlen() are more then
												// the ones we additionally have to spend in our own loop.
		CheckNextChar:
		if((*lastdot)!='.'){
			lastdot--;
			goto CheckNextChar;
		}else{lastdot++;}//Set the Pointer to Position after the dot.
//		myaddr=(char)strtol(lastdot, &endpoint, 10);
		myaddr=*((char*)(*(ifaddrstart+i))+3);
//		printfc(yellow,"myaddr: %d\n",myaddr);
		memset(lastdot,'\0',sizeof(pingcmd)-(lastdot-pingcmd));//Clean everything after the last dot to NULL / EndOfString
		for(j=0;j<myaddr;j++){
			snprintf(lastdot,22,"%d >/dev/null 2>&1 &",j);
//			printfc(red,"Ping-Command: %s\n", pingcmd);
			system(pingcmd);
		}
		for(j=myaddr+1;j<256;j++){
			snprintf(lastdot,22,"%d >/dev/null 2>&1 &",j);
//			printfc(red,"Ping-Command: %s\n", pingcmd);
			system(pingcmd);
		}
		//Loop over Subnet
	}
	return err;
}






int printNeighbours(struct DiscoveredNeighbours *neighbours){
	int err;err=0;
	int i;
	char bufTemp[16];

	for(i=0;i<neighbours->neighc;i++){
		printf("\t");
		printf("Port: %s | ",((*(neighbours->dnstart)+i))->dev);

		format_host(AF_INET,
				sizeof((*(neighbours->dnstart)+i)->IP4),
				&((*(neighbours->dnstart)+i)->IP4),
				bufTemp, sizeof(bufTemp));
		printf("IP4: %s | ",bufTemp);
		printf("MAC: ");
		printMAC(((*(neighbours->dnstart)+i))->mac,sizeof(((*(neighbours->dnstart)+i))->mac));

		int nud = ((*(neighbours->dnstart)+i))->state;
		printf(" | ");
		#define PRINT_FLAG(f) if (nud & NUD_##f) { \
			nud &= ~NUD_##f; printf(#f "%s", nud ? "," : ""); }
				PRINT_FLAG(INCOMPLETE);
				PRINT_FLAG(REACHABLE);
				PRINT_FLAG(STALE);
				PRINT_FLAG(DELAY);
				PRINT_FLAG(PROBE);
				PRINT_FLAG(FAILED);
				PRINT_FLAG(NOARP);
				PRINT_FLAG(PERMANENT);
		#undef PRINT_FLAG

		printf("\n");
	}

	return err;
}


int getNeighboursFromARP(struct DiscoveredNeighbours *neighbours){
	int err;err=0;
	int i;

	int bytes_recvd;
	char bufRecv[16384];

	int sock = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);

	/* Because we could start this multiple times at Runtime to renew the discovered Neighbours:
	 * Clean the allocated spaces with already before got Network-Partners, if any.
	 */
	if(neighbours->dnstart){
		free(*(neighbours->dnstart));
		free(neighbours->dnstart);
		neighbours->dnstart=NULL;
	}
	neighbours->neighc=0;
	/*Allocate a small initial memspace, just for one entry
	 * Like this it works smooth with the generic Allocation further down and the
	 * 'Bad Pointer' Pointer Problem from the second "realloc" is handled
	 * Bad thing: If we do not have any Neighbour discovered, then this initial memspace here
	 * is wasted without any usage. But then everything is screwed up anyway... and we quit the Program
	 * so... who cares...
	 * Good thing: We could implement a concept like: Check everytime vor the Neighbour-Counter == 1
	 * And handle the second "realloc" in another way, but i think this would be CPU-Time thrown into
	 * Garbage... I prefer to waste some memory in worst cases ;o)
	 */
	neighbours->dnstart=malloc(sizeof(struct DNeigh *));
	*(neighbours->dnstart)=malloc(sizeof(struct DNeigh));


	struct ndmsg ndm = { 0 };
	ndm.ndm_family = AF_UNSPEC;

	struct nlmsghdr nlhead;
	struct sockaddr_nl nladdrSend = { .nl_family = AF_NETLINK };
	struct iovec iovSend[2] = {
		{ .iov_base = &nlhead, .iov_len = sizeof(nlhead) },
		{ .iov_base = &ndm, .iov_len = sizeof(struct ndmsg) }
	};
	struct msghdr msgSend = {
		.msg_name = &nladdrSend,
		.msg_namelen = 	sizeof(nladdrSend),
		.msg_iov = iovSend,
		.msg_iovlen = 2,
	};

	nlhead.nlmsg_len = NLMSG_LENGTH(sizeof(struct ndmsg));
	nlhead.nlmsg_type = RTM_GETNEIGH;
	nlhead.nlmsg_flags = NLM_F_DUMP|NLM_F_REQUEST;
	nlhead.nlmsg_pid = 0;
//	nlhead.nlmsg_seq = rth.dump = ++rth.seq;


	struct sockaddr_nl nladdr;
	struct iovec iov;
	struct msghdr msg = {
		.msg_name = &nladdr,
		.msg_namelen = sizeof(nladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	iov.iov_base = bufRecv;
	struct nlmsghdr *nlmsgrecv;


	struct ndmsg *ndmsg;
	struct rtattr * rta[NDA_MAX+1];
	int len;
	char bufTemp[256];


	sendmsg(sock, &msgSend, 0);


	while(1){
		iov.iov_len = sizeof(bufRecv);
		bytes_recvd=recvmsg(sock,&msg,MSG_DONTWAIT);
//		printf("Bytes Recvd: %d\n",bytes_recvd);
		if(bytes_recvd<0){
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			fprintf(stderr, "netlink receive error %s (%d)\n",strerror(errno),errno);
			return -1;
		}
		if (bytes_recvd == 0) {
			fprintf(stderr, "Netlink closed Connection\n");
			return -1;
		}


		nlmsgrecv = (struct nlmsghdr *)bufRecv;
		while (NLMSG_OK(nlmsgrecv, bytes_recvd)) {
			/*TODO: Something like the Tracking of
			 * sockaddr_nl->nl_pid
			 * nlmsghdr->nlmsg_pid
			 * nlmsghdr->nlmsg_seq
			 *in case of Socket Multi-Use. If one of them doesn't match ( || ) Skip the msg
			 */
			if (nlmsgrecv->nlmsg_type == NLMSG_DONE) {
				break;
			}
			if (nlmsgrecv->nlmsg_type == NLMSG_ERROR) {
				struct nlmsgerr *err = (struct nlmsgerr*)NLMSG_DATA(nlmsgrecv);
				if (nlmsgrecv->nlmsg_len < NLMSG_LENGTH(sizeof(struct nlmsgerr))) {
					fprintf(stderr,
						"ERROR truncated\n");
				} else {
					errno = -err->error;
					if ((errno == ENOENT ||
					     errno == EOPNOTSUPP))//Hm, maybe an andalso ( && ) Check for NETLINK_SOCK_DIAG?
						return -1;

					perror("RTNETLINK answers");
				}
				return -1;
			}

			//Here comes the Netlink-Message read-out and extraction of the Neighbours
			ndmsg = NLMSG_DATA(nlmsgrecv);
			len=nlmsgrecv->nlmsg_len;

			if (nlmsgrecv->nlmsg_type != RTM_NEWNEIGH && nlmsgrecv->nlmsg_type != RTM_DELNEIGH &&
					nlmsgrecv->nlmsg_type != RTM_GETNEIGH) {
				fprintf(stderr, "Not RTM_NEWNEIGH: %08x %08x %08x\n",
						nlmsgrecv->nlmsg_len, nlmsgrecv->nlmsg_type, nlmsgrecv->nlmsg_flags);

				return 0;
			}
			len -= NLMSG_LENGTH(sizeof(*ndmsg));
			if (len < 0) {
				fprintf(stderr, "ERROR: Bad Netlink Msg Length: %d\n", len);
				return -1;
			}

			parse_rtattr(rta, NDA_MAX, NDA_RTA(ndmsg), nlmsgrecv->nlmsg_len - NLMSG_LENGTH(sizeof(*ndmsg)));

			/* Filter the States:
			 * We de not consider the 'FAILED' Adresses. Under these isn't any Device reachable, so don't care about
			 * In Case of 'Incomplete': There was something send to this Addr, but neither a
			 * response came nor a Timeout was thrown. This means: If we have done a Ping request (sweep) to
			 * discover the Network, then we read out the ARP Tables to early after.
			 * We have to wait a bit and try the thing again.
			 */
			if (ndmsg->ndm_state) {
				switch(ndmsg->ndm_state){
				case NUD_FAILED:
					goto SkipMsg;
					break;
				case NUD_INCOMPLETE:
					return FUNC_ERR_TRY_AGAIN;
				default:
					break;
				}
			}
			if (ndmsg->ndm_family) {//SEE: Here we skip some Families, like IP6. We actually only need the IP4
				switch(ndmsg->ndm_family){
				case AF_MPLS:
				case AF_IPX:
				case AF_DECnet:
				case AF_INET6:
					goto SkipMsg;
					break;
				default:
					break;
				}
			}
			/*Read out the Device (Interface) over which this HW-Platform reaches the Network-Device of the current Message.
			 * If it's the "local", then skip it
			 * else: Store the Device for later use down there
			 */
			char bufDev[IFNAMSIZ];
			if(ndmsg->ndm_ifindex){
				if (if_indextoname(ndmsg->ndm_ifindex, bufDev) == NULL){
					snprintf(bufDev, IFNAMSIZ, "IfIdx%d", ndmsg->ndm_ifindex);
				}
				if(strcmp(bufDev,"lo")==0){
					goto SkipMsg;
				}
//				printf("Dev: %s ",bufDev);
			}
			(neighbours->neighc)++;
			neighbours->dnstart=realloc(neighbours->dnstart,(neighbours->neighc)*sizeof(struct DNeigh *));
			*(neighbours->dnstart)=realloc(*(neighbours->dnstart),(neighbours->neighc)*sizeof(struct DNeigh));
			(neighbours->dnstart)[(neighbours->neighc)-1]=(neighbours->dnstart)[0]+((neighbours->neighc)-1);//Remember: Compiler knows what Kind of Pointer we have here, so it knows the size of the pointed Object. This means its like there is a *sizeof(struct DNeigh) at the End

			if (nlmsgrecv->nlmsg_type == RTM_DELNEIGH){
//				printf("delete ");
			}
			else if (nlmsgrecv->nlmsg_type == RTM_GETNEIGH){
//				printf("miss ");
			}

			memcpy(((neighbours->dnstart)[(neighbours->neighc)-1])->dev,bufDev,IFNAMSIZ);
			if (rta[NDA_DST]) {
//				format_host(ndmsg->ndm_family,
//						RTA_PAYLOAD(rta[NDA_DST]),
//						RTA_DATA(rta[NDA_DST]),
//						bufTemp, sizeof(bufTemp));
//				printf("host: %s ",bufTemp);
				memcpy(&(((neighbours->dnstart)[(neighbours->neighc)-1])->IP4),
						RTA_DATA(rta[NDA_DST]),
						sizeof(((neighbours->dnstart)[(neighbours->neighc)-1])->IP4));
			}
			if (rta[NDA_LLADDR]) {
//				unsigned char buf1[64];
//				memcpy(buf1,RTA_DATA(rta[NDA_LLADDR]),RTA_PAYLOAD(rta[NDA_LLADDR]));
//				printf("HW-Addr: ");
//				printf("%02X",buf1[0]);
//				for(i=1;i<RTA_PAYLOAD(rta[NDA_LLADDR]);i++){
//					printf(":%02X",buf1[i]);
//				}
				memcpy(((neighbours->dnstart)[(neighbours->neighc)-1])->mac,
						RTA_DATA(rta[NDA_LLADDR]),
						sizeof(((neighbours->dnstart)[(neighbours->neighc)-1])->mac));
			}

			if (ndmsg->ndm_flags & NTF_ROUTER) {
//				printf(" router");
			}
			if (ndmsg->ndm_flags & NTF_PROXY) {
//				printf(" proxy");
			}

			if (ndmsg->ndm_state) {
//				int nud = ndmsg->ndm_state;
//				printf(" ");
//
//				#define PRINT_FLAG(f) if (nud & NUD_##f) { \
//					nud &= ~NUD_##f; printf(#f "%s", nud ? "," : ""); }
//						PRINT_FLAG(INCOMPLETE);
//						PRINT_FLAG(REACHABLE);
//						PRINT_FLAG(STALE);
//						PRINT_FLAG(DELAY);
//						PRINT_FLAG(PROBE);
//						PRINT_FLAG(FAILED);
//						PRINT_FLAG(NOARP);
//						PRINT_FLAG(PERMANENT);
//				#undef PRINT_FLAG
				((neighbours->dnstart)[(neighbours->neighc)-1])->state=ndmsg->ndm_state;
			}
//			printf("\n");


			SkipMsg:
			nlmsgrecv = NLMSG_NEXT(nlmsgrecv, bytes_recvd);
		}


		if (msg.msg_flags & MSG_TRUNC) {
			fprintf(stderr, "Message truncated\n");
			continue;
		}
	}
	/* After all we do have to reconnect all the pointers again.
	 * Because of the usage of realloc, the memspace of the concatenated pointers
	 * could be moved
	 */
	for(i=1;i<neighbours->neighc;i++){
		(neighbours->dnstart)[i]=(neighbours->dnstart)[i-1]+1;//Remember: Compiler knows what Kind of Pointer we have here, so it knows the size of the pointed Object. This means its like there is a sizeof(struct DNeigh) instead of +1 at the End
	}

	close(sock);
	printf("Discovered following Neighbours\n");
	printNeighbours(neighbours);
	return err;
}




int getPortPeer(struct AbsintInterfaces *ifcollect){
	int err;err=0;

	err=populateARP(ifcollect->ifc,ifcollect->ifaddrstart);

	//Wait a bit to get sure every response from the network came back respectively is timed out
	struct timespec remainingdelay;
	remainingdelay.tv_sec = 1;
	remainingdelay.tv_nsec = 0;
	do {
		err = nanosleep(&remainingdelay, &remainingdelay);
	} while (err<0);

	GetNeighAgain:
	err=getNeighboursFromARP(&(ifcollect->neighbours));
	switch(err){
	case FUNC_ERR_TRY_AGAIN:
		remainingdelay.tv_sec = 0;
		remainingdelay.tv_nsec = 500000000;
		do {
			err = nanosleep(&remainingdelay, &remainingdelay);
		} while (err<0);
		goto GetNeighAgain;
		break;
	default:
		break;
	}

	return err;
}




char check_if_dev_exists(char *dev,struct AbsintInterfaces *ifcollect){
	int err;err=0;
	int i;
	for(i=0;i<(ifcollect->ifc);i++){
		if(strcmp((ifcollect->ifacestart)[i],dev)==0)
			err=1;
	}
	return err;
}




#undef NO_ABSINT_NETF_C_FUNCTIONS
