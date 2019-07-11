/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#include <sys/stat.h>

#include "ollerus.h"

#include "ollerus_extern_functions.h"

/*
 * Some Functions like the Bandwidth-Measurement and Connection Continuity Check
 */
 
 
 
#ifdef DEBUG
	int DebugPrintCount=0;
#endif


//#ifndef REMAINDER_EXTENDED_H
//	static int senddetermined (int socket, char *msg, int msglen) {
//		int bytes_sent;
//	//	msglen = strlen(msg);
//		SendAgain:
//		bytes_sent = send(socket, msg, msglen, 0);
//		switch(bytes_sent) {
//		case -1:
//			//Something gone wrong. Read out errno for further detail
//			return errno;
//			break;
//		default:
//			if(bytes_sent==msglen) {
//				//Everything sent, everything done. Just fine and we are finished.
//			} else if(bytes_sent<msglen) {
//				//Not everything sent, but the beginning. Go on and send the rest.
//				msg=msg+bytes_sent;
//				msglen=msglen-bytes_sent;
//				goto SendAgain;
//			} else {
//				//How could this happen o.O ?
//				//SHUTDOWN!!!
//				//SHUTDOWN!!!
//				//Just kidding. Should never occur...
//				return OPERATION_ERR_NEVER_HAPPEN;
//			}
//			break;
//		}
//		return 0;
//	}
//#endif



