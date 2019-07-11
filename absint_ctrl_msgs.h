#ifndef ABSINT_CTRL_MSGS_H
#define ABSINT_CTRL_MSGS_H

/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */



typedef uint16_t msgfreq;
typedef uint16_t msgchan;
typedef uint32_t msgcount;
typedef uint32_t msgswitchid;





/* AITC - AI To Controller
 * CTAI - Controller To AI
 * INTERN - Doesn't go out. Just inside the controller - Like between CLI and Manager (e.g. print connections)
 */
/* The send-msgs can be used in to ways:
 * 1.: Send the value, terminated with '\0', than the device, that should be used
 * 2.: Send only the value, terminated with '\0', if string, or just only the value, if number
 * If no Device is sent inside the message, the AI takes the value for the currently used sending device.
 */
enum AICtrlMsgType{
	AIC_INTERN_DUMP,//To explicitly say: Type doesn't care right now. To tell after command-mux, that there was no hit.
	AIC_AITC_IFACE,
	AIC_AITC_MAC,
	AIC_AITC_DPID,// AIC_AITC_MAC & AIC_AITC_DPID are redundant. They are parsed exactly equal from the Controller. They are both just existent for clarification
	AIC_AITC_WLANSTAT_RSSI,
	AIC_AITC_WLANSTAT_TRAFFICSTAT,
	AIC_AITC_WLANSTAT_COMPLETE,
	AIC_AITC_BITRATE,
	AIC_CTAI_CHAN_CHANGE_SEAMLESS,
	AIC_CTAI_START_ADHOC,
	AIC_CTAI_STOP_ADHOC,
	AIC_CTAI_SET_ADHOC_ESSID,
	AIC_CTAI_SET_ADHOC_FREQ,
	AIC_CTAI_SET_ADHOC_CHAN,
	AIC_CTAI_SET_ADHOC_ESSID_FREQ,
	AIC_CTAI_SET_ADHOC_SECUR_PW,
	AIC_CTAI_SET_IFACE_IP4,
	AIC_CTAI_INQUIRE_IFACE,
	AIC_CTAI_INQUIRE_RSSI,//Sends Device and Freq. Let the AI do one Sniffing Interval
	AIC_CTAI_MONITOR_RSSI,//Sends Device and Freq. The AI goes permanently into Monitor Mode on the delivered Device and frequency. Send a "Wildcard-Frequency" (i.e. 0) to force the AI to circle over all channels in Intervals
	AIC_WLAN_SWITCH_FSM_CTAI_START,
	AIC_WLAN_SWITCH_FSM_CTAI_SWITCH_DONE,
	AIC_WLAN_SWITCH_FSM_CTAI_FINISH,
	AIC_WLAN_SWITCH_FSM_CTAI_STAY_READY,
	AIC_WLAN_SWITCH_FSM_CTAI_STAY_DONE,
	AIC_WLAN_SWITCH_FSM_AITC_READY_TO_SWITCH,
	AIC_WLAN_SWITCH_FSM_AITC_SWITCH_DONE_ACK,
	AIC_WLAN_SWITCH_FSM_AITC_READY_TIMEOUT,
	AIC_WLAN_SWITCH_FSM_AITC_DONE_TIMEOUT,
	AIC_WLAN_SWITCH_FSM_AITC_STAY_READY_ACK,
	AIC_WLAN_SWITCH_FSM_AITC_STAY_DONE_ACK,
	AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_R,
	AIC_WLAN_SWITCH_FSM_AITC_TIMEOUT_FALLBACK_D,
	AIC_WLAN_SWITCH_FSM_AITC_FINISH_ACK,
	AIC_WLAN_SWITCH_FSM_CTSDN_,
	AIC_WLAN_SWITCH_FSM_SDNTC_,
	AIC_INTERN_PRINT_CONNECTIONS,
	AIC_INTERN_PRINT_DPIDS,
	AIC_INTERN_GET_DPIDS,
	AIC_INTERN_AI_DISCONNECT,
	AIC_INTERN_LET_AI_DISCONNECT,
	AIC_INTERN_DO_WLAN_TOPOLOGY_CONNECTIONS,
	AIC_INTERN_WLAN_SWITCH_FSM_START_CTRL_THREAD,
	AIC_INTERNAI_TIMEOUT,//A general Timeout.
	AIC_INTERNAI_TIMEOUT_R,//Timeout in ReadyToSwitch State
	AIC_INTERNAI_TIMEOUT_D,//Timeout in SwitchDone State
	AIC_INTERNAI_TIMEOUT_2nd,//The second timeout, in a Timeout State
};
struct AICtrlMsgHeader{
	unsigned int msgsize;
	enum AICtrlMsgType type;
};
struct AICtrlInterThreadManageMsgHeader{
	unsigned int msgsize;
	char flags;
	enum AICtrlMsgType type;
	unsigned int idx;//The Index of the Thread(Connected AI) from which the msg came
			//Or which gets the next sending msg
	char *msg;
};
struct AICtrlInterThreadSendMsgHeader{
	unsigned int msgsize;
	char flags;
	enum AICtrlMsgType type;
};
struct AICtrlInterThreadSDNSendMsgHeader{
	unsigned int msgsize;
	char flags;
	enum AICtrlMsgType type;
};
struct AICtrlInterThreadFSMMsgHeader{
	unsigned int msgsize;
	char flags;
	enum AICtrlMsgType type;
	unsigned int idx;//The Index of the Thread(Connected AI) from which the msg came
			//Or which gets the next sending msg
	char *msg;
};
struct AICtrlFSMSocketMsgHeader{
	int thread_index;
	unsigned int msgsize;
	enum AICtrlMsgType type;
};
//The Flags:
#define FLAG_ABSINT_CTRL_INTER_MSG_NEW 0x01//If Set: Message is new and Main hasn't read out yet
//#define FLAG_ABSINT_CTRL_INTER_MSG_ 0x02
//#define FLAG_ABSINT_CTRL_INTER_MSG_ 0x04
//#define FLAG_ABSINT_CTRL_INTER_MSG_ 0x08
//#define FLAG_ABSINT_CTRL_INTER_MSG_ 0x10
#define FLAG_ABSINT_CTRL_INTER_MSG_SDNCTRL 0x20//IF set: Msg comes from the SDN-Ctrl (Listening Thread)
#define FLAG_ABSINT_CTRL_INTER_MSG_AI 0x40//IF set: The message comes from an AI-Thread
#define FLAG_ABSINT_CTRL_INTER_MSG_CLI 0x80//If set: The message comes from the CLI-Thread

