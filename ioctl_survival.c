/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#define NO_IOCTL_SURVIVAL_C_FUNCTIONS

#include "ollerus.h"

//#include <linux/if.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/wireless.h>
#include <linux/if_arp.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/ioctl.h>



int ioctl_get_txpower (int *txpowerdest){
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	struct iwreq req = {
	.ifr_name = "wlan0",
	};
//	snprintf(req.ifr_name, 6, "%s", WhatWeWant.interfacename);
	memcpy(req.ifr_name,WhatWeWant.interfacename,5);//IFNAMSIZ);
//	printf("\ntest: %s\n%s\n",WhatWeWant.interfacename,req.ifr_name);

	// Getting Trasmission power in req
	if(ioctl(sockfd, SIOCGIWTXPOW, &req) == -1) {
		printf("\nError performing SIOCGIWTXPOW on [%s]\nCouldn't get the current Transmission Power!\n", WhatWeWant.interfacename);
		close(sockfd);
		return OPERATION_ERR_STD;
	}
	//Extract the power
	*txpowerdest = req.u.txpower.value;

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



int get_access_point_local_ip (struct sockaddr_in *dest){

	struct ifaddrs *myaddrs, *ifa;
	int err;
	err = getifaddrs(&myaddrs);
	if (err != 0){
		fprintf(stderr, "getifaddrs failed!\nSo i couldn't get Access Points IP.\nHence i'm not able to communicate with it\n Do:|\n");
		return err;
	}
	for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next){
		if (NULL == ifa->ifa_addr){
		  continue;
		}
//		if ((ifa->ifa_flags & IFF_UP) == 0) {
		if ((ifa->ifa_flags & IFF_UP) == 0) {
		  continue;
		}
		if(strcmp(ifa->ifa_name,"wlan0")==0) {
			switch (((struct sockaddr_in *)ifa->ifa_addr)->sin_family) {
			case AF_INET:
				memcpy(dest, (struct sockaddr_in *)ifa->ifa_ifu.ifu_dstaddr, sizeof(struct sockaddr_in));
				break;
			case AF_INET6:
				break;
			default:
				break;
			}
		}
	}
	freeifaddrs(myaddrs);

	return 0;
}



#undef NO_IOCTL_SURVIVAL_C_FUNCTIONS