// For now just do it with a simple flush from few kb Size
int monitorBandwidth(char mode, char **dest, int duration){
	int err=0;

	int s; /* connected socket descriptor */
	struct hostent *hp; /* pointer to host info for remote host */
	struct servent *sp; /* pointer to service information */
	struct linger linger = {1,1};
		//allow lingering, graceful close;
		//used when setting SO_LINGER

	long timevar; //contains time returned by time()
	char *ctime(); //declare time formatting routine

	time_t systime,systimeold;

	struct sockaddr_in myaddr_in; //for local socket address
	struct sockaddr_in peeraddr_in; //for peer socket address

	int addrlen, i;
	char *msg;
	unsigned int bytes_recvd;
	char msg_recvd[RECVBUFFSIZEBWMEASURE];
	unsigned long long count;
	unsigned long long lnc;//linecount
	lnc=0;

	umask(0);
	CREATE_PROGRAM_PATH(*args);

	#define _LogPathAppendix_Subfolders log/Monitor
				//Mind that the preceding last "/" is missing here and is concatenated two lines below
	#define _LogPathAppendix_FName_1 MonitorBW_
				#define _LogPathAppendix_1 _LogPathAppendix_Subfolders/_LogPathAppendix_FName_1
	#define _LogPathAppendix_DateTemplate XXXX-XX-XX_XX:XX:XX
	#define _LogPathAppendix_Suffix log
		#define _LogPathAppendix_Template CAT(_LogPathAppendix_1, _LogPathAppendix_DateTemplate._LogPathAppendix_Suffix)

	#define LogPathAppendix_Subfolders STRING_EXP(_LogPathAppendix_Subfolders)
	#define LogPathAppendix_FName_1 STRING_EXP(_LogPathAppendix_FName_1)
	#define LogPathAppendix_1 STRING_EXP(_LogPathAppendix_1)
	#define LogPathAppendix_DateTemplate STRING_EXP(_LogPathAppendix_DateTemplate)
	#define LogPathAppendix_Suffix STRING_EXP(_LogPathAppendix_Suffix)
		#define LogPathAppendix_Template STRING_EXP(_LogPathAppendix_Template)

	//	#define LogPath_DateOffset sizeof(LogPathAppendix_1)
	//	#define LogPath_DateLength sizeof(LogPathAppendix_DateTemplate)-1

	char LogPath[strlen(ProgPath)+sizeof(LogPathAppendix_Template)+4];//We allocate 4 Bytes more, because if a file already exists, we create a new one with trailing incrementing counter.
	char *LogPath_DateOffset = LogPath+sizeof(LogPathAppendix_1)+strlen(ProgPath)-1;
	int fidx=0;
	char *LogPath_IndexOff = LogPath_DateOffset+sizeof(LogPathAppendix_DateTemplate)-1;
	{// Get sure, that the whole Folder-Path exists. Maybe creating it.
	//DO INCLUDE the preceding '/' from the folder-path
		char LogFolderPath[strlen(ProgPath)+sizeof(LogPathAppendix_Subfolders)+1];
		snprintf(LogFolderPath,sizeof(LogFolderPath),"%s%s/",ProgPath,LogPathAppendix_Subfolders);
		//Check the complete Path and eventually create the folders
		CREATE_COMPLETE_FOLDER_PATH(LogFolderPath);
	}

	snprintf(LogPath,strlen(ProgPath)+sizeof(LogPathAppendix_1),"%s%s",ProgPath,LogPathAppendix_1);



	switch (mode){
	case DEBUG_SERVER:
		printf("\nStarting Server for Bandwidth-Measurement.\n");

		int ls; /* listen socket descriptor */

		pthread_t threads_adapttxpow[THREADS_CLIENTS_MAX]; //to store the references to active connections
		//Don't forget the Mutex-handling on this thing, if the necessity comes in place for you!

		printf("\nGetting ready to listen for incoming connections...\n");
		memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in));
		printf("\tsockaddr_in struct for port listening...\n");
		myaddr_in.sin_family = AF_INET;
		printf("\t...Socket Family to: %d\n", myaddr_in.sin_family);
		printf("\t\t-To Check->Should be: %d (AF_INET)\n", AF_INET);
		myaddr_in.sin_addr.s_addr = INADDR_ANY;
		printf("\t...Listening to wildcard-address: %d\n", myaddr_in.sin_addr.s_addr);

		myaddr_in.sin_port = PORTBWMEASURE;
		printf("\t...Port number: %d\n", myaddr_in.sin_port);

		printf("\tCreating listening socket...\n");
		ls = socket (AF_INET, SOCK_STREAM, 0);
		if (ls == -1) {
			fprintf(stderr, "unable to create socket\n");
			exit(1);
		}
		printf("\t...Socket created!\n");
		printf("\tBinding listen Address to socket...\n");
		if (bind(ls, &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
			fprintf(stderr, "unable to bind address\n");
			exit(1);
		}
		printf("\t...bound!\n");
		printf("\tInitiating listening on socket...\n");
		if (listen(ls, 5) == -1) {
			fprintf(stderr, "unable to listen on socket\n");
			exit(1);
		}
		printf("...Initiation complete!\n");

		addrlen = sizeof(struct sockaddr_in);

		printf("\nWaiting for incoming Connections...\n");
		unsigned long long bytesges=0;
		unsigned char timediff;
		double bandwidth;
		double bandwidthMovAve;
		//Using a Ring-Buffer for the Moving Average of the calculated Bandwidth
		double bandwidthMovAveAr[BWNUMAVERAGEINTERVALLS];
		char bwMovAveIndex=0;

	    FILE *logf;

		StartMonitorBWServer:

		lnc=0;
		bwMovAveIndex=0;
		bandwidthMovAve=0;
		while( s = accept(ls, (struct sockaddr *)&peeraddr_in, (socklen_t*)&addrlen) ) {
			if (s<0) {
				perror("accept failed");
				return 1;
			}
			puts("");
			puts("Incoming Connection accepted");

			{
			struct timespec ts;
			struct tm time;
			GetRealTimeM(nano,&ts,CLOCK_REALTIME);
			localtime_r(&(ts.tv_sec), &time);
			time.tm_mon+=1;
			time.tm_year+=1900;
			//The sizeofs one line below automatically give +2. One for the "\0" and one for the dot "."
			snprintf(LogPath_DateOffset,sizeof(LogPathAppendix_DateTemplate)+sizeof(LogPathAppendix_Suffix),"%04d-%02d-%02d_%02d:%02d:%02d.%s",time.tm_year,time.tm_mon,time.tm_mday,time.tm_hour,time.tm_min,time.tm_sec,LogPathAppendix_Suffix);
			}
			IndexedFile:
		    logf = fopen(LogPath, "r");
		    if (logf){
		    	fclose(logf);
		    	printf("A Log-File with such Name does already exist. Creating Indexed one...\n");
		    	if(fidx>9){
		    		printfc(red,"ERROR");printf("This is ridiculous! Are you kidding me? I won't create more than 10 Files for one Logging-Second!\n");
		    		exit(9000);
		    	}
		    	snprintf(LogPath_IndexOff,5+sizeof(LogPathAppendix_Suffix),"_(%d).%s",fidx,LogPathAppendix_Suffix);
		    	fidx++;
		    	goto IndexedFile;
		    }else{
		    	fidx=0;
		    	switch(errno) {
		    		case EACCES:
		    			printf("\tERROR: Couldn't open cfg-File!\n\t\tReason:Permission denied!\n");
		    			exit(FILE_ERR_PERMISSION_DENIED);
		    		break;
		    		case ENOENT:
		    			logf = fopen(LogPath, "w+");
		    		    if(!logf){
		    		    	printf("\t\tERROR: Couldn't create new cfg-file!\n");
		    		    	return FILE_ERR_PERMISSION_DENIED;
		    		    }
		    			printf("\tStarting Log-File: %s\n",LogPath);
		    		break;
					default:
						fprintf(stderr, "Ups, seems like we've encountered a case, which isn't caught yet :o( (While Log-File Opening)\n");
						exit(MAIN_ERR_FUNC_INCOMPLETE);
					break;
		    	}
		    }
			#undef _LogPathAppendix_Subfolders
			#undef _LogPathAppendix_FName_1
			#undef _LogPathAppendix_1
			#undef _LogPathAppendix_DateTemplate
			#undef _LogPathAppendix_Suffix
			#undef _LogPathAppendix_Template
			#undef LogPathAppendix_Subfolders
			#undef LogPathAppendix_FName_1
			#undef LogPathAppendix_1
			#undef LogPathAppendix_DateTemplate
			#undef LogPathAppendix_Suffix
			#undef LogPathAppendix_Template

			fprintf(logf,"Timestamp\tBandwidth\tTesting_Statistic\n"
						 "Timestamp\tCurrent_Datarate\tMoving_Average\tBytes_per_Calculation_Interval_(Received_Amount_of_Data)\n"
						 "YYYY-MM-DD,_HH:MM:SS.Nanoseconds\tMbit/s\tMbit/s\tMegaBytes\n");

			time(&systimeold);
			while((bytes_recvd=recv(s,msg_recvd,RECVBUFFSIZEBWMEASURE,0)) > 0 )
			{
				bytesges+=bytes_recvd;
				time(&systime);
				timediff=systime-systimeold;
				if((timediff)>=BWMEASURETIMEINTERVALL){
					bandwidth=(8*bytesges)/timediff;
					//Calculating the Moving Average
					bandwidthMovAve=bandwidthMovAve-bandwidthMovAveAr[bwMovAveIndex];
						//We save already the Divided Values in the Ring-Buffer
						//We calculate the Moving Average like Value1/Number + Value2/Number ... instead of (Value1+Value2+...)/Number
					bandwidthMovAveAr[bwMovAveIndex]=bandwidth/BWNUMAVERAGEINTERVALLS;
					bandwidthMovAve=bandwidthMovAve+bandwidthMovAveAr[bwMovAveIndex];
					bwMovAveIndex++;
					if(bwMovAveIndex>=BWNUMAVERAGEINTERVALLS){
						bwMovAveIndex=0;
					}
					//Printing out
					lnc++;
					printf("%llu. Bandwidth: Mov.Ave.: %f Mbit/s\t| Current: %f Mbit/s\t|  %llu.%02llu MByte received\n",lnc,(bandwidthMovAve)/1048576,(bandwidth)/1048576,bytesges/1048576,bytesges%1048576);
					print_daytime_file(logf);fprintf(logf,"\t%f\t%f\t%llu.%02llu\n",(bandwidth)/1048576,(bandwidthMovAve)/1048576,bytesges/1048576,bytesges%1048576);
					bytesges=0;
					time(&systimeold);
				}
			}
			printf("Client closed connection!\n");
			goto ConnectionClosedByClient;
		}
		//Shutting down the Connection by myself. Shouldn't occur in this Operation Mode...
		//So just prepare it, maybe for future use?
		printf("Shutting down the connection...\n");
		if (shutdown(s, 1) == -1) {
			fprintf(stderr, "unable to shutdown socket\n");
			exit(1);
		}
		ConnectionClosedByClient:
		printf("Closing the socket...\n");
		close(s);
		printf("\t...Done!\n");
		printf("Closing the Log-File...\n");
		fclose(logf);
		printf("\t...Done!\n");
		printf("Starting new Server over again.\n");
		puts("");
		memset(bandwidthMovAveAr,0,sizeof(bandwidthMovAveAr));
		goto StartMonitorBWServer;
		break;
/*
 * ^^^^^^^^^^^^^^^^^
 * Server Mode
 * ----------------------------------
 *
 * Platzhalter für die Übersicht
 *
 * ----------------------------------
 * Client Mode
 * vvvvvvvvvvvvvvvvv
 */
	case DEBUG_CLIENT:
		printf("\nWorking now as Client and connecting to a server to measure the Bandwidth.\n");

		time_t runtimestart,runtimenow;

		printf("\nSetting up Connection to Access Point...\n");
		memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in));
		printf("\tsockaddr_in struct for connection...\n");
		peeraddr_in.sin_family = AF_INET;
		printf("\t...Socket Family to: %d\n", peeraddr_in.sin_family);
		printf("\t\t-To Check->Should be: %d (AF_INET)\n", AF_INET);
		inet_pton(AF_INET,*dest,&(peeraddr_in.sin_addr));
		char printaddrv4[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(peeraddr_in.sin_addr), printaddrv4, INET_ADDRSTRLEN);
		printf("\t...Connecting to IP Address (IPv4): %s\n", printaddrv4);
		peeraddr_in.sin_port = PORTBWMEASURE;
		printf("\t...Port number: %d\n", peeraddr_in.sin_port);
		/* Create the socket. */
		printf("\tCreating Socket...\n");
		s = socket (AF_INET, SOCK_STREAM, 0);
		if (s == -1) {
			fprintf(stderr,"unable to create socket\n");
			exit(1);
		}
		printf("\t...Socket created!\n");
		printf("\tNow connecting...\n");
		if (connect(s, &peeraddr_in, addrlen=sizeof(struct sockaddr_in)) ==-1) {
			printf("\n: ERROR: Couldn't connect to specified Peer Address. | ERRNO: %d\n",errno);
			return NETWORK_ERR_NO_CONNECTION;
		}

		if (getsockname(s, &myaddr_in, &addrlen) == -1) {
			fprintf(stderr, "unable to read socket address\n");
			exit(1);
		}
		time(&timevar);
		printf("Connected to %s on port %u at %s\n","TODO",ntohs(myaddr_in.sin_port),ctime(&timevar));
		printf("\n");
		printf("You want the statistics? The Server has what you wants!\n");
		printf("\n");

		runtimestart=(time_t)GetRealTimeM();

		unsigned char msg_send[RECVBUFFSIZEBWMEASURE/10];
		memset(msg_send,42,sizeof(msg_send));
		RESEND:
		err = senddetermined(s,msg_send,sizeof(msg_send));
			//Was just for Debugging: Specified Waiting time to determine a Sending Rate
