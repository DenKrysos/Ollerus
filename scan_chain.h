#ifndef SCAN_CHAIN_H
#define SCAN_CHAIN_H
/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#include <stdbool.h>


/*
 * ChainElement flags:
 * 00000001 - No BSS Attribute was delivered or Program couldn't parse delivered
 * 00000010 - Set if the Signal Level came in unspecified unit [scaled to 0..100 (u8)] instead of nice mBm [(100 * dBm) (s32)]
 * 00000100 - No SSID was in received message contained
 * 00001000 - Delivered SSID didn't properly fit in Payload
 * 00010000 - Capability Switch, depending on frequency
 * 00100000 - Connected with this Net
 * 01000000 - Authenticated with this
 * 10000000 - IBSS Joined
 */
/* ChainElement flags definition */
#define SCAN_CHAIN_ELE_NO_BSS_INF 0x01
#define SCAN_CHAIN_ELE_SIGNAL_LEVEL 0x02
#define SCAN_CHAIN_ELE_NO_SSID 0x04
#define SCAN_CHAIN_ELE_SSID_TO_LONG 0x08
#define SCAN_CHAIN_ELE_CAPA_DMG 0x10
#define SCAN_CHAIN_ELE_ASSOCIATED 0x20
#define SCAN_CHAIN_ELE_AUTHENTICATED 0x40
#define SCAN_CHAIN_ELE_IBSS_JOINED 0x80
/*----------------------------------*/
struct chain_element {
	char ssid[33];
	unsigned char mac[20];
	unsigned long long tsf;
	unsigned int freq;
	unsigned short capability;
	unsigned short beaconinterval;
	signed int signallevel;
	unsigned int lastseen;
	unsigned char flags;
	struct chain_test *next;
};
struct chain_start {
	unsigned int count;
	unsigned int currentcnt;
	struct chain_element *start;
	struct chain_element *current;
	int (*append) ();
};


/*
 * flags for WLan Capability Multiplex
 */
#define WLAN_CAPABILITY_ESS		(1<<0)
#define WLAN_CAPABILITY_IBSS		(1<<1)
#define WLAN_CAPABILITY_CF_POLLABLE	(1<<2)
#define WLAN_CAPABILITY_CF_POLL_REQUEST	(1<<3)
#define WLAN_CAPABILITY_PRIVACY		(1<<4)
#define WLAN_CAPABILITY_SHORT_PREAMBLE	(1<<5)
#define WLAN_CAPABILITY_PBCC		(1<<6)
#define WLAN_CAPABILITY_CHANNEL_AGILITY	(1<<7)
#define WLAN_CAPABILITY_SPECTRUM_MGMT	(1<<8)
#define WLAN_CAPABILITY_QOS		(1<<9)
#define WLAN_CAPABILITY_SHORT_SLOT_TIME	(1<<10)
#define WLAN_CAPABILITY_APSD		(1<<11)
#define WLAN_CAPABILITY_RADIO_MEASURE	(1<<12)
#define WLAN_CAPABILITY_DSSS_OFDM	(1<<13)
#define WLAN_CAPABILITY_DEL_BACK	(1<<14)
#define WLAN_CAPABILITY_IMM_BACK	(1<<15)
/* DMG (60gHz) 802.11ad */
/* type - bits 0..1 */
#define WLAN_CAPABILITY_DMG_TYPE_MASK		(3<<0)

#define WLAN_CAPABILITY_DMG_TYPE_IBSS		(1<<0) /* Tx by: STA */
#define WLAN_CAPABILITY_DMG_TYPE_PBSS		(2<<0) /* Tx by: PCP */
#define WLAN_CAPABILITY_DMG_TYPE_AP		(3<<0) /* Tx by: AP */

#define WLAN_CAPABILITY_DMG_CBAP_ONLY		(1<<2)
#define WLAN_CAPABILITY_DMG_CBAP_SOURCE		(1<<3)
#define WLAN_CAPABILITY_DMG_PRIVACY		(1<<4)
#define WLAN_CAPABILITY_DMG_ECPAC		(1<<5)

#define WLAN_CAPABILITY_DMG_SPECTRUM_MGMT	(1<<8)
#define WLAN_CAPABILITY_DMG_RADIO_MEASURE	(1<<12)




extern struct chain_start *chainstart;

#endif /* SCAN_CHAIN_H */
