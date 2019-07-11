#ifndef OLLERUS_H
#define OLLERUS_H

/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 * Based on Experience from iw (Johannes Berg)
 */

#include <stdbool.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <string.h>
#include <linux/types.h>
#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <stdbool.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/socket.h>
#include <endian.h>
#include <pthread.h>
#include <semaphore.h>
#include <ncurses.h>
#include <time.h>
#include <pcap.h>
#include <signal.h>
#include <malloc.h>
#include <arpa/inet.h>

#include "ollerus_globalsettings.h"
#include "ollerus_base.h"
#include "debug.h"
#include "ieee80211.h"
#include "nl80211.h"
#include "remainder.h"

//#include "head/ollerus_extern_functions.h" //Stands inside every src at it's own




#define CFG_FILE_PATH_MAIN "ollerus.cfg"
#define CFG_FILE_PATH_PREFIX_ADAPTTXPOWER "adapttxpower_"
#define LOG_FILE_PATH_PREFIX_ADAPTTXPOWER "log_adapttxpower_"


#define ETH_ALEN 6
#define THREADS_STD_MAX 3
#define THREADS_CLIENTS_MAX 30
/*
 * Threads for (except the obligatory main...):
 * (References for usage to this Threads are stored in an array.
 * 	Call it something like threads_std)
 * 1. Listening to live Console Commands (x1)
 * 2. Handling the ConfigFile (Read/Write) (x1)
 * 3. Keep the txpower setup on the optimum Level (x1)
 * 		(This Thread instantiates more Threats. It just handles
 * 		 to work with an undefined number of clients (if running in server-mode).
 * 		 So it makes one setting-thread for each connected client and they
 * 		 communicate with their respective clients.
 * 		 It stores references to the threads in a own array.
 * 		 Call it something like threads_adapttxpow)
 */


/* libnl 1.x compatibility code */
/*
#if !defined(CONFIG_LIBNL20) && !defined(CONFIG_LIBNL30)
#define nl_sock nl_handle
#endif
*/

struct nl80211_state {
	struct nl_sock *nl_sock;
	int nl80211_id;
};


enum Width {
	STA_WIDTH_DEFAULT,
	STA_WIDTH_10_MHZ,
	STA_WIDTH_20_MHZ,
	STA_WIDTH_25_MHZ,
	STA_WIDTH_40_MHZ,
	STA_WIDTH_80_MHZ,
	STA_WIDTH_80P80_MHZ,
	STA_WIDTH_160_MHZ,
};
/*
 * StationInfo flags:
 * 00000001 - short GI instead of standard
 * 00000010 - VHT MCS instead of std MCS
 * 00000100
 * 00001000
 * 00010000
 * 00100000 - BSS flag. CTS-Protection
 * 01000000 - BSS flag. Short Preamble
 * 10000000 - BSS flag. Short Slot Time
 */
/* StationInfo flags definition */
#define STA_INF_SHORT_GI    0x01
#define STA_INF_VHT_MCS   0x02
//#define STA_INF_   0x04
//#define STA_INF_             0x08
//#define STA_INF_    0x10
#define STA_BSS_CTS_PROT   0x20
#define STA_BSS_SHORT_PREAMBLE     0x40
#define STA_BSS_SHORT_SLOT_TIME            0x80
/*----------------------------------*/
struct StationInfo {
	int bitrate; // In MBit/s * 10. If you want to Display it think about a nice unit-prefix...
	enum Width width;
	unsigned char MCS; // Modulation Coding Scheme (MCS) index - specified by ieee
	unsigned char VHT_NSS;
	unsigned int RX_Bytes;
	unsigned int RX_Packets;
	unsigned int TX_Bytes;
	unsigned int TX_Packets;
	unsigned char dtim; // dtim Period
	unsigned short beacon; //beacon int
	int SigLvl; //Signal Level in dBm
	char flags;
};
/*
 * WLANConnectionData flags:
 * 00000001 -
 * 00000010 -
 * 00000100
 * 00001000
 * 00010000
 * 00100000 -
 * 01000000 - The Transmission Power is in mW
 * 10000000 - The Transmission Power is in dBm
 */
/* WLANConnectionData flags definition */
//#define WLAN_CON_DATA_ 0x01
//#define WLAN_CON_DATA_ 0x02
//#define WLAN_CON_DATA_ 0x04
//#define WLAN_CON_DATA_ 0x08
//#define WLAN_CON_DATA_ 0x10
//#define WLAN_CON_DATA_ 0x20
#define WLAN_CON_DATA_TXPOWER_MW 0x40
#define WLAN_CON_DATA_TXPOWER_DBM 0x80
/*----------------------------------*/
struct WLANConnectionData {
	char *interfacename;
	char essid[33]; // Space for ESSID-String (32-Bit for String, 1-Bit for End of String [\0])
	unsigned char MAC_ConnectedTo[20]; // The "unsigned" predicate is mighty important
	unsigned int type; //infrastructure type of connection. Index specified by ieee
	unsigned int frequency; // In MHz. If you want to Display it think about a nice unit-prefix...
	unsigned char MAC_Own[20]; // The "unsigned" predicate is mighty important
	signed int owntranspower; // Transmission Power in Unit specified in flags (of your device here, with this established connection)
	struct StationInfo stainf; // Contains the nice things like bitrate, modulation, channel-width.
	unsigned char noise;
	unsigned char flags;
};

struct AttsToAppend {
	struct preparedAttributes *first;
	struct preparedAttributes *current; //in first instance used to point to the last element in chain (to easy be able to append a lot in rapid succession
};
struct preparedAttributes {
	enum nl80211_attrs attype;
	int attpoint; //used as Pointer to the actual Value of the attribute (Holds the Address)
	struct preparedAttributes *next;
};
struct CommandContainer {
	char *identifier;
	enum nl80211_commands cmd;
	struct AttsToAppend *prepatt;
	int nl_msg_flags;
	void *callbackToUse;
	void* callbackargpass; //This is a pointer to the argument, the callback shall get passed
		//Because this could be everything (a single int, a struct...) just store the address
		//of it in this, whereas this is done over a void Pointer.
		//Later on the callback function gets this address inside the
		//CallbackArgPass Structure down under here. There you have to correctly
		//cast it back.
};
struct CallbackArgPass {
	int *err;
	void* ArgPointer;
};







extern struct WLANConnectionData WhatWeWant;

extern int ifIndex;
extern int expectedId;

extern char logatxp;






void ctrl_c();






//Logging
//Level-Values
//Flags
#define LOG_ATXP_BASIC 0x01
//#define LOG_ATXP_ 0x02
//#define LOG_ATXP_ 0x04
//#define LOG_ATXP_ 0x08
//#define LOG_ATXP_ 0x11
//#define LOG_ATXP_ 0x12
//#define LOG_ATXP_ 0x14
//#define LOG_ATXP_ 0x18





//flags to bypass partial functions
#define BYPASS_HANDLELINK_NL80211_CMD_GET_SCAN 0X01
#define BYPASS_HANDLELINK_NL80211_CMD_GET_INTERFACE 0X02
#define BYPASS_HANDLELINK_NL80211_CMD_GET_STATION 0X04
#define BYPASS_HANDLELINK_NL80211_CMD_GET_SURVEY 0X08





#endif /* OLLERUS_H */