//			struct timespec remainingdelay;
//			remainingdelay.tv_sec = 0;
//			remainingdelay.tv_nsec = 250000000;
//			do {
//				err = nanosleep(&remainingdelay, &remainingdelay);
//			} while (err<0);

		//Detect if Server disconnected:
		if (recv(s,msg_recvd,sizeof(msg_recvd), MSG_PEEK | MSG_DONTWAIT) == 0) {
			// if recv returns zero, that means the connection has been closed:
			goto ConnectionClosedByServer;
		}
		runtimenow=(time_t)GetRealTimeM();
		if( (duration==0) ||
			(runtimenow-runtimestart < duration)){
			goto RESEND;
		}else{
			// Old Comment:
								//Shutting down the Connection by myself. Shouldn't occur in this Operation Mode...
								//So just prepare it, maybe for future use?
			// And Yaay! Today is this time. Preparation comes in Use :D
			printf("Shutting down the connection...\n");
			if (shutdown(s, 1) == -1) {
				fprintf(stderr, "unable to shutdown socket\n");
				exit(1);
			}
		}
		ConnectionClosedByServer:
		printf("Closing the socket...\n");
		close(s);
		printf("\t...Done!\n");
		break;
	default:
		printf("Ups, this was bad...\n");
		break;
	}
	return 0;
}