struct AICtrlMsgIfaceHeader{

};
struct AICtrlMsgRSSIHeader{
	msgfreq freq;
	msgcount count;
};
struct AICtrlMsgTrafficstat{
	msgfreq currentfreq;
	msgfreq freq;
	double trafficstat;
};




struct AICtrlThreadArgPassing {
	int solid_s; //handy for putting socket descr. to thread
	struct AI_ChanSwitch_Couple **AICouple;
	sem_t *sem_all;
	sem_t *sem_sendwait;
	int thread_index; //to let the thread know on which index in the all holding array it stands
	pthread_t *all_threads_array; //Pass the array with references to all active threads. Could be handy somewhere...
	struct AICtrlInterThreadManageMsgHeader *manmsg;
	struct AICtrlInterThreadSendMsgHeader ***sendmsg_all;//A Pointer to an Array of Pointers
};
struct AICtrlThreadListenArgPassing {
	int solid_s; //handy for putting socket descr. to thread
	struct AI_ChanSwitch_Couple **AICouple;
	sem_t *sem_all;
	int thread_index;
	int *bytes_recvd;
	struct AICtrlInterThreadManageMsgHeader *manmsg;
};
struct AICtrlManageThreadArgPassing {
	pthread_t *all_threads_array;
	sem_t *sem_all;
	sem_t *sem_sendwait;
	struct AICtrlInterThreadManageMsgHeader *manmsg;
	struct AICtrlInterThreadSDNSendMsgHeader **sdnsendmsg;
	struct AICtrlInterThreadSendMsgHeader ***sendmsg_all;//A Pointer to an Array of Pointers
	struct AICtrlConnectedAI **AIConnections;//A Pointer to an Array of structures
	struct AI_ChanSwitch_Couple **AICouples;
};
struct AICtrlSDNCtrlComThreadArgPassing {
	pthread_t *all_threads_array;//Mainly to start the listen thread inside
	sem_t *sem_all;
	sem_t *sem_sendwait;
	struct AICtrlInterThreadManageMsgHeader *manmsg;
	struct AICtrlInterThreadSDNSendMsgHeader **sdnsendmsg;
	struct AICtrlInterThreadSendMsgHeader ***sendmsg_all;//A Pointer to an Array of Pointers
};
struct AICtrlSDNCtrlComThreadListenArgPassing {
	int solid_s;
	sem_t *sem_all;
	sem_t *sem_sendwait;
	struct AICtrlInterThreadManageMsgHeader *manmsg;
	struct AICtrlInterThreadSendMsgHeader ***sendmsg_all;//A Pointer to an Array of Pointers
};
struct AICtrlFSMThreadArgPassing {
	msgfreq newfreq;
	sem_t *sem_all;
	sem_t *sem_sendwait;
	struct AICtrlInterThreadSendMsgHeader ***sendmsg_all;//A Pointer to an Array of Pointers
	struct AI_ChanSwitch_Couple *AICouple;
	int socket_fsmside;
};








#endif /* ABSINT_CTRL_MSGS_H */
