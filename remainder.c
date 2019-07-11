/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#define REMAINDER_C
#define NO_REMAINDER_C_FUNCTIONS

#include <sys/socket.h>
#include </usr/include/linux/wireless.h>
#include </usr/include/linux/if_arp.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#include "ollerus_globalsettings.h"
#include "ollerus_base.h"
#include "nl80211.h"
#include "remainder.h"
#include "function_creator.h"

#include "ollerus_extern_functions.h"
//-------END ioctl-------//

/*
 * Just a bunch of helpful functions/makros
 * Especially some used for debugging/developing
 * Or something nice like the ansi_escape_use setting
 */



int senddetermined (int socket, char *msg, int msglen){
	int bytes_sent;
//	msglen = strlen(msg);
	SendAgain:
	bytes_sent = send(socket, msg, msglen, 0);
	switch(bytes_sent) {
	case -1:
		//Something gone wrong. Read out errno for further detail
		return errno;
		break;
	default:
		if(bytes_sent==msglen) {
			//Everything sent, everything done. Just fine and we are finished.
		} else if(bytes_sent<msglen) {
			//Not everything sent, but the beginning. Go on and send the rest.
			msg=msg+bytes_sent;
			msglen=msglen-bytes_sent;
			goto SendAgain;
		} else {
			//How could this happen o.O ?
			//SHUTDOWN!!!
			//SHUTDOWN!!!
			//Just kidding. Should never occur...
			return OPERATION_ERR_NEVER_HAPPEN;
		}
		break;
	}
	return 0;
}


void printMAC_fromStr (unsigned char *MAC, int length){
	int i;
	length--;
	// Find the real length of the String
	for(length; MAC[length] == '\0'; length--) {
		if (length < 0)
			goto NothingToPrint;
	}
	// length now holds the max index
	// And now we are able to properly print it.
	for(i=0; i<length; i++) {
		printf("%02X:", MAC[i]);
	}

	printf("%02X", MAC[length]);

	NothingToPrint:
//	printf("\nDebug-Stringlength: %d\n", length);
	;
}
void printMAC (unsigned char *MAC, int len, ...){
	//Only if len!=6 are optional arguments taken into account.
	//Just pass 'anything' but '0' as optional argument to make the function force to print MAC-like formated Data with any length and suppress the warnings.
	int i;
	va_list variadics;

	va_start(variadics,len);
	char option = (char)va_arg(variadics,int);

	if(!len){
		len=sizeof(MAC);
	}

	if((len<6) && (0==option)){
		printf("\nWARNING: You gave me an Array shorter than 6 Byte.Has: %d.\nI'll print the few you gave me, to help you debug ;o)\n",len);
	} else if((len>6) && (0==option)){
		printf("\nWARNING: You gave me an Array bigger than 6 Byte.Has: %d.\nI use the first six Byte.\n",len);
		len=6;
	}

	len--;
	for(i=0; i<len; i++) {
		printf("%02X:", MAC[i]);
	}

	printf("%02X", MAC[len]);

	NothingToPrint:
//	printf("\nDebug-Stringlength: %d\n", length);
	;
}





CREATE_PRINT_DAYTIME(,,stdout)
CREATE_PRINT_DAYTIME(_file,FILE *file,file)
//The Buffer needs at least 31 Bytes (the Null-Terminating-Character included)
CREATE_PRINT_DAYTIME(_buffer,char *buffer,buffer)

int GetRealDayTime(struct timespec *ts, struct tm *tim){
	GetRealTimeM(nano,ts,CLOCK_REALTIME);
	localtime_r(&(ts->tv_sec), tim);
	tim->tm_mon+=1;
	tim->tm_year+=1900;
	return 0;
}






time_t tmToSeconds(struct tm *tim){
	//Give this Function the "Real Numbers", not the "adjusted since Epoch" ones.
	//I.e. The month value has to match "Real Numbers", like 1-12, not 0-11
	//The Year value has to represent the actual year instead of beeing subtracted by 1900, i.e. 2017, not 117


    // Month-to-day offset for non-leap-years.
    static const int month_day[12] =
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    // Calculation is actually very easy. There is only one difficulty: Leap Years.
    time_t month = (tim->tm_mon - 1) % 12;
    time_t year = (tim->tm_year - 1900) + (tim->tm_mon - 1) / 12;
    if (month < 0) {   // Negative values % 12 are still negative.
        month += 12;
        --year;
    }

    // This is the number of Februaries since 1900.
    const int year_for_leap = (month > 1) ? year + 1 : year;

	tzset();
    time_t res = tim->tm_sec                         // Seconds
        + 60 * (tim->tm_min                          // Minute = 60 seconds
        + 60 * (tim->tm_hour                         // Hour = 60 minutes
        + 24 * (month_day[month] + tim->tm_mday - 1  // Day = 24 hours
        + 365 * (year - 70)                          // Year = 365 days
        + (year_for_leap - 69) / 4                   // Every 4 years is     leap...
        - (year_for_leap - 1) / 100                  // Except centuries...
        + (year_for_leap + 299) / 400)))             // Except 400s.
		+ timezone;                                  // Adjust the timezone. This is a global variable from <time.h>, set by tzset();
    return res < 0 ? -1 : res;
}






