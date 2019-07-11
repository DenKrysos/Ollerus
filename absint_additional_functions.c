/*
 * Authored by
 * Dennis Krummacker (25.06.15-)
 */


#define NO_ABSINT_ADDITIONAL_FUNCTIONS_C_FUNCTIONS


#include "ollerus_globalsettings.h"
#include "ollerus.h"
#include "absint.h"
#include "remainder_extended.h"

#include "head/ollerus_extern_functions.h"


/* TODO:
 * WICHTIG
 * Beim Establish und Cut und useports die Routing-Informationen einstellen.
 * <-> Erst noch: Angepasstes Initial-Setup
 * -> Dann wirds kompliziert. Muss genau überlegt werden, wie mit "ip route" und "ip rule" umgegangen wird
 * --> Assumption: Die zu verwaltenden Ports liegen ja alle zwischen den beiden AI.
 * --> Diese Ports sollten daher demselben Subnetz zugewiesen sein.
 *
 * -> Ablauf nun so:
 * Alle Subnetze ermitteln: Check! Implizit durch die Device-IP4-Addressen
 * Dann alle Geräte in den Subnetzen anpingen: Entweder per Broadcast Ping oder durch Iteration.
 * --> Danach sind die ARP Tables populated (Neighbour-Tabellen sind gefüllt)
 * Nun lässt sich der ARP-Neighbour Kram auslesen.
 *
 */


//The Function absint() is like the main() for the AbstractInterface
//It does the commandline Multiplex and calls the corresponding other Function
//for the Operations. In this Way also the "non-stop operation" ("Dauerbetrieb") of the AI is able to use
//the Subfunctions declared for Operations like "add Interface to MPTCP" or "deplete Interface from MPTCP".



/* TODO:
 * Zu Beachten:
 * You should run ip route flush cache to flush out the routing cache after inserting rules (with ip rule).
 */
int MPTCPInitCap(){
	int err;err=0;

	return err;
}



int absint_establish(char **argstart,char hardOrNot){
	//SEE: In the dereferenced argstart we have the requested Device
	//REMARK: On Connection Establishment first bring up device from down-state (if it is set down)
	//			Then enable multipath capability
	int err=0;
	char *dev=*argstart;

	int commlength2=strlen("ip link set dev  up")+strlen(dev)+1;
	char command2[commlength2];

	switch(hardOrNot){
	case 0:
		break;
	case 1:
		printf("Bring up Device %s from eventual Down-State\n",dev);
		//Do Stuff for "ip link set dev <interface> up"
		//(In other cases just do nothing...)
		snprintf(command2,sizeof(command2),"ip link set dev %s up",dev);
		system(command2);
		break;
	default:
		break;
	}

	int commlength=strlen("ip link set dev  multipath on")+strlen(dev)+1;
	char command[commlength];
	snprintf(command,sizeof(command),"ip link set dev %s multipath on",dev);
	printf("Set up Device %s for Multipath TCP\n",dev);
	system(command);

	return err;
}



int absint_cut(char **argstart,char hardOrNot){
	//SEE: In the dereferenced argstart we have the requested Device
	//REMARK: On Connection Abortion first disable multipath capability
	//			Then bring device to down-state (if requested)
	int err=0;
	char *dev=*argstart;
	int commlength=strlen("ip link set dev  multipath off")+strlen(dev)+1;
	char command[commlength];
	snprintf(command,sizeof(command),"ip link set dev %s multipath off",dev);
	printf("Cut down Device %s from Multipath TCP\n",dev);
	system(command);

	int commlength2=strlen("ip link set dev  down")+strlen(dev)+1;
	char command2[commlength2];

	switch(hardOrNot){
	case 0:
		break;
	case 1:
		printf("Bring down Device %s on Down-State (no further Use of Port possible until Bringing Up again...)\n",dev);
		//Do Stuff for "ip link set dev <interface> down"
		//(In other cases just do nothing...)
		snprintf(command2,sizeof(command2),"ip link set dev %s down",dev);
		system(command2);
		break;
	default:
		break;
	}
	return err;
}



