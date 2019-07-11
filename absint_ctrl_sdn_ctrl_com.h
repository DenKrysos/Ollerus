#ifndef ABSINT_CTRL_SDN_CTRL_COMM_H
#define ABSINT_CTRL_SDN_CTRL_COMM_H

/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */

#include "ollerus_globalsettings.h"
//#include "absint_ctrl_msgs.h"




/* either
 * local:		0
 * external:	1
 * auto:		2
 * (every other Value equals to external
*/
#define SDN_CTRL_IS_LOCAL_OR_EXTERN 1

#if (SDN_CTRL_IS_LOCAL_OR_EXTERN == 0)
#elif (SDN_CTRL_IS_LOCAL_OR_EXTERN == 1)
#else
	#undef SDN_CTRL_IS_LOCAL_OR_EXTERN
	#define SDN_CTRL_IS_LOCAL_OR_EXTERN 1
#endif



#if (SDN_CTRL_IS_LOCAL_OR_EXTERN == 0)
	#define SDN_CTRL_ADDR INADDR_LOOPBACK
#elif (SDN_CTRL_IS_LOCAL_OR_EXTERN == 1)
	#define SDN_CTRL_ADDR controlleraddress
#endif








#if (SDN_CTRL_IS_LOCAL_OR_EXTERN == 0)

	#define ABSINT_CTRL_INET_SOCKET_CONTROLLER_COM_DNS_LOOKUP

#elif (SDN_CTRL_IS_LOCAL_OR_EXTERN == 1)

	#define ABSINT_CTRL_INET_SOCKET_CONTROLLER_COM_DNS_LOOKUP ABSINT_INET_SOCKET_CONTROLLER_COM_DNS_LOOKUP

#endif