int ieee80211_channel_to_frequency(int chan, enum nl80211_band band){
	/* see 802.11 17.3.8.3.2 and Annex J
	 * there are overlapping channel numbers in 5GHz and 2GHz bands */
	if (chan <= 0)
		return 0; /* not supported */
	switch (band) {
	case NL80211_BAND_2GHZ:
		if (chan == 14)
			return 2484;
		else if (chan < 14)
			return 2407 + chan * 5;
		break;
	case NL80211_BAND_5GHZ:
		if (chan >= 182 && chan <= 196)
			return 4000 + chan * 5;
		else
			return 5000 + chan * 5;
		break;
	case NL80211_BAND_60GHZ:
		if (chan < 5)
			return 56160 + chan * 2160;
		break;
	default:
		;
	}
	return 0; /* not supported */
}


int ieee80211_frequency_to_channel(int freq){
	/* see 802.11-2007 17.3.8.3.2 and Annex J */
	if (freq == 2484)
		return 14;
	else if (freq < 2484)
		return (freq - 2407) / 5;
	else if (freq >= 4910 && freq <= 4980)
		return (freq - 4000) / 5;
	else if (freq <= 45000) /* DMG band lower limit */
		return (freq - 5000) / 5;
	else if (freq >= 58320 && freq <= 64800)
		return (freq - 56160) / 2160;
	else
		return 0;
}


int getDigitCountofInt(int n){
    if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    /*      2147483647 is 2^31-1 - add more ifs as needed
       and adjust this final return as well. */
    return 10;
}






char check_system_endianess(char *endianess){
#define ENDIAN_CHECK_VALUE ((char)(0x11))
	int n = ENDIAN_CHECK_VALUE;
	// TRUE: little endian
	if(*((char *)(&n)) == ENDIAN_CHECK_VALUE){
		*endianess = ENDIANESS_LITTLE;
		printfc(gray,"Info: ");printf("System is Little Endian.\n");
		return ENDIANESS_LITTLE;
	} else if(*(((char *)(&n))+sizeof(int)-1) == ENDIAN_CHECK_VALUE){
		printfc(gray,"Info: ");printf("System is Big Endian.\n");
		*endianess = ENDIANESS_BIG;
		return ENDIANESS_BIG;
	} else{
		printfc(yellow,"WARNING: ");printf("Couldn't get the System Endianess. Thus using Little Endian.\n");
		printf("This may cause some Operations like Network-Communication to not work properly...\n");
		*endianess = ENDIANESS_LITTLE;
		return ENDIANESS_LITTLE;
	}
#undef ENDIAN_CHECK_VALUE
}



uint64_t byte_swap(uint64_t value){
#define SHIFT_FROM_MID_1 8
#define SHIFT_FROM_MID_2 24
#define SHIFT_FROM_MID_3 40
#define SHIFT_FROM_MID_4 56
#define BITMASK_BYTE_0 0xff00000000000000
#define BITMASK_BYTE_1 0xff000000000000
#define BITMASK_BYTE_2 0xff0000000000
#define BITMASK_BYTE_3 0xff00000000
#define BITMASK_BYTE_4 0xff000000
#define BITMASK_BYTE_5 0xff0000
#define BITMASK_BYTE_6 0xff00
#define BITMASK_BYTE_7 0xff

	uint64_t swapped;

	swapped =	((value>>SHIFT_FROM_MID_4)&BITMASK_BYTE_7) |
				((value>>SHIFT_FROM_MID_3)&BITMASK_BYTE_6) |
				((value>>SHIFT_FROM_MID_2)&BITMASK_BYTE_5) |
				((value>>SHIFT_FROM_MID_1)&BITMASK_BYTE_4) |
				((value<<SHIFT_FROM_MID_1)&BITMASK_BYTE_3) |
				((value<<SHIFT_FROM_MID_2)&BITMASK_BYTE_2) |
				((value<<SHIFT_FROM_MID_3)&BITMASK_BYTE_1) |
				((value<<SHIFT_FROM_MID_4)&BITMASK_BYTE_0);

	return swapped;

#undef SHIFT_FROM_MID_1
#undef SHIFT_FROM_MID_2
#undef SHIFT_FROM_MID_3
#undef SHIFT_FROM_MID_4
#undef BITMASK_BYTE_0
#undef BITMASK_BYTE_1
#undef BITMASK_BYTE_2
#undef BITMASK_BYTE_3
#undef BITMASK_BYTE_4
#undef BITMASK_BYTE_5
#undef BITMASK_BYTE_6
#undef BITMASK_BYTE_7
}








#undef NO_REMAINDER_C_FUNCTIONS
#undef REMAINDER_C