/*
 * BEDENKEN:
 * Unterschied der disjunkten zur "normalen" Version
 * Die disjunkte Version setzt weniger Ports auf multipath, nämlich nur jene, die verwendet werden sollen.
 * Das sorgt nicht immer für einen unterbrechungsfreien Übergang, wenn nämlich keiner der "alten" Ports
 * zwischenzeitlich auf multipath steht.
 */
int absint_useports_disjunkt(int devc,char **devstart,int ifc,char **ifacestart){
	//SEE: In the dereferenced argstart we have the first delivered Port
	//There could be an arbitrary Number of Ports. The number comes with argc
	/*
	 * Procedure:
	 * Loop over the Array of all available Interfaces (after function getInterfacesAndAdresses)
	 * Compare each entry with the given arguments
	 * Match: Set up everything (maybe from hardware down-state and for multipath)(use function absint_establish)
	 * Not Match: remember Interface for second operation
	 * After Set up all "to-Use-Ports"
	 * loop over the remembered ports and cut them down with the function absint_cut
	 */

	int err=0;
	int i,j;
	char setit;//Control the Call of the absint_establish function over this variable
			//instead of immediate call on a match, because theoretically the same device could be
			//given multiple times. Repeated Call of the function with the same device wouldn't be nice
			//(See the Loop down below)
	char notuse[devc];
	memset(notuse,0,sizeof(notuse));

	if(devc>ifc){
		printf("WARNING: You want me to use more Interfaces, than i have o.O\nI only own %d Devices, but you gave me %d to use -.-\nI'll try with everyone, maybe you gave me duplicated. The ones you gave me, but do not exit won't have any effect.\n",ifc,devc);
	}

	for(i=0;i<ifc;i++){
		for(j=0;j<devc;j++){
			if(strcmp(*devstart[j],*ifacestart[i])==0){
				setit=1;
			}
		}
		if(setit==1){
			err=absint_establish(ifacestart+i,0);
			setit=0;
		}else{
			notuse[i]=1;
		}
	}

	for(i=0;i<ifc;i++){
		if(notuse[i]==1){
			err=absint_cut(ifacestart+i,0);
		}
	}

	return err;
}

int absint_useports(int devc,char **devstart,int ifc,char **ifacestart){
	//SEE: In the dereferenced argstart we have the first delivered Port
	//There could be an arbitrary Number of Ports. The number comes with argc
	/*
	 * Procedure:
	 * Loop over the Array of all availabel Interfaces (after function getInterfacesAndAdresses)
	 * and set up everyone for multipath during this.
	 * while Compare each entry with the given arguments
	 * Match: Additional bring up from hardware down-state to secure, that Device isn't switched off
	 * Not Match: remember Interface for second operation
	 * After Set up all "to-Use-Ports"
	 * loop over the remembered ports and cut them down with the function absint_cut
	 */

	int err=0;
	int i,j;
	char setit;//Control the Call of the absint_establish function with this variable
			//instead of immediate call on a match, because theoretically the same device could be
			//given multiple times. Repeated Call of the function with the same device wouldn't be nice
			//(See the Loop down below)
	char notuse[devc];
	memset(notuse,0,sizeof(notuse));

	if(devc>ifc){
		printf("WARNING: You want me to use more Interfaces, than i have o.O\nI only own %d Devices, but you gave me %d to use -.-\nI'll try with everyone, maybe you gave me duplicated. The ones you gave me, but do not exit won't have any effect.\n",ifc,devc);
	}

	for(i=0;i<ifc;i++){
		for(j=0;j<devc;j++){
			if(strcmp(*devstart[j],*ifacestart[i])==0){
				setit=1;
			}
		}
		if(setit==1){
			err=absint_establish(ifacestart+i,1);
			setit=0;
		}else{
			err=absint_establish(ifacestart+i,0);
			notuse[i]=1;
		}
	}

	//Set the Routing-Information


	//Um sicher zu gehen warten wir kurz, damit die multiplen Data Flows auch erzeugt wurden
	//bevor die "alten" beendet werden.
	int timeerr;
	struct timespec remainingdelay;
	remainingdelay.tv_sec = 0;
	remainingdelay.tv_nsec = 1000000;
	do {
		timeerr = nanosleep(&remainingdelay, &remainingdelay);
	} while (timeerr<0);

	for(i=0;i<ifc;i++){
		if(notuse[i]==1){
			err=absint_cut(ifacestart+i,0);
		}
	}

	return err;
}
/* Eventuell um eine Nachricht erweitern:
 * Die AI schicken sich ihre Port<->IP4 Address-Paare. Jedes AI kann damit ermitteln welcher eigene Port
 * mit welchem fremden Port verbunden ist
 * Aber nützt mir die Info überhaupt was?
 * Eigentlich reicht mir zu wissen mit welcher IP4 vom Gegenüber mein Port X verbunden ist und das weiß ich
 * auch so
 */
