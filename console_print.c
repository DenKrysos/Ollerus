/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#include "ollerus.h"

void printfreq (unsigned int *freq) {
	printf("\tFrequency: %.3f GHz (Channel %d)", (float) (*freq) / 1000, ieee80211_frequency_to_channel(*freq));
}
void printStaBSS (struct StationInfo *stainf) {
	printf("\tBSS Flags:");
	if (stainf->flags & STA_BSS_CTS_PROT)
		printf("\n\t\tCTS Protection");
	if (stainf->flags & STA_BSS_SHORT_PREAMBLE)
		printf("\n\t\tShort Preamble");
	if (stainf->flags & STA_BSS_SHORT_SLOT_TIME)
		printf("\n\t\tShort-Slot-Time");
	printf("\n");
	printf("\tdtim period: %d\n", stainf->dtim);
	printf("\tbeacon int: %d\n", stainf->beacon);
}
void printRecTr (struct StationInfo *stainf) {
	printf("\tRX: %u bytes (%u packets)\n",stainf->RX_Bytes,stainf->RX_Packets);
	printf("\tTX: %u bytes (%u packets)\n",stainf->TX_Bytes,stainf->TX_Packets);
	printf("\tSignal Level: %d dBm",stainf->SigLvl);
}
void printStationInfo (struct StationInfo *stainf) {
    printf("\tBitrate: %d.%d Mbit/s",(stainf->bitrate) / 10,(stainf->bitrate) % 10);
    if (stainf->MCS != (unsigned char)(-1)) {
		printf(", ");
		printf(((stainf->flags) & STA_INF_VHT_MCS) ? "VHT-MCS %d" : "MCS %d", stainf->MCS);
    }
    printf("\n\t\t");

    switch (stainf->width) {
    case STA_WIDTH_10_MHZ:
    	printf("Channel width 10MHz");
    	break;
    case STA_WIDTH_20_MHZ:
    	printf("Channel width 20MHz");
    	break;
    case STA_WIDTH_25_MHZ:
    	printf("Channel width 25MHz");
    	break;
    case STA_WIDTH_40_MHZ:
    	printf("Channel width 40MHz");
    	break;
    case STA_WIDTH_80_MHZ:
    	printf("Channel width 80MHz");
    	break;
    case STA_WIDTH_80P80_MHZ:
    	printf("Channel width 80P80MHz");
    	break;
    case STA_WIDTH_160_MHZ:
    	printf("Channel width 160MHz");
    	break;
    default:
    	break;
    }

    if (stainf->flags & STA_INF_SHORT_GI)
    	printf(", short GI");

}

void link_print_console () {
	printf("\tESSID: %s\n", WhatWeWant.essid);
	printf("\tConnected to (MAC): ");
	printMAC_fromStr(WhatWeWant.MAC_ConnectedTo, 6);//sizeof(WhatWeWant.MAC_ConnectedTo)); //Additional 10 Bytes of bloody Information. Maybe someday useful...
	printf("\n");
	printf("\t    (on Interface: %s)\n", WhatWeWant.interfacename);
	printfreq(&(WhatWeWant.frequency));
	printf("\n");
	printf("\tType: %d\n", WhatWeWant.type);
	//TODO: When someday TxPower is implemented in cfg80211, better do it over Netlink
	//And thereby change the type of output
	printf("\tTransmission Power: %d %s\n", WhatWeWant.owntranspower, (WhatWeWant.flags & WLAN_CON_DATA_TXPOWER_DBM) ? ((WhatWeWant.flags & WLAN_CON_DATA_TXPOWER_MW)?"(Unspecified Unit)":"dBm") : ((WhatWeWant.flags & WLAN_CON_DATA_TXPOWER_MW) ? "mW" :"(Unspecified Unit)"));
//	printf("Own MAC Address: ");
//	printMAC(WhatWeWant.MAC_Own, 6);//sizeof(WhatWeWant.MAC_Own);
//	printf("\n");
	printf("\n");
	printStationInfo(&(WhatWeWant.stainf));
	printf("\n");
	printStaBSS(&(WhatWeWant.stainf));
	printf("\n");
	printRecTr(&(WhatWeWant.stainf));
	printf("\n");
//	printf("transmission power: %d dBm\n",WhatWeWant.transpower);
}