//REMARK: This current Implementation is only able to give you arround 30% Bandwidth.
//(Und das bereits nur als gedrosselte Brutto-Rate, da ja kleine Nachrichten von 8 Byte Nutzdaten geschickt werden und bei der
//Berechnung der Bandbreite keine Header-Informationen bedacht werden...
//So it is just usefull to compare links while checking the Continuity.
int monitorBandwidthAndContinuityCheck(char mode, char **dest){
	int err=0;

	int s; /* connected socket descriptor */
	struct hostent *hp; /* pointer to host info for remote host */
	struct servent *sp; /* pointer to service information */
	struct linger linger = {1,1};
		//allow lingering, graceful close;
		//used when setting SO_LINGER

	long timevar; //contains time returned by time()
	char *ctime(); //declare time formatting routine

	time_t systime,systimeold;
	unsigned char timediff;

	struct sockaddr_in myaddr_in; //for local socket address
	struct sockaddr_in peeraddr_in; //for peer socket address

	int addrlen, i;
	char *msg;
	unsigned long long count=1;
	unsigned long long countold=0;
	unsigned long long lnc=0;//linecount

	switch (mode){
	case DEBUG_SERVER:
		printf("\nStarting Server for Bandwidth-Measurement with Data Stream Continuity Check.\n");

		int ls; /* listen socket descriptor */

		pthread_t threads_adapttxpow[THREADS_CLIENTS_MAX]; //to store the references to active connections
		//Don't forget the Mutex-handling on this thing, if the necessity comes in place for you!

		printf("\nGetting ready to listen for incoming connections...\n");
		memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in));
		printf("\tsockaddr_in struct for port listening...\n");
		myaddr_in.sin_family = AF_INET;
		printf("\t...Socket Family to: %d\n", myaddr_in.sin_family);
		printf("\t\t-To Check->Should be: %d (AF_INET)\n", AF_INET);
		myaddr_in.sin_addr.s_addr = INADDR_ANY;
		printf("\t...Listening to wildcard-address: %d\n", myaddr_in.sin_addr.s_addr);

		myaddr_in.sin_port = PORTBWMEASURE;
		printf("\t...Port number: %d\n", myaddr_in.sin_port);

		printf("\tCreating listening socket...\n");
		ls = socket (AF_INET, SOCK_STREAM, 0);
		if (ls == -1) {
			fprintf(stderr, "unable to create socket\n");
			exit(1);
		}
		printf("\t...Socket created!\n");
		printf("\tBinding listen Address to socket...\n");
		if (bind(ls, &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
			fprintf(stderr, "unable to bind address\n");
			exit(1);
		}
		printf("\t...bound!\n");
		printf("\tInitiating listening on socket...\n");
		if (listen(ls, 5) == -1) {
			fprintf(stderr, "unable to listen on socket\n");
			exit(1);
		}
		printf("...Initiation complete!\n");

		addrlen = sizeof(struct sockaddr_in);

		printf("\nWaiting for incoming Connections...\n");
		unsigned int bytes_recvd;
		unsigned char msg_recvd[sizeof(count)];
		unsigned long long bytesges=0;
		double bandwidth;
		double bandwidthMovAve;
		//Using a Ring-Buffer for the Moving Average of the calculated Bandwidth
		double bandwidthMovAveAr[BWNUMAVERAGEINTERVALLS];
		char bwMovAveIndex=0;
		while( s = accept(ls, (struct sockaddr *)&peeraddr_in, (socklen_t*)&addrlen) ) {
			if (s<0) {
				perror("accept failed");
				return 1;
			}
			puts("");
			puts("Incoming Connection accepted");

			char msg_locbuff[sizeof(msg_recvd)];
			char buff_fill;
			countold=0;
			time(&systimeold);
			while((bytes_recvd=recv(s,msg_recvd,sizeof(msg_recvd),0)) > 0 )
			{
				//Get sure that we have received and read out 8 Bytes from the Socket
				//(It could be any Value between 1-8 and very propably is a 4...)
				//For sure we want the whole long long.
				buff_fill=bytes_recvd;
				while(buff_fill < sizeof(msg_recvd)){
					bytes_recvd=recv(s,msg_recvd+buff_fill,sizeof(msg_recvd)-buff_fill,0);
					buff_fill+=bytes_recvd;
				}
				memcpy(&count,msg_recvd,sizeof(count));
				if((countold+1)!=count){
//					Shutting down the Connection by myself.
					printf("WARNING: Stream Discontinuity detected!\n");
					printf("\t--> INFO: Last Value:\n");
					printf("\t--> Value: %llu\t| bytes_recvd: %d\n",count,bytes_recvd);
					printf("\t--> Previous Value: %llu\n",countold);
					printf("Shutting down the connection...\n");
					goto ConnectionCloseServer;
				}else{
					countold=count;
				}
				bytesges+=bytes_recvd;
				time(&systime);
				timediff=systime-systimeold;
				if((timediff)>=BWMEASURETIMEINTERVALL){
					bandwidth=(8*bytesges)/timediff;
					//Calculating the Moving Average
					bandwidthMovAve=bandwidthMovAve-bandwidthMovAveAr[bwMovAveIndex];
						//We save already the Divided Values in the Ring-Buffer
						//We calculate the Moving Average like Value1/Number + Value2/Number ... instead of (Value1+Value2+...)/Number
					bandwidthMovAveAr[bwMovAveIndex]=bandwidth/BWNUMAVERAGEINTERVALLS;
					bandwidthMovAve=bandwidthMovAve+bandwidthMovAveAr[bwMovAveIndex];
					bwMovAveIndex++;
					if(bwMovAveIndex>=BWNUMAVERAGEINTERVALLS){
						bwMovAveIndex=0;
					}
					//Printing out
					lnc++;
					printf("%llu. Bandwidth: Mov.Ave.: %f Mbit/s\t| Current: %f Mbit/s\t|  %llu.%02llu MByte received\n",lnc,(bandwidthMovAve)/1048576,(bandwidth)/1048576,bytesges/1048576,bytesges%1048576);
					printf("\tContinuity still given. Current Value: %llu\n",count);
					bytesges=0;
					time(&systimeold);
				}
			}
			printf("Client closed connection!\n");
			goto ConnectionCloseServer;
		}
		//Shutting down the Connection by myself.
		ConnectionCloseServer:
		printf("Shutting down the connection...\n");
		if (shutdown(s, 1) == -1) {
			fprintf(stderr, "unable to shutdown socket\n");
			exit(1);
		}
		printf("Closing the socket...\n");
		close(s);
		printf("\t...Done!\n");
		break;
/*
 * ^^^^^^^^^^^^^^^^^
 * Server Mode
 * ----------------------------------
 *
 * Platzhalter für die Übersicht
 *
 * ----------------------------------
 * Client Mode
 * vvvvvvvvvvvvvvvvv
 */
	case DEBUG_CLIENT:
		printf("\nWorking now as Client and connecting to a server to\nmeasure the Bandwidth and Check the Continuity of a Data Stream.\n");

		printf("\nSetting up Connection to Access Point...\n");
		memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in));
		printf("\tsockaddr_in struct for connection...\n");
		peeraddr_in.sin_family = AF_INET;
		printf("\t...Socket Family to: %d\n", peeraddr_in.sin_family);
		printf("\t\t-To Check->Should be: %d (AF_INET)\n", AF_INET);
		inet_pton(AF_INET,*dest,&(peeraddr_in.sin_addr));
		char printaddrv4[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(peeraddr_in.sin_addr), printaddrv4, INET_ADDRSTRLEN);
		printf("\t...Connecting to IP Address (IPv4): %s\n", printaddrv4);
		peeraddr_in.sin_port = PORTBWMEASURE;
		printf("\t...Port number: %d\n", peeraddr_in.sin_port);
		/* Create the socket. */
		printf("\tCreating Socket...\n");
		s = socket (AF_INET, SOCK_STREAM, 0);
		if (s == -1) {
			fprintf(stderr,"unable to create socket\n");
			exit(1);
		}
		printf("\t...Socket created!\n");
		printf("\tNow connecting...\n");
		if (connect(s, &peeraddr_in, addrlen=sizeof(struct sockaddr_in)) ==-1) {
			printf("\n: ERROR: Couldn't connect to specified Peer Address. | ERRNO: %d\n",errno);
			return NETWORK_ERR_NO_CONNECTION;
		}

		if (getsockname(s, &myaddr_in, &addrlen) == -1) {
			fprintf(stderr, "unable to read socket address\n");
			exit(1);
		}
		time(&timevar);
		printf("Connected to %s on port %u at %s\n","TODO",ntohs(myaddr_in.sin_port),ctime(&timevar));
		printf("\n");

		unsigned char msg_send[sizeof(unsigned long long)];
		count=1;
		printf("Sending Value: %llu\n",count);
		time(&systimeold);
		RESEND:
		time(&systime);
		timediff=systime-systimeold;
		if((timediff)>=BWMEASURETIMEINTERVALL){
			//Printing out
			printf("Sending Value: %llu\n",count);
			time(&systimeold);
		}
		memcpy(msg_send,&count,sizeof(count));
		err = senddetermined(s,msg_send,sizeof(msg_send));
			//Was just for Debugging: Specified Waiting time to determine a Sending Rate