void *absint_com_AI_sender(void* arg){
	//ggf. Erweitern damit neu gesendet wird, wenn sich am Interface Setup was ändert.
	//aktuell ist nix Hotplug. Nur beim Startup des Programms wird gesendet.
	//Der Thread hier kann auch auf beliebige weitere Nachrichten erweitert werden.
	//freilich auch iterrierend, prüfend usw.
	//gegenwärtig erfolgt einfach nur das Senden zum Set up und eine Terminierung.
	int *s=((struct absintSendThreadArgPassing *)arg)->socket;
	pthread_t *threadindex=((struct absintSendThreadArgPassing *)arg)->threadindex;
	struct AbsintInterfaces *ifcollect=((struct absintSendThreadArgPassing *)arg)->ifcollect;
	//Free the arguments passing memory space
	free(arg);
	int err;

//	unsigned char msg_send[INTER_AI_MSG_MAX_SIZE];
//	err = senddetermined(*s,msg_send,strlen(msg_send));

	err=0;

	return (void *)(uintptr_t)err;
}
void *absint_com_AI_server(void* arg){
	int aiport=((struct absintComThreadArgPassing *)arg)->destport;
	struct in_addr listenIP4=((struct absintComThreadArgPassing *)arg)->destIP4;
	struct ComThrShMem *shmem=((struct absintComThreadArgPassing *)arg)->shmem;
	sem_t *sem_mainsynch=((struct absintComThreadArgPassing *)arg)->sem_mainsynch;
	pthread_t *threadindex=((struct absintComThreadArgPassing *)arg)->threadindex;
	struct AbsintInterfaces *ifcollect=((struct absintComThreadArgPassing *)arg)->ifcollect;
	//Free the arguments passing memory space
	free(arg);
	int err;
	int pthreadoldcancelstate;

	ABSINT_INET_SOCKET_SERVER_SETUP;

	ABSINT_INET_SOCKET_START_AI_SENDER_THREAD;



	//REMEMBER:
	//After every socket-readout check for bytes_recvd==0
	//Then there is a disconnect -> handle it
	//Aufpassen beim Connection-Loss: Der Sender-Thread muss dann auch beachtet werden
	//Entweder nach Reconnect neustarten oder im Sender Thread ein Handler einbauen
	//der das Socket überwacht und solange pausiert bis es wieder gültig ist.
	NextMsg:
    bytes_recvd= recv(s , msg_recvd , sizeof(msg_recvd) , 0);
    	msg_recvd[bytes_recvd]= '\0';
		printf(ANSI_COLOR_RED);
    	printf("\n-->Message from Server: %d Bytes.",bytes_recvd);
		printf(ANSI_COLOR_RESET);
		printf("\n");
    	switch (msg_recvd[0]) {
    	}

	err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
	TryShMemAccessAgain:
    sem_wait(&(shmem->sem_shmem));
    if(shmem->flags & FLAG_ABSINT_COMTHR_SHMEM_NEW){
    	sem_post(&(shmem->sem_shmem));
    	goto TryShMemAccessAgain;
    }
    //Shared Memory Access
    sem_post(sem_mainsynch);
    sem_post(&(shmem->sem_shmem));
	err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);

	goto NextMsg;


	//If Client closed Connection (or get lost...)
	//Jump back and get ready to accept new Connection
	//(Sort it in the right Place...)
	goto AcceptNewConnection;

	return (void *)(uintptr_t)err;
}
void *absint_com_AI_client(void* arg){
	int aiport=((struct absintComThreadArgPassing *)arg)->destport;
	struct in_addr aiserverIP4=((struct absintComThreadArgPassing *)arg)->destIP4;
	struct ComThrShMem *shmem=((struct absintComThreadArgPassing *)arg)->shmem;
	sem_t *sem_mainsynch=((struct absintComThreadArgPassing *)arg)->sem_mainsynch;
	pthread_t *threadindex=((struct absintComThreadArgPassing *)arg)->threadindex;
	struct AbsintInterfaces *ifcollect=((struct absintComThreadArgPassing *)arg)->ifcollect;
	//Free the arguments passing memory space
	free(arg);
	int err;
	int pthreadoldcancelstate;

	ABSINT_INET_SOCKET_CLIENT_CONNECT;

	ABSINT_INET_SOCKET_START_AI_SENDER_THREAD;



	//REMEMBER:
	//After every socket-readout check for bytes_recvd==0
	//Then there is a disconnect -> handle it
	NextMsg:
    bytes_recvd= recv(s , msg_recvd , sizeof(msg_recvd) , 0);
    	msg_recvd[bytes_recvd]= '\0';
		printf(ANSI_COLOR_RED);
    	printf("\n-->Message from Server: %d Bytes.",bytes_recvd);
		printf(ANSI_COLOR_RESET);
		printf("\n");
    	switch (msg_recvd[0]) {
    	}

	err = pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,&pthreadoldcancelstate);
	TryShMemAccessAgain:
    sem_wait(&(shmem->sem_shmem));
    if(shmem->flags & FLAG_ABSINT_COMTHR_SHMEM_NEW){
    	sem_post(&(shmem->sem_shmem));
    	goto TryShMemAccessAgain;
    }
    //Shared Memory Access
    sem_post(sem_mainsynch);
    sem_post(&(shmem->sem_shmem));
	err = pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&pthreadoldcancelstate);

	goto NextMsg;

	return (void *)(uintptr_t)err;
}


