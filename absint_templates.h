/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */


#include "absint.h"





/*Look through this Template: It does things in this order:
 * 1. Defines struct and detects all existing network Interfaces on this device
 * 2. Defines struct and selects the WLAN Interfaces
 * 3. Stops a running ad-hoc daemon (if any) before -
 * 4. Starting WLAN Packet Sniffer to detect other devices on specified WLAN Channel
 * 5. Starts Ad-hoc Network
 * 6. Changes frequency of (own) Ad-hoc affiliation
 * 7. Changes ESSID of (own) Ad-hoc affiliation
 */
#define ABSINT_TEMPLATE_START_WIFI_SNIFFER struct AbsintInterfaces ifcollect; \
	ifcollect.neighbours.dnstart=NULL; \
	uintptr_t ifps; \
	err = getInterfacesAndAdresses(&(ifcollect.ifc),&ifps,&(ifcollect.ifacestart),&(ifcollect.ifmacstart),&(ifcollect.ifaddrstart)); \
	 \
	struct AbsintInterfacesWLAN wifcollect; \
	wifcollect.wlanc=0; \
	wifcollect.wlanidx=NULL; \
	select_AI_wlan_dev(&wifcollect,&ifcollect,sktctr,cmd); \
	char *dev; \
	dev=(ifcollect.ifacestart)[(wifcollect.wlanidx)[0]]; \
	struct wlansniff_chain_start *wlanp; \
	wlanp=NULL; \
	enum nl80211_band band; \
	int chan,freq; \
	chan=1; \
	band = chan <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ; \
	freq = ieee80211_channel_to_frequency(chan, band); \
	/*Better stop a running wpa_supplicant daemon (if any) before Wifi-Sniffing on this port.*/ \
	stop_adhoc_wpa_supplicant(dev); \
	err=wifi_package_parse(dev,freq,&wlanp,sktctr,cmd); \
	switch(err){ \
	case ERR_WLAN_SNIFF_BAD_DEVICE: \
		printfc(YELLOW,"WARNING: "); \
		printf("Raw-WLAN-Parser was called on something not a WLAN-Device.\n\tMaybe this messed up the Interface Configuration.\n"); \
		break; \
	} \
	err=printf_sniffed_wlan_packets(wlanp); \
	 \
	err=start_adhoc_wpa_supplicant(dev); \
	 \
	chan=5; \
	band = chan <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ; \
	freq = ieee80211_channel_to_frequency(chan, band); \
	set_adhoc_freq_wpa_supplicant(dev,freq,check_running_wpa_supplicant(dev)); \
	 \
	set_adhoc_essid_wpa_supplicant(dev,"AbsintAdhoc1",check_running_wpa_supplicant(dev));