//			struct timespec remainingdelay;
//			remainingdelay.tv_sec = 0;
//			remainingdelay.tv_nsec = 250000000;
//			do {
//				err = nanosleep(&remainingdelay, &remainingdelay);
//			} while (err<0);

		//Detect if Server disconnected:
		if (recv(s,msg_recvd,sizeof(msg_recvd), MSG_PEEK | MSG_DONTWAIT) == 0) {
			// if recv returns zero, that means the connection has been closed:
			goto ConnectionClosedByServer;
		}
		if(count==ULLONG_MAX){
			count=1;
		}else{
			count++;
		}
		goto RESEND;
		//Shutting down the Connection by myself. Shouldn't occur in this Operation Mode...
		//So just prepare it, maybe for future use?
		printf("Shutting down the connection...\n");
		if (shutdown(s, 1) == -1) {
			fprintf(stderr, "unable to shutdown socket\n");
			exit(1);
		}
		ConnectionClosedByServer:
		printf("WARNING: Server disconnected. Maybe it detected a Stream Discontinuity?\n");
		printf("Closing the socket...\n");
		close(s);
		printf("\t...Done!\n");
		break;
	default:
		printf("Ups, this was bad...\n");
		break;
	}
	return 0;
}


//Really not finished!!!
//And also really crappy for now. Like this it cannot work -.- Stupid thinking...
//Do it more like above
int continuityCheck(char mode, char **dest){
	int err=0;

	int s; /* connected socket descriptor */
	struct hostent *hp; /* pointer to host info for remote host */
	struct servent *sp; /* pointer to service information */
	struct linger linger = {1,1};
		//allow lingering, graceful close;
		//used when setting SO_LINGER

	long timevar; //contains time returned by time()
	char *ctime(); //declare time formatting routine

	time_t systime,systimeold;

	struct sockaddr_in myaddr_in; //for local socket address
	struct sockaddr_in peeraddr_in; //for peer socket address

	int addrlen, i;
	char *msg;
	unsigned int bytes_recvd;
	char msg_recvd[RECVBUFFSIZEBWMEASURE];
	unsigned long long count=0;
	unsigned long long countold=0;
	char msg_send[sizeof(unsigned long long)];

	switch (mode){
	case DEBUG_SERVER:
		printf("\nStarting Server for Bandwidth-Measurement.\n");

		int ls; /* listen socket descriptor */

		pthread_t threads_adapttxpow[THREADS_CLIENTS_MAX]; //to store the references to active connections
		//Don't forget the Mutex-handling on this thing, if the necessity comes in place for you!

		printf("\nGetting ready to listen for incoming connections...\n");
		memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in));
		printf("\tsockaddr_in struct for port listening...\n");
		myaddr_in.sin_family = AF_INET;
		printf("\t...Socket Family to: %d\n", myaddr_in.sin_family);
		printf("\t\t-To Check->Should be: %d (AF_INET)\n", AF_INET);
		myaddr_in.sin_addr.s_addr = INADDR_ANY;
		printf("\t...Listening to wildcard-address: %d\n", myaddr_in.sin_addr.s_addr);

		myaddr_in.sin_port = PORTBWMEASURE;
		printf("\t...Port number: %d\n", myaddr_in.sin_port);

		printf("\tCreating listening socket...\n");
		ls = socket (AF_INET, SOCK_STREAM, 0);
		if (ls == -1) {
			fprintf(stderr, "unable to create socket\n");
			exit(1);
		}
		printf("\t...Socket created!\n");
		printf("\tBinding listen Address to socket...\n");
		if (bind(ls, &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
			fprintf(stderr, "unable to bind address\n");
			exit(1);
		}
		printf("\t...bound!\n");
		printf("\tInitiating listening on socket...\n");
		if (listen(ls, 5) == -1) {
			fprintf(stderr, "unable to listen on socket\n");
			exit(1);
		}
		printf("...Initiation complete!\n");

		addrlen = sizeof(struct sockaddr_in);

		printf("\nWaiting for incoming Connections...\n");
		unsigned char timediff;
		while( s = accept(ls, (struct sockaddr *)&peeraddr_in, (socklen_t*)&addrlen) ) {
			if (s<0) {
				perror("accept failed");
				return 1;
			}
			puts("");
			puts("Incoming Connection accepted");

			//Erster Sendevorgang:
			count=0;
			countold=0;
			memcpy(msg_send,&count,sizeof(count));
			err = senddetermined(s,msg_send,sizeof(msg_send));
			time(&systimeold);
			//Dann gleichermaßen für Server und Client:
			//	Empfangen, Prüfen, Schicken.
			//	Server erhöht und wartet. Client schickt zurück
			while((bytes_recvd=recv(s,msg_recvd,RECVBUFFSIZEBWMEASURE,0)) > 0 )
			{
				count++;
				//Buffer auslesen -> countold
				memcpy(&countold,msg_recvd,sizeof(countold));
				//Compare countold+1==count
				//if match do all the stuff down below
				//else report the discontinuity
				if((countold+1)!=count){
					//Shutting down the Connection by myself.
					printf("WARNING: Stream Discontinuity detected!\n");
					printf("Shutting down the connection...\n");
					if (shutdown(s, 1) == -1) {
						fprintf(stderr, "unable to shutdown socket\n");
						exit(1);
					}
					goto ConnectionCloseServer;
				}

				//Print out every some Time Intervalls, that everything is alright.
				time(&systime);
				timediff=systime-systimeold;
				if((timediff)>=BWMEASURETIMEINTERVALL){
					//Printing out
					printf("Stream Continuity still given. (Current Value: %llu)\n",count);
					time(&systimeold);
				}
				//
					//Specified Waiting time to determine a Sending Rate, so that not the entire Bandwidth is used.
					//Just outcommend the waiting stuff to send at max Rate.
				struct timespec remainingdelay;
				remainingdelay.tv_sec = 0;
				remainingdelay.tv_nsec = 1000000;
				do {
					err = nanosleep(&remainingdelay, &remainingdelay);
				} while (err<0);
				//send out the current (highered) value
				memcpy(msg_send,&count,sizeof(count));
				err = senddetermined(s,msg_send,sizeof(msg_send));
			}
			//If we drop out from the recv, then the Client disconnected and recv delivered a '0'
			printf("WARNING: Client closed connection. Maybe it detected a Stream Discontinuity?\n");
			goto ConnectionCloseServer;
		}
		ConnectionCloseServer:
		printf("Closing the socket...\n");
		close(s);
		printf("\t...Done!\n");
		break;
/*
 * ^^^^^^^^^^^^^^^^^
 * Server Mode
 * ----------------------------------
 *
 * Platzhalter für die Übersicht
 *
 * ----------------------------------
 * Client Mode
 * vvvvvvvvvvvvvvvvv
 */
	case DEBUG_CLIENT:
		printf("\nWorking now as Client and connecting to a server to measure the Bandwidth.\n");

		printf("\nSetting up Connection to Access Point...\n");
		memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset ((char *)&peeraddr_in, 0, sizeof(struct sockaddr_in));
		printf("\tsockaddr_in struct for connection...\n");
		peeraddr_in.sin_family = AF_INET;
		printf("\t...Socket Family to: %d\n", peeraddr_in.sin_family);
		printf("\t\t-To Check->Should be: %d (AF_INET)\n", AF_INET);
		inet_pton(AF_INET,*dest,&(peeraddr_in.sin_addr));
		char printaddrv4[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(peeraddr_in.sin_addr), printaddrv4, INET_ADDRSTRLEN);
		printf("\t...Connecting to IP Address (IPv4): %s\n", printaddrv4);
		peeraddr_in.sin_port = PORTBWMEASURE;
		printf("\t...Port number: %d\n", peeraddr_in.sin_port);
		/* Create the socket. */
		printf("\tCreating Socket...\n");
		s = socket (AF_INET, SOCK_STREAM, 0);
		if (s == -1) {
			fprintf(stderr,"unable to create socket\n");
			exit(1);
		}
		printf("\t...Socket created!\n");
		printf("\tNow connecting...\n");
		if (connect(s, &peeraddr_in, addrlen=sizeof(struct sockaddr_in)) ==-1) {
			printf("\n: ERROR: Couldn't connect to specified Peer Address. | ERRNO: %d\n",errno);
			return NETWORK_ERR_NO_CONNECTION;
		}

		if (getsockname(s, &myaddr_in, &addrlen) == -1) {
			fprintf(stderr, "unable to read socket address\n");
			exit(1);
		}
		time(&timevar);
		printf("Connected to %s on port %u at %s\n","TODO",ntohs(myaddr_in.sin_port),ctime(&timevar));
		printf("\n");

		time(&systimeold);
		while((bytes_recvd=recv(s,msg_recvd,RECVBUFFSIZEBWMEASURE,0)) > 0 )
		{
			//Buffer auslesen und prüfen

			//if check failed
			{
				//Shutting down the Connection by myself.
				printf("WARNING: Stream Discontinuity detected!\n");
				printf("Shutting down the connection...\n");
				if (shutdown(s, 1) == -1) {
					fprintf(stderr, "unable to shutdown socket\n");
					exit(1);
				}
				goto ConnectionCloseClient;
			}
			//else: Wert in Sendenachricht kopieren und zurück schicken.

			//Print out every some Time Intervalls, that everything is alright.
			time(&systime);
			timediff=systime-systimeold;
			if((timediff)>=BWMEASURETIMEINTERVALL){
				//Printing out
				printf("Stream Continuity still given.\n");
				time(&systimeold);
			}
		}
		//If we drop out from the recv, then the Server disconnected and recv delivered a '0'
		printf("WARNING: Server disconnected. Maybe it detected a Stream Discontinuity?\n");
		ConnectionCloseClient:
		printf("Closing the socket...\n");
		close(s);
		printf("\t...Done!\n");
		printf("Shutting down.\n");
		break;
	default:
		printf("Ups, this was bad...\n");
		break;
	}
	return 0;
}






