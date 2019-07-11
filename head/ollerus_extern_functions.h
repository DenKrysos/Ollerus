#ifndef OLLERUS_EXTERN_FUNCTIONS_H
#define OLLERUS_EXTERN_FUNCTIONS_H

/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */


#include "function_creator_funcname.h"







#ifndef NO_OLLERUS_C_FUNCTIONS
extern int nl80211_init_socket(struct nl80211_state *skt);
extern void nl80211_cleanup_socket(struct nl80211_state *state);
#endif






#ifndef NO_REMAINDER_C_FUNCTIONS
extern int senddetermined (int socket, char *msg, int msglen);
extern void printMAC_fromStr (unsigned char *MAC, int length);
extern void printMAC (unsigned char *MAC, int len,...);
extern CREATE_PRINT_DAYTIME_FUNCNAME(,);
extern CREATE_PRINT_DAYTIME_FUNCNAME(_file,FILE *file);
//The Buffer needs at least 31 Bytes (the Null-Terminating-Character included)
extern CREATE_PRINT_DAYTIME_FUNCNAME(_buffer,char *buffer);
extern int GetRealDayTime(struct timespec *ts, struct tm *tim);
extern time_t tmToSeconds(struct tm *tim);
extern int ieee80211_channel_to_frequency(int chan, enum nl80211_band band);
extern int ieee80211_frequency_to_channel(int freq);
extern int getDigitCountofInt(int n);
extern char check_system_endianess(char *endianess);
extern uint64_t byte_swap(uint64_t value);
#endif




#ifndef NO_ABSINT_NETF_C_FUNCTIONS
extern char check_running_wpa_supplicant(char *dev);
extern int start_adhoc_wpa_supplicant(char *dev,char running);
extern void stop_adhoc_wpa_supplicant(char *dev,char running);
extern int set_adhoc_essid_wpa_supplicant(char *dev,char *ssid,char running);
extern int set_adhoc_freq_wpa_supplicant(char *dev,int freq,char running);
extern int printWLANInterfaces(struct AbsintInterfacesWLAN *wifcollect,struct AbsintInterfaces *ifcollect);
extern int select_AI_wlan_dev(struct AbsintInterfacesWLAN *wifcollect,struct AbsintInterfaces *ifcollect,struct nl80211_state *sktctr,struct CommandContainer *cmd);
extern int netlinkAddressInterfaceVorlage(int argc, char **argstart);
extern int printInterfacesAndAdresses(int argc, char **argstart);
extern int getInterfacesAndAdresses(unsigned int *ifc,uintptr_t *ifps,char ***ifacestart,char ***ifmacstart,char ***ifaddrstart);
extern int getConfiguredInterfacesAndAdresses(unsigned int *ifc,uintptr_t *ifps,char ***ifacestart,char ***ifmacstart,char ***ifaddrstart);
extern int populateARP(unsigned int ifc,char **ifaddrstart);
extern int printNeighbours(struct DiscoveredNeighbours *neighbours);
extern int getNeighboursFromARP(struct DiscoveredNeighbours *neighbours);
extern int getPortPeer(struct AbsintInterfaces *ifcollect);
extern char check_if_dev_exists(char *dev,struct AbsintInterfaces *ifcollect);
extern int get_packeterrorrateTX(char *dev,uint16_t *packerrrate);
extern int get_freq_ssid(char *dev,int *freq, char *ssid);
#endif






#ifndef NO_ABSINT_CTRL_C_FUNCTIONS
extern int absint_controller(int argc, char **argstart,struct nl80211_state *sktctr,struct CommandContainer *cmd);
#endif




#ifndef NO_ABSINT_CTRL_ADDITIONAL_FUNCTIONS_C_FUNCTIONS
extern int print_rssi_of_ai_connections(int idx,pthread_t *all_threads,struct AICtrlConnectedAI **AIConnections);
extern int print_active_ai_connections(pthread_t *all_threads,struct AICtrlConnectedAI **AIConnections,struct AI_ChanSwitch_Couple **AICouples);
extern char dpid_is_present(uint64_t mac,struct DPID_Collector *DPIDs);
extern char refresh_dpids(struct DPID_Collector *DPIDs);
extern int establish_wlan_topology_connections(struct AI_ChanSwitch_Couple **UnestablishedAIs,pthread_t *all_threads, int *index1, int *index2);
extern int analyse_wlanstatistics(int *newfreq, int currentfreq, double *stat, int statsize);
#endif





#ifndef NO_ABSINT_ADDITIONAL_FUNCTIONS_C_FUNCTIONS
extern int MPTCPInitCap();
extern int absint_establish(char **argstart,char hardOrNot);
extern int absint_cut(char **argstart,char hardOrNot);
extern int absint_useports_disjunkt(int devc,char **devstart,int ifc,char **ifacestart);
extern int absint_useports(int devc,char **devstart,int ifc,char **ifacestart);
extern void *absint_com_AI_sender(void* arg);
extern void *absint_com_AI_server(void* arg);
extern void *absint_com_AI_client(void* arg);
extern int secure_device_is_up(char *dev);
#endif




#ifndef NO_WLAN_SNIFF_C_FUNCTIONS
extern int printf_sniffed_wlan_packets(struct wlansniff_chain_start *wlanp);
extern int wifi_package_parse(char *dev,int freq,struct wlansniff_chain_start **wlanp,double timeToMonitor,struct wlansniff_pack_stat *pack_stat);
extern struct sigkill_wlanmon_struct sigkill_wlanmon_stuff;
#endif







#ifndef NO_IOCTL_SURVIVAL_C_FUNCTIONS
extern int get_access_point_local_ip (struct sockaddr_in *dest);
extern int ioctl_get_txpower (int *txpowerdest);
#endif







#ifndef NO_ABSINT_CTRL_CLI_LIVE_C_FUNCTIONS
extern void *absint_ctrl_cli (void* arg);
#endif







#ifndef NO_GETREALTIME_C_FUNCTIONS
#define getRealTime() getRealTime_double()
extern double getRealTime_double();
extern char getRealTime_nanoseconds(struct timespec *ts);
extern char getRealTime_microseconds(struct timeval *tv);
extern uint16_t getRealTime_nanoseconds_clock(struct timespec *ts, clockid_t clockid);
extern uint16_t getRealTime_microseconds_clock(struct timeval *tv, clockid_t clockid);
extern double getRealTime_double_clock(clockid_t clockid);
#endif








#endif /* OLLERUS_EXTERN_FUNCTIONS_H */
