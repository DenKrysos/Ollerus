/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

//#include <expat.h>
//#include <expat_config.h>
//#include <expat_external.h>
#include <libxml/parser.h>

#include "ollerus.h"


#define XMLFILEPATH "//home//zbox01//test.txt"




void fileprintfreq (FILE *file, unsigned int *freq) {
	fprintf(file, "Frequency: %.3f GHz (Channel %d)", (float) (*freq) / 1000, ieee80211_frequency_to_channel(*freq));
}
void fileprintStaBSS (FILE *file, struct StationInfo *stainf) {
	fprintf(file, "BSS Flags:");
	if (stainf->flags & STA_BSS_CTS_PROT)
		fprintf(file, "\n\tCTS Protection");
	if (stainf->flags & STA_BSS_SHORT_PREAMBLE)
		fprintf(file, "\n\tShort Preamble");
	if (stainf->flags & STA_BSS_SHORT_SLOT_TIME)
		fprintf(file, "\n\tShort-Slot-Time");
	fprintf(file, "\n");
	fprintf(file, "dtim period: %d\n", stainf->dtim);
	fprintf(file, "beacon int: %d\n", stainf->beacon);
}
void fileprintRecTr (FILE *file, struct StationInfo *stainf) {
	fprintf(file, "RX: %u bytes (%u packets)\n",stainf->RX_Bytes,stainf->RX_Packets);
	fprintf(file, "TX: %u bytes (%u packets)\n",stainf->TX_Bytes,stainf->TX_Packets);
	fprintf(file, "Signal Level: %d dBm",stainf->SigLvl);
}
void fileprintStationInfo (FILE *file, struct StationInfo *stainf) {
    fprintf(file, "Bitrate: %d.%d Mbit/s",(stainf->bitrate) / 10,(stainf->bitrate) % 10);
    if (stainf->MCS != (unsigned char)(-1)) {
		fprintf(file, ", ");
		fprintf(file, ((stainf->flags) & STA_INF_VHT_MCS) ? "VHT-MCS %d" : "MCS %d", stainf->MCS);
    }
    fprintf(file, "\n\t");

    switch (stainf->width) {
    case STA_WIDTH_10_MHZ:
    	fprintf(file, "Channel width 10MHz");
    	break;
    case STA_WIDTH_20_MHZ:
    	fprintf(file, "Channel width 20MHz");
    	break;
    case STA_WIDTH_25_MHZ:
    	fprintf(file, "Channel width 25MHz");
    	break;
    case STA_WIDTH_40_MHZ:
    	fprintf(file, "Channel width 40MHz");
    	break;
    case STA_WIDTH_80_MHZ:
    	fprintf(file, "Channel width 80MHz");
    	break;
    case STA_WIDTH_80P80_MHZ:
    	fprintf(file, "Channel width 80P80MHz");
    	break;
    case STA_WIDTH_160_MHZ:
    	fprintf(file, "Channel width 160MHz");
    	break;
    default:
    	break;
    }

    if (stainf->flags & STA_INF_SHORT_GI)
    	fprintf(file, ", short GI");
}
void fileprintMAC (FILE *file, unsigned char *MAC, int length) {
	int i;

	// Find the real length of the String
	for(length; WhatWeWant.MAC_ConnectedTo[length] == '\0'; length--) {
		if (length < 0)
			goto NothingToPrint;
	}
	// length now holds the max index
	// And now we are able to properly print it.
	for(i=0; i<length; i++) {
		fprintf(file, "%02X:", WhatWeWant.MAC_ConnectedTo[i]);
	}

	fprintf(file, "%02X", WhatWeWant.MAC_ConnectedTo[length]);

	NothingToPrint:
//	fprintf(file, "\nDebug-Stringlength: %d\n", length);
	;
}



int fileOperation() {
	int entrycnt;
	char buf[100];

    FILE *wlandatafile;
    wlandatafile = fopen(XMLFILEPATH, "a+");
    if (!wlandatafile) {
    	goto file_not_opened;
    }


    fseek(wlandatafile, 0, SEEK_END);

//    printf("\n\n");
//    printf("%c", fgetc(wlandatafile));
//    printf("%c", fgetc(wlandatafile));
//    printf("%c", fgetc(wlandatafile));
//    printf("%c", fgetc(wlandatafile));
//    printf("%c", fgetc(wlandatafile));
//    fseek(wlandatafile, 0, SEEK_END);
//    printf("%c", fgetc(wlandatafile));
//    printf("%c", fgetc(wlandatafile));
//    printf("%c", fgetc(wlandatafile));
//    printf("%c", fgetc(wlandatafile));
//    printf("%c", fgetc(wlandatafile));

    fprintf(wlandatafile, "\n________________________________________\n");
    fprintf(wlandatafile, "|\tEntry #\n");
    fprintf(wlandatafile, "----------------------------------------\n");
	fprintf(wlandatafile, "\n");
	fprintf(wlandatafile, "ESSID: %s\n", WhatWeWant.essid);
	fprintf(wlandatafile, "Connected to (MAC): ");
	fileprintMAC(wlandatafile,WhatWeWant.MAC_ConnectedTo, 6);//sizeof(WhatWeWant.MAC_ConnectedTo)); //Additional 10 Bytes of bloody Information. Maybe someday useful...
	fprintf(wlandatafile, "\n");
	fprintf(wlandatafile, "(on Interface: %s)\n", WhatWeWant.interfacename);
	fileprintfreq(wlandatafile, &(WhatWeWant.frequency));
	fprintf(wlandatafile, "\n");
	fprintf(wlandatafile, "Type: %d\n", WhatWeWant.type);
//    fprintf(wlandatafile, "Own MAC Address: ");
//    fileprintMAC(wlandatafile, WhatWeWant.MAC_Own, 6);//sizeof(WhatWeWant.MAC_Own);
//    fprintf(wlandatafile, "\n");
	fprintf(wlandatafile, "\n");
	fileprintStationInfo(wlandatafile,&(WhatWeWant.stainf));
    fprintf(wlandatafile, "\n");
    fileprintStaBSS(wlandatafile,&(WhatWeWant.stainf));
    fprintf(wlandatafile, "\n");
    fileprintRecTr(wlandatafile,&(WhatWeWant.stainf));
    fprintf(wlandatafile, "\n");
//    fprintf(wlandatafile, "transmission power: %d dBm\n",WhatWeWant.transpower);



    if (fclose(wlandatafile)) {
    	goto file_not_closed;
    }


    return 0;
	file_not_opened:
	fprintf(stderr, "File could not be opened!");
	return FILE_ERR_NOT_OPENED;
	file_not_closed:
	fprintf(stderr, "File could not be closed successfully!");
	return FILE_ERR_NOT_CLOSED;
}


int fileOperationChained() {

	return 0;
}