int executeDebugTask(int argc, char **argstart){
	int err = 0;
	if (strcmp(*(argstart), "monitorbw") == 0){
		argc--;
		argstart++;
		if (argc < 1) {
			printf("To few arguments for the Bandwidth Monitoring.\nShall i act as \"server\" or \"client\"?\n");
			return MAIN_ERR_FEW_CMDS;
		}
		if (strcmp(*(argstart), "server") == 0){
			err=monitorBandwidth(DEBUG_SERVER,NULL,0);
		} else if (strcmp(*(argstart), "client") == 0){
			if (argc<3){
				printf("Not enough arguments for the Client-Mode of Bandwidth Measurement.\n\tNeed the Destination Address of the server.\n\tAnd the Duration in seconds (0 for inifinite run).\n");
				return MAIN_ERR_FEW_CMDS;
			}
//			printf("\n\tIP: %s\n\n",*(argstart+1));
			int duration;
			{
			char *endp;
			duration=(int)strtoul(*(argstart+2),&endp,10);
			if(*endp!='\0'){
				printf("Something wrong with the passed duration\n");
				return(MAIN_ERR_BAD_CMDLINE);
			}
			}
			err=monitorBandwidth(DEBUG_CLIENT,(argstart+1),duration);
		} else{
			printf("Bad Command Line on \"debug monitorbw\"\nAllowed are server and client.\n");
			return MAIN_ERR_BAD_CMDLINE;
		}
	} else if (strcmp(*(argstart), "monitorbwcont") == 0){
		argc--;
		argstart++;
		if (argc < 1) {
			printf("To few arguments for the Bandwidth Monitor and Data Stream Continuity Check.\nShall i act as \"server\" or \"client\"?\n");
			return MAIN_ERR_FEW_CMDS;
		}
		if (strcmp(*(argstart), "server") == 0){
			err=monitorBandwidthAndContinuityCheck(DEBUG_SERVER,NULL);
		} else if (strcmp(*(argstart), "client") == 0){
			if (argc<2){
				printf("Not enough arguments for the Client-Mode of BW Monitor and Stream Continuity Check.\nNeed the Destination Address of the server.\n");
				return MAIN_ERR_FEW_CMDS;
			}
			printf("\n\tIP: %s\n\n",*(argstart+1));
			err=monitorBandwidthAndContinuityCheck(DEBUG_CLIENT,(argstart+1));
		} else{
			printf("Bad Command Line on \"debug monitorbwcont\"\nAllowed are server and client.\n");
			return MAIN_ERR_BAD_CMDLINE;
		}
	} else if (strcmp(*(argstart), "contcheck") == 0){
		argc--;
		argstart++;
		if (argc < 1) {
			printf("To few arguments for the Data Stream Continuity Check.\nShall i act as \"server\" or \"client\"?\n");
			return MAIN_ERR_FEW_CMDS;
		}
		if (strcmp(*(argstart), "server") == 0){
			err=continuityCheck(DEBUG_SERVER,NULL);
		} else if (strcmp(*(argstart), "client") == 0){
			if (argc<2){
				printf("Not enough arguments for the Client-Mode of Stream Continuity Check.\nNeed the Destination Address of the server.\n");
				return MAIN_ERR_FEW_CMDS;
			}
			printf("\n\tIP: %s\n\n",*(argstart+1));
			err=continuityCheck(DEBUG_CLIENT,(argstart+1));
		} else{
			printf("Bad Command Line on \"debug contcheck\"\nAllowed are server and client.\n");
			return MAIN_ERR_BAD_CMDLINE;
		}
	}else{
		printf("Bad Command-Line. Wrong/Unknown Command after \"debug\"\n");
		exit(MAIN_ERR_BAD_CMDLINE);
	}
	return err;
}