int secure_device_is_up(char *dev){
	int err;err=0;
	//2>&1: Redirect stderr to stdout stream
	char cmd[strlen("ip link show  2>&1")+
				 strlen(dev)+1];
	snprintf(cmd,sizeof(cmd),"ip link show %s 2>&1",dev);
	FILE *fp;
	char buf[1024];
	char *rd;
	int i;i=0;

	fp = popen(cmd, "r");
	if (fp == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}
//	while (fgets(buf, sizeof(buf)-1, fp) != NULL) {
//	}
//	sleep(1);
	while ((buf[i]=fgetc(fp)) != EOF) {
		i++;
	}
	buf[i]='\0';
	pclose(fp);

	char ipcmd_down[strlen("ip link set dev  down")+strlen(dev)+1];
//	memset(ipcmd,0,sizeof(ipcmd));
	snprintf(ipcmd_down,sizeof(ipcmd_down),"ip link set dev %s down",dev);
	char ipcmd_up[strlen("ip link set dev  up")+strlen(dev)+1];
//	memset(ipcmd,0,sizeof(ipcmd));
	snprintf(ipcmd_up,sizeof(ipcmd_up),"ip link set dev %s up",dev);

//	printf("buf:\n");
//	printf("%s\n",buf);
//	printf("cmd1:\n%s\n",ipcmd_up);
//	printf("cmd2:\n%s\n",ipcmd_down);
//	exit(0);

	if(strstr(buf,"state UP")!=NULL){
	}else if(strstr(buf,"state DOWN")!=NULL){
		printfc(gray,"INFO:");printf(" Device %s is DOWN. Calling: %s\n",dev,ipcmd_up);
		system(ipcmd_up);
	}else {
		printfc(gray,"INFO:");printf(" Device state of %s is something else.\n\tCalling: %s\n\tand: %s\n",dev,ipcmd_down,ipcmd_up);
		system(ipcmd_down);
		system(ipcmd_up);
	}

	return 0;
}




#undef NO_ABSINT_ADDITIONAL_FUNCTIONS_C_FUNCTIONS
