/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */


#include "ollerus_globalsettings.h"
#include "ollerus.h"
#include "ollerus_base.h"
#include "absint.h"
#include "absint_ctrl.h"
#include "absint_ctrl_sdn_ctrl_com.h"

#include "head/ollerus_extern_functions.h"




int absint_ctrl_debug_testsdnctrlcomm(){
	int err;
	printf("Testing Communication with SDN Controller.\n");
	//-------------------------------

	CREATE_PROGRAM_PATH(*args);
	ABSINT_READ_CONFIG_FILE_CTRL;

	//-------------------------------
	char *writepoint;
	char testbody[]="{\"dpid\": [9796758300265], \"switch_to_port\":[6]}";
	char content_length[getDigitCountofInt(sizeof(testbody)-1)+1];
	sprintf(content_length, "%d", strlen(testbody));
	char testpost[(sizeof(SDNCtrlPortTriggerHTTPHead)-1)+
				  (sizeof(content_length)-1)+
				  (sizeof(SDNCtrlPortTriggerHTTPHeadEnd)-1)+
				  (sizeof(testbody)-1)+
				  1];
	memcpy(testpost,SDNCtrlPortTriggerHTTPHead,sizeof(SDNCtrlPortTriggerHTTPHead)-1);
	writepoint=testpost+sizeof(SDNCtrlPortTriggerHTTPHead)-1;
	memcpy(writepoint,content_length,sizeof(content_length)-1);
	writepoint+=sizeof(content_length)-1;
	memcpy(writepoint,SDNCtrlPortTriggerHTTPHeadEnd,sizeof(SDNCtrlPortTriggerHTTPHeadEnd)-1);
	writepoint+=sizeof(SDNCtrlPortTriggerHTTPHeadEnd)-1;
	memcpy(writepoint,testbody,sizeof(testbody)-1);
	testpost[sizeof(testpost)-1]='\0';
	puts("");
	printfc(red,"Sending String (strlen %d | sizeof %d):\n",strlen(testpost),sizeof(testpost));
	printfc(blue,"%s\n",testpost);
	//---------------------------------------
	puts("");
	printfc(red,"Sending String (strlen %d | sizeof %d):\n",strlen(SDNCtrlGetSwitchesHTTPHead),sizeof(SDNCtrlGetSwitchesHTTPHead));
	printfc(blue,"%s\n",SDNCtrlGetSwitchesHTTPHead);
	//---------------------------------------
//					inet_pton(AF_INET,"184.25.47.240",&controlleraddress);
	ABSINT_CTRL_INET_SOCKET_CONTROLLER_COM_DNS_LOOKUP;
	ABSINT_CTRL_INET_SOCKET_SDN_CONTROLLER_COM_CONNECT;
	err = senddetermined(s,testpost,sizeof(testpost)-1);
	struct timespec remainingdelay;
	remainingdelay.tv_sec = 1;
	remainingdelay.tv_nsec = 0;
	do {
		err = nanosleep(&remainingdelay, &remainingdelay);
	} while (err<0);
	int bytes_recvd;
	char recv_buf[256];
	while(1){
		bytes_recvd=recv(s,recv_buf,sizeof(struct AICtrlMsgHeader),MSG_DONTWAIT);
//						printf("Received %d Bytes\n",bytes_recvd);
		if(bytes_recvd<=0)
			goto SOCKET_CLEANED;
	}
	SOCKET_CLEANED:
	;
//					printf("Socket cleaned.\n");
//					printf("Msg sent connected with err %d | ERRNO %d\n",err,errno);
//					err=sendto(s, testpost, sizeof(testpost), 0, &sendto_addr, sizeof(struct sockaddr_storage));
//					printf("Msg sent connectionless with err %d | ERRNO: %d\n",err,errno);
	err = senddetermined(s,SDNCtrlGetSwitchesHTTPHead,sizeof(SDNCtrlGetSwitchesHTTPHead)-1);
	remainingdelay.tv_sec = 1;
	remainingdelay.tv_nsec = 0;
	do {
		err = nanosleep(&remainingdelay, &remainingdelay);
	} while (err<0);
	puts("");
	char *readPoint;
	readPoint=recv_buf;
	memset(recv_buf,0,sizeof(recv_buf));
	int bytes_recvd_compl;
	bytes_recvd_compl=0;
	while(1){
		bytes_recvd=recv(s,readPoint,sizeof(recv_buf),MSG_DONTWAIT);
//						printf("Received %d Bytes\n",bytes_recvd);
		if(bytes_recvd<=0){
			break;
		}else{
			bytes_recvd_compl+=bytes_recvd;
			readPoint+=bytes_recvd;
		}
	}
	//Free from the HTTP Header
	for(readPoint=strstr(recv_buf,"Date:");*readPoint!='\n';readPoint++){}
	readPoint++;
	printfc(red,"||");
	printfc(yellow,"------");
	printfc(green,">>>");
	printf("Answer from the SDN-Controller");
	printfc(green,"<<<");
	printfc(yellow,"------");
	printfc(red,"||");
	printf("\n%s\n",readPoint);
	puts("");
	printfc(red,"||");
	printfc(yellow,"------");
	printfc(green,">>>");
	printf("Answer End");
	printfc(green,"<<<");
	printfc(yellow,"------");
	printfc(red,"||");
	printfc(red,"||");
	printfc(yellow,"------");
	printfc(green,">>>");
	puts("");
	printf("Complete Answer");
	printfc(green,"<<<");
	printfc(yellow,"------");
	printfc(red,"||");
	printf("\n%s\n",recv_buf);
	puts("");
	printfc(red,"||");
	printfc(yellow,"------");
	printfc(green,">>>");
	printf("Answer End");
	printfc(green,"<<<");
	printfc(yellow,"------");
	printfc(red,"||");
//					char sec_buf[]
//					while(bytes_recvd_compl>0){
//
//					}
//					printf("Received %d Bytes complete\n",bytes_recvd_compl);
	close(s);
	return MAIN_ERR_NONE;
}