#define ABSINT_CTRL_INET_SOCKET_SDN_CONTROLLER_COM_CONNECT \
					int s; \
					 \
					{ /* Scope */ \
					struct sockaddr_in myaddr_in; \
					struct sockaddr_in peeraddr_in; \
					/*struct in_addr sdnctrlIP4; \
					memcpy(&sdnctrlIP4,&(SDN_CTRL_ADDR),sizeof(struct in_addr));*/ \
					struct in_addr sdnctrlIP4=SDN_CTRL_ADDR; \
					 \
					unsigned int addrlen; \
					ANSICOLORSET(ANSI_COLOR_BLUE); \
					printf("-->SDN-Ctrl-Com:"); \
					ANSICOLORRESET; \
					printf(" Setting up Connection to SDN Controller...\n"); \
					memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in)); \
					memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in)); \
					peeraddr_in.sin_family = AF_INET; \
					memcpy(&(peeraddr_in.sin_addr),&sdnctrlIP4,sizeof(struct in_addr)); \
					char printaddrv4[INET_ADDRSTRLEN]; \
					inet_ntop(AF_INET, &(peeraddr_in.sin_addr), printaddrv4, INET_ADDRSTRLEN); \
					ANSICOLORSET(ANSI_COLOR_BLUE); \
					printf("\t-->SDN-Ctrl-Com:"); \
					ANSICOLORRESET; \
					printf(" ...Connecting to IP Address (IPv4): %s\n", printaddrv4); \
					peeraddr_in.sin_port = htons(sdnctrlport); \
					ANSICOLORSET(ANSI_COLOR_BLUE); \
					printf("\t-->SDN-Ctrl-Com:"); \
					ANSICOLORRESET; \
					printf(" ...Port number: %d\n", ntohs(peeraddr_in.sin_port)); \
					ANSICOLORSET(ANSI_COLOR_BLUE); \
					printf("-->SDN-Ctrl-Com:"); \
					ANSICOLORRESET; \
					printf(" Creating Socket...\n"); \
					addrlen=sizeof(struct sockaddr_in); \
					ABSINT_INET_SOCKET_CONTROLLER_COM_CONNECT_AGAIN: \
					s = socket (AF_INET, SOCK_STREAM, 0); \
					err=0;/*Say: At the moment, the socket is a Stream Socket*/ \
					if (s == -1) { \
						ANSICOLORSET(ANSI_COLOR_BLUE); \
						fprintf(stderr,"-->SDN-Ctrl-Com:"); \
						ANSICOLORRESET; \
						fprintf(stderr," unable to create socket\n"); \
						exit(1); \
					} \
					ANSICOLORSET(ANSI_COLOR_BLUE); \
					printf("\t-->SDN-Ctrl-Com:"); \
					ANSICOLORRESET; \
					printf(" ...Socket created!\n"); \
					/* \
					char tempbuf[IFNAMSIZ]; \
					unsigned int tempsize; \
					memset(tempbuf,0,sizeof(tempbuf)); \
					tempsize=IFNAMSIZ; \
					err=getsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, tempbuf, &tempsize); \
					printf("Socket Device: %s | tempsize: %d | Err: %d |  ERRNO: %d\n",tempbuf,tempsize,err,errno); \
					err=setsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, "eth1", 4); \
					printf("Socket Option Error: %d\n",err); \
					tempsize=IFNAMSIZ; \
					err=getsockopt(s, SOL_SOCKET, SO_BINDTODEVICE, tempbuf, &tempsize); \
					printf("Socket Device: %s | tempsize: %d | Err: %d |  ERRNO: %d\n",tempbuf,tempsize,err,errno); \
					*/ \
					ANSICOLORSET(ANSI_COLOR_BLUE); \
					printf("\t-->SDN-Ctrl-Com:"); \
					ANSICOLORRESET; \
					printf(" Now connecting...\n"); \
					if (connect(s, &peeraddr_in, addrlen) ==-1) { \
						ANSICOLORSET(ANSI_COLOR_BLUE); \
						printf("\n-->SDN-Ctrl-Com:"); \
						ANSICOLORRESET; \
						if(errno==ECONNREFUSED){ \
							/*switch(err){*/ \
							/*case 0:*/ \
								printf(" ERROR: Couldn't connect to specified Peer Address with "); \
								printfc(yellow,"STREAM"); \
								printf(" Socket. | ERRNO: %d\n",errno); \
								printf("\tTrying again with DATAGRAM\n"); \
								close(s); \
								s = socket (AF_INET, SOCK_DGRAM, 0); \
								err=1;/*Say: At the moment, the socket is a Stream Socket*/ \
								if (s == -1) { \
									ANSICOLORSET(ANSI_COLOR_BLUE); \
									fprintf(stderr,"-->SDN-Ctrl-Com:"); \
									ANSICOLORRESET; \
									fprintf(stderr," unable to re-create socket\n"); \
									exit(1); \
								} \
								if (connect(s, &peeraddr_in, addrlen) ==-1) { \
									printf(" ERROR: Couldn't connect with "); \
									printfc(yellow,"DATAGRAM"); \
									printf(" either. | ERRNO: %d\n",errno); \
									close(s); \
								}else{ \
									goto DGRAM_CONNECTION_ESTABLISHED; \
								} \
							/*	break; \
							case 1: \
								break; \
							default: \
								break; \
							}*/ \
						} \
						printf(" ERROR: Couldn't connect to specified Peer Address. | ERRNO: %d\n",errno); \
						ANSICOLORSET(ANSI_COLOR_BLUE); \
						printf("\t-->SDN-Ctrl-Com:"); \
						ANSICOLORRESET; \
						printf(" Hence waiting a bit and try again.\n"); \
						/*err=NETWORK_NO_CONNECTION;*/ \
						struct timespec remainingdelay; \
						remainingdelay.tv_sec = 1; \
						remainingdelay.tv_nsec = 0; \
						do { \
							err = nanosleep(&remainingdelay, &remainingdelay); \
						} while (err<0); \
						goto ABSINT_INET_SOCKET_CONTROLLER_COM_CONNECT_AGAIN; \
					} \
					DGRAM_CONNECTION_ESTABLISHED: \
					 \
					if (getsockname(s, &myaddr_in, &addrlen) == -1) { \
						ANSICOLORSET(ANSI_COLOR_BLUE); \
						fprintf(stderr,"\t-->SDN-Ctrl-Com:"); \
						ANSICOLORRESET; \
						fprintf(stderr, " unable to read socket address\n"); \
						exit(1); \
					} \
					ANSICOLORSET(ANSI_COLOR_BLUE); \
					printf("-->SDN-Ctrl-Com:"); \
					ANSICOLORRESET; \
					printf(" Connected with a "); \
					switch(err){ \
					case 0: \
						printfc(yellow,"STREAM "); \
						break; \
					case 1: \
					printfc(yellow,"DATAGRAM "); \
						break; \
					default: \
						break; \
					} \
					printf("Socket\n"); \
					} /*Close Scope*/ \




//------------------------------------------------
/* The RESTful Interface Stuff
 * like the HTTP Headers and Creation Makros
 */
//------------------------------------------------

// HTTP Header
#define SDNCtrlPortTriggerHTTPHead "POST /handover/trigger HTTP/1.1\r\nContent-Length: "
#define SDNCtrlPortTriggerHTTPHeadEnd "\r\nContent-Type: text/xml; charset=\"UTF-8\"\r\n\r\n"

#define SDNCtrlGetSwitchesHTTPHead "GET /cocos/switches HTTP/1.1\r\n\r\n"

// Structs
struct DPID_Collector{
	int count;
	uint64_t *dpid;//Allocate Space for a macro behind this.
};

// Makros


//------------------------------------------------
/* End of the RESTful Interface Stuff */
//------------------------------------------------





#define DPID_REFRESH \
	sem_wait(sem_sdnsend_wr); \
	sdnmsghp=malloc(sizeof(struct AICtrlInterThreadSDNSendMsgHeader)); \
	sdnmsghp->type=AIC_INTERN_GET_DPIDS; \
	sdnmsghp->msgsize; \
	sem_post(sem_sdnsend_rd);


#define DPID_REFRESH_AND_WAIT DPID_REFRESH \
	WAITNS(0,MS_TO_NS(500))







#endif /* ABSINT_CTRL_SDN_CTRL_COMM_H */