int schedule_task_debug(int argc, char **argstart){
	int err=0;
	puts("");

	if (argc < 3) {
		printf("To few arguments for:\n");
		//misuse the err as loop-counter.
		for(err=-1;err<argc;err++)
			printf(" %s", *(argstart+err));
		printf("\n");
		return MAIN_ERR_FEW_CMDS;
	}

	//We need the time in seconds anyway at various spots.
	struct timespec tscur;
	struct tm timcur;

	int schedn=1; //How many Scheduling Time-Points do we have?
	int schedcur=0; //The current Scheduling Point.
	{
		//Initially we skip the first two args and check afterwards, if there are given
		// additional Time-Points; assuming the first one (Date and Time) are valid.
		// If these already are invalid it will terminate anyway afterwards while
		// decoding and saving the given dates.
		int argccur=argc-2;
		char **argcur=argstart+2;
		char *aftp;
		char *startp;
		//Decoding the Date:
		while(argccur > 3){
			strtoul(*argcur,&aftp,10);
			if((*(aftp++)!='-') || (*argcur == aftp)){
				break;
			}
			startp=aftp;
			strtoul(startp,&aftp,10);
			if((*(aftp++)!='-') || (startp == aftp)){
				break;
			}
			startp=aftp;
			strtoul(startp,&aftp,10);
			if((*(aftp)!='\0') || (startp == aftp)){
				break;
			}
			argccur-=2;
			argcur+=2;
			schedn++;
		}
	}

		struct tm tim[schedn];
		memset(tim,0,sizeof(struct tm)*schedn);
		time_t timts[schedn];
		memset(timts,0,sizeof(time_t)*schedn);

	{
		int i;
		char *aftp;
		char *startp;
		for(schedcur=0;schedcur<schedn;schedcur++){
			//At Last: Decoding and Saving the Date:
			tim[schedcur].tm_year=(time_t)strtoul(*argstart,&aftp,10);
			if((*(aftp++)!='-') || (*argstart == aftp)){
				printfc(red,"ERROR");printf(": Wrong Date Format.\n");
				exit(MAIN_ERR_BAD_CMDLINE);
			}
			startp=aftp;
			tim[schedcur].tm_mon=(time_t)strtoul(startp,&aftp,10);
			if((*(aftp++)!='-') || (startp == aftp)){
				printfc(red,"ERROR");printf(": Wrong Date Format.\n");
				exit(MAIN_ERR_BAD_CMDLINE);
			}
			startp=aftp;
			tim[schedcur].tm_mday=(time_t)strtoul(startp,&aftp,10);
			if((*(aftp)!='\0') || (startp == aftp)){
				printfc(red,"ERROR");printf(": Wrong Date Format.\n");
				exit(MAIN_ERR_BAD_CMDLINE);
			}
			argc--;
			argstart++;

			//Decoding the Time:
			tim[schedcur].tm_hour=(time_t)strtoul(*argstart,&aftp,10);
			if((*(aftp++)!=':') || (*argstart == aftp)){
				printfc(red,"ERROR");printf(": Wrong Time Format.\n");
				exit(MAIN_ERR_BAD_CMDLINE);
			}
			startp=aftp;
			tim[schedcur].tm_min=(time_t)strtoul(startp,&aftp,10);
			if((*(aftp++)!=':') || (startp == aftp)){
				printfc(red,"ERROR");printf(": Wrong Time Format.\n");
				exit(MAIN_ERR_BAD_CMDLINE);
			}
			startp=aftp;
			tim[schedcur].tm_sec=(time_t)strtoul(startp,&aftp,10);
			if((*(aftp)!='\0') || (startp == aftp)){
				printfc(red,"ERROR");printf(": Wrong Time Format.\n");
				exit(MAIN_ERR_BAD_CMDLINE);
			}
			argc--;
			argstart++;

			timts[schedcur]=tmToSeconds(&tim[schedcur]);

			//First check, if the Time-Point already lays in the past:
			GetRealDayTime(&tscur,&timcur);
			if(timts[schedcur]>tscur.tv_sec){
			}else{
				printfc(red,"ERROR");printf(": Scheduled Time lays in the past!\n");
				exit(MAIN_ERR_BAD_CMDLINE);
			}
		}

		// Now Sort them!
		{
			int i,j;
			time_t swap;
			struct tm swapst;

			for(i=0;i<(schedn-1);i++){
				for(j=0;j<schedn-i-1;j++){
					if(timts[j] > timts[j+1]){ //For decreasing order, after Sort:  <
						swap       = timts[j];
						timts[j]   = timts[j+1];
						timts[j+1] = swap;
						//We don't really need a swap of the struct tm array, because we don't use it further on
						//  except for the print out, but it's nice to have ;oP
						//						memcpy(&swapst,&tim[j],sizeof(struct tm));
						//						memcpy(&tim[j],&tim[j+1],sizeof(struct tm));
						//						memcpy(&tim[j+1],&swapst,sizeof(struct tm));
						memcpy(&swapst,tim+j,sizeof(struct tm));
						memcpy(tim+j,tim+(j+1),sizeof(struct tm));
						memcpy(tim+(j+1),&swapst,sizeof(struct tm));
						//Using Bitwise Or makes the variables swap, without using third variable. Just for Info
//						a ^= b ^= a ^= b;
					}
				}
			}
		}



		printf("\nSetting up Execution Schedule for:\n");
		for(schedcur=0;schedcur<schedn;schedcur++)
			printf("\t%04d-%02d-%02d, %02d:%02d:%02d\n",tim[schedcur].tm_year,tim[schedcur].tm_mon,tim[schedcur].tm_mday,tim[schedcur].tm_hour,tim[schedcur].tm_min,tim[schedcur].tm_sec);

		printf("\nCommand:\n\t");
			for(i=0;i<argc;i++)
				printf(" %s", *(argstart+i));
			printf("\n");
		}


	struct timespec remainingdelay;
	remainingdelay.tv_nsec = 0;
	// We approximate the waiting over several Intervals, that are getting smaller and closer,
	// until we (hopefully) get a last interval of few seconds.
	// So we can achieve more precision and won't jump to far.
	#define YEAR_IN_SECONDS ((time_t)31536000)
	#define MONTH_IN_SECONDS ((time_t)2678400)
	#define DAY_IN_SECONDS ((time_t)86400)
	#define HOUR_IN_SECONDS ((time_t)3600)
	#define MIN10_IN_SECONDS ((time_t)600)
	#define MIN_IN_SECONDS ((time_t)60)
	#define SEC10_IN_SECONDS ((time_t)10)

	for(schedcur=0;schedcur<schedn;schedcur++){
		{
		time_t timdiff;
		timdiff=timts[schedcur]-tscur.tv_sec;
		while(timdiff>YEAR_IN_SECONDS){
			//Wait
			remainingdelay.tv_sec = timts[schedcur] - tscur.tv_sec - MONTH_IN_SECONDS;//minus 1 Month (31 Days)
			if(remainingdelay.tv_sec < 0){//To regulate Border Cases. Contemplate something like when we enter here on 31. Dec and aim for following 1. Jan...
				remainingdelay.tv_sec = (time_t)((remainingdelay.tv_sec + MONTH_IN_SECONDS) / 2);
			}
			while (nanosleep(&remainingdelay, &remainingdelay)<0){}
			//GetTime Difference
			GetRealDayTime(&tscur,&timcur);
			timdiff=timts[schedcur]-tscur.tv_sec;
		}
		while(timdiff>MONTH_IN_SECONDS){
			remainingdelay.tv_sec = timts[schedcur] - tscur.tv_sec - DAY_IN_SECONDS;//minus 1 Day (24 Hours)
			if(remainingdelay.tv_sec < 0){
				remainingdelay.tv_sec = (time_t)((remainingdelay.tv_sec + DAY_IN_SECONDS) / 2);
			}
			while (nanosleep(&remainingdelay, &remainingdelay)<0){}
			GetRealDayTime(&tscur,&timcur);
			timdiff=timts[schedcur]-tscur.tv_sec;
		}
		while(timdiff>DAY_IN_SECONDS){
			remainingdelay.tv_sec = timts[schedcur] - tscur.tv_sec - HOUR_IN_SECONDS;//minus 1 Hour
			if(remainingdelay.tv_sec < 0){
				remainingdelay.tv_sec = (time_t)((remainingdelay.tv_sec + HOUR_IN_SECONDS) / 2);
			}
			while (nanosleep(&remainingdelay, &remainingdelay)<0){}
			GetRealDayTime(&tscur,&timcur);
			timdiff=timts[schedcur]-tscur.tv_sec;
		}
		while(timdiff>HOUR_IN_SECONDS){
			remainingdelay.tv_sec = timts[schedcur] - tscur.tv_sec - MIN10_IN_SECONDS;//minus 10 Minutes
			if(remainingdelay.tv_sec < 0){
				remainingdelay.tv_sec = (time_t)((remainingdelay.tv_sec + MIN10_IN_SECONDS) / 2);
			}
			while (nanosleep(&remainingdelay, &remainingdelay)<0){}
			GetRealDayTime(&tscur,&timcur);
			timdiff=timts[schedcur]-tscur.tv_sec;
		}
		while(timdiff>MIN10_IN_SECONDS){
			remainingdelay.tv_sec = timts[schedcur] - tscur.tv_sec - MIN_IN_SECONDS;//minus 10 Minutes
			if(remainingdelay.tv_sec < 0){
				remainingdelay.tv_sec = (time_t)((remainingdelay.tv_sec + MIN_IN_SECONDS) / 2);
			}
			while (nanosleep(&remainingdelay, &remainingdelay)<0){}
			GetRealDayTime(&tscur,&timcur);
			timdiff=timts[schedcur]-tscur.tv_sec;
		}
		while(timdiff>MIN_IN_SECONDS){
			remainingdelay.tv_sec = timts[schedcur] - tscur.tv_sec - SEC10_IN_SECONDS;
			if(remainingdelay.tv_sec < 0){
				remainingdelay.tv_sec = (time_t)((remainingdelay.tv_sec + SEC10_IN_SECONDS) / 2);
			}
			while (nanosleep(&remainingdelay, &remainingdelay)<0){}
			GetRealDayTime(&tscur,&timcur);
			timdiff=timts[schedcur]-tscur.tv_sec;
			depr()
		}
		while(timdiff>SEC10_IN_SECONDS){
			remainingdelay.tv_sec = timts[schedcur] - tscur.tv_sec - 1;
			if(remainingdelay.tv_sec < 0){
				remainingdelay.tv_sec = (time_t)((remainingdelay.tv_sec + 1) / 2);
			}
			while (nanosleep(&remainingdelay, &remainingdelay)<0){}
			GetRealDayTime(&tscur,&timcur);
			timdiff=timts[schedcur]-tscur.tv_sec;
		}
		while(timdiff>0){
			remainingdelay.tv_sec = 0;
			remainingdelay.tv_nsec = MS_TO_NS(10);
			while (nanosleep(&remainingdelay, &remainingdelay)<0){}
			GetRealDayTime(&tscur,&timcur);
			timdiff=timts[schedcur]-tscur.tv_sec;
		}
		}


//		print_daytime();depr()

		print_daytime();printf(" Starting Execution.\n");
		err=executeDebugTask(argc, argstart);
	}

	#undef YEAR_IN_SECONDS
	#undef MONTH_IN_SECONDS
	#undef DAY_IN_SECONDS
	#undef HOUR_IN_SECONDS
	#undef MIN10_IN_SECONDS
	#undef MIN_IN_SECONDS

	return err;
}






int handleDebug(int argc, char **argstart){
	int err = 0;
	if (argc < 1) {
		printf("To few arguments for:\n");
		//misuse the err as loop-counter.
		for(err=-1;err<argc;err++)
			printf(" %s", *(argstart+err));
		printf("\n");
		return MAIN_ERR_FEW_CMDS;
	}
	if (strcmp(*(argstart), "schedule") == 0){
		err=schedule_task_debug(--argc,++argstart);
	}else{
		err=executeDebugTask(argc, argstart);
		return err;
	}
	return err;
}



