/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 * Based on Experience from iw (Johannes Berg)
 */


#define NO_OLLERUS_C_FUNCTIONS




#include "ollerus.h"

//#include <linux/if.h>
#include <net/if.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#include <netlink/object.h>
#include <netlink/utils.h>
#include <netlink/socket.h>
//#include <netlink/route/link.h>
#include <netlink/cache.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <xmlSendEngine.h>

#include "scan_chain.h"
#include "absint.h"
#include "remainder_extended.h"
//#include "nl80211.h"

#include "head/ollerus_extern_functions.h"




//"Eleganter" Control+C Handler
void ctrl_c(){
	printf ("Exiting on User Desire.\n");
	exit (0);
}




//TODO (still... -.-): Something like a Manualprint
/* usage */
void usage(char *name){
  printf ("%s - simple ARP sniffer\n", name);
  printf ("Usage: %s [-i interface] [-l] [-v]\n", name);
  printf ("    -i    interface to sniff on\n");
  printf ("    -l    list available interfaces\n");
  printf ("    -v    print verbose info\n\n");
  exit (1);
}






unsigned char do_debug = 0;
char logatxp=0;//Some Flags/Settings for the Logging-Level of the TX Power Adaption
int expectedId;

struct WLANConnectionData WhatWeWant = {
		.stainf.flags = 0,
};

int nl80211_init_socket(struct nl80211_state *skt){
	int err;
	const int buffsize = 8192;

	skt->nl_sock = nl_socket_alloc();

	if (!skt->nl_sock) {
		fprintf(stderr, "Failed to allocate netlink socket.\n");
		return -ENOMEM;
	}

	nl_socket_set_buffer_size(skt->nl_sock, buffsize, buffsize);

	if (genl_connect(skt->nl_sock)) {
		fprintf(stderr, "Failed to connect to generic netlink.\n");
		err = -ENOLINK; //stdErrorNr: Link has been severed
		goto out_handle_destroy;
	}

	skt->nl80211_id = genl_ctrl_resolve(skt->nl_sock, "nl80211");
	if (skt->nl80211_id < 0) {
		fprintf(stderr, "nl80211 not found.\n");
		err = -ENOENT;
		goto out_handle_destroy;
	}

	return 0;

	out_handle_destroy: nl_socket_free(skt->nl_sock);
	return err;
}

void nl80211_cleanup_socket(struct nl80211_state *state){
	if(state->nl_sock){
		nl_socket_free(state->nl_sock);
		state->nl_sock=NULL;
	}
}

static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
//	printf("Debug-error_handler\n");
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
//	printf("Debug-finish_handler\n");
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
//	printf("Debug-ack_handler\n");
	//--------------------------------------------------------------------------
	//--------------------------------------------------------------------------
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}








static int appendAttributes(struct nl_msg *msg, struct CommandContainer *cmd) {
	if (!cmd->prepatt)
		goto NothingToAppend;
	cmd->prepatt->current = cmd->prepatt->first;
	do {
		switch(cmd->prepatt->current->attype){
		case NL80211_ATTR_IFINDEX:
		case NL80211_ATTR_WIPHY:
		case NL80211_ATTR_WIPHY_TX_POWER_SETTING:
		case NL80211_ATTR_IFTYPE:
		case NL80211_ATTR_WIPHY_TX_POWER_LEVEL:
		case NL80211_ATTR_CHANNEL_WIDTH:
		case NL80211_ATTR_WIPHY_FREQ:
		case NL80211_ATTR_CENTER_FREQ1:
		case NL80211_ATTR_CENTER_FREQ2:
			;
			int *value = (cmd->prepatt->current->attpoint);
//			printf("\ntest: %d\n",*value);
			NLA_PUT_U32(msg, cmd->prepatt->current->attype, *value);
			break;
		case  NL80211_ATTR_MAC:
//			char *mac = cmd->prepatt->current->attpoint;
			NLA_PUT(msg, cmd->prepatt->current->attype, 6, cmd->prepatt->current->attpoint);
			break;
		case NL80211_ATTR_WIPHY_NAME:
		case NL80211_ATTR_IFNAME:
			NLA_PUT(msg, cmd->prepatt->current->attype, 5, cmd->prepatt->current->attpoint);
			break;
		default:
			printfc(red,"\n!!! Not handled Case in appendAttributes!!!\n");
			puts("");
			exit(1);
			break;
		}
					/*Old Construct*/
						//		if (cmd->prepatt->current->attype == NL80211_ATTR_IFINDEX ||
						//			cmd->prepatt->current->attype == NL80211_ATTR_WIPHY ||
						//			cmd->prepatt->current->attype == NL80211_ATTR_WIPHY_TX_POWER_SETTING ||
						//			cmd->prepatt->current->attype == NL80211_ATTR_IFTYPE ||
						//			cmd->prepatt->current->attype == NL80211_ATTR_WIPHY_TX_POWER_LEVEL) {
						//			int *value = (cmd->prepatt->current->attpoint);
						////			printf("\ntest: %d\n",*value);
						//			NLA_PUT_U32(msg, cmd->prepatt->current->attype, *value);
						//		} else if (cmd->prepatt->current->attype == NL80211_ATTR_MAC) {
						////			char *mac = cmd->prepatt->current->attpoint;
						//			NLA_PUT(msg, cmd->prepatt->current->attype, 6, cmd->prepatt->current->attpoint);
						//		} else if (cmd->prepatt->current->attype == NL80211_ATTR_WIPHY_NAME ||
						//				   cmd->prepatt->current->attype == NL80211_ATTR_IFNAME) {
						//			NLA_PUT(msg, cmd->prepatt->current->attype, 5, cmd->prepatt->current->attpoint);
						//		}
		if (cmd->prepatt->current->next == cmd->prepatt->first) //little bit of a workaround to make the loop run the correct number of times
			break; //put the condition with != in the while breaks the loop one iteration to early
		cmd->prepatt->current = cmd->prepatt->current->next;
	} while (1);
	return 0;
	nla_put_failure: fprintf(stderr, "building message failed\n");
	return OPERATION_ERR_STD;
	NothingToAppend:
	return OPERATION_ERR_NOTHING_TO_DO;
}
int prepareAttribute (struct CommandContainer *cmd, enum nl80211_attrs attype, int valpoint) {
	struct preparedAttributes *new = malloc(sizeof(struct preparedAttributes));
	new->attype = attype;
	new->attpoint = valpoint;
	if (!cmd->prepatt) {
		struct AttsToAppend *start = malloc(sizeof(struct AttsToAppend));
		start->first = new;
		start->current = new;
		cmd->prepatt = start;
	} else {
		cmd->prepatt->current->next = new;
		cmd->prepatt->current = new;
	}
	new->next = cmd->prepatt->first;
	return 0;
}
static int cleanPreparedAttributes (struct CommandContainer *cmd) {
	if (!(cmd->prepatt))
		goto NothingToClean;
	if (cmd->prepatt->first->next == cmd->prepatt->first) {
		free(cmd->prepatt->first);
	} else {
		struct preparedAttributes *pre = cmd->prepatt->first;
		cmd->prepatt->current = cmd->prepatt->first->next;
		do {
			free(pre);
			pre = cmd->prepatt->current;
			cmd->prepatt->current = pre->next;
		} while (pre->next != cmd->prepatt->first);
		free(pre);
	}
	free(cmd->prepatt);
	cmd->prepatt = NULL;

	return 0;
	NothingToClean:
	return OPERATION_ERR_NOTHING_TO_DO;
}
//int debuggothrough4 = 0;
int send_with_cmdContainer(struct nl80211_state *sktctr, int argc, char **argv, struct CommandContainer *cmd) {
//debuggothrough4++;
//printf("Debug-Times through send_with_cmdContainer: %d\n\n",debuggothrough4);
	//You have to pass this Function a CommandContainer-struct <- containing nl80211_commands and flags (for nl_msg)
	struct nl_msg *msg;
	int err = 0;
	struct nl_cb *socket_cb;

//allocate Space for message, with error-detection
	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		return 2;
	}

// Would be alternative to the allocate, set and append way, i used down on, but not that nice with this multiple-sets
//	nl_socket_modify_cb(sktctr->nl_sock, NL_CB_VALID, do_debug ? NL_CB_DEBUG : NL_CB_CUSTOM, cmd->callbackToUse, NULL);

	// setup the message
	genlmsg_put(msg, 0, 0, sktctr->nl80211_id, 0, cmd->nl_msg_flags, cmd->cmd, 0);

	err = appendAttributes(msg,cmd);
	if (err == OPERATION_ERR_STD) {
		goto nla_put_failure;
	}
	cleanPreparedAttributes (cmd);

	socket_cb = nl_cb_alloc(do_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	if (!socket_cb) {
		fprintf(stderr, "failed to allocate callback struct\n");
		err = 2;
		goto out_free_msg;
	}

	static struct CallbackArgPass cbargs;
	cbargs.err = &err;
	cbargs.ArgPointer=cmd->callbackargpass;
	nl_cb_set(socket_cb, NL_CB_VALID, NL_CB_CUSTOM, cmd->callbackToUse, &cbargs);
	nl_socket_set_cb(sktctr->nl_sock, socket_cb);

	nl_cb_err(socket_cb, NL_CB_CUSTOM, error_handler, &err);
	nl_cb_set(socket_cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, &err);
	nl_cb_set(socket_cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &err);

//	printf("\ntest %d\n",err);
	err = nl_send_auto_complete(sktctr->nl_sock, msg);
	if (err < 0) {
		fprintf(stderr, "failed to send message!");
		goto out;
	}
//	printf("\ntest2 %d\n",err);

	err = 1;
	while (err > 0) {
//		printf("Debug: Receive Message\n");
		nl_recvmsgs_default(sktctr->nl_sock);
//		printf("Err:%d\n",err);
	}
//	printf("\ntest3 %d\n",err);

	out:
	out_free_msg:
	nlmsg_free(msg);
	return err;
	nla_put_failure:
	nlmsg_free(msg);
	fprintf(stderr, "building message failed\n");
	return 2;
}




char ansi_escape_use;
char system_endianess=ENDIANESS_UNKNOWN;
int ifIndex;
char **args;
//int main(int argc, char *argv[]) {
int main(int argc, char **argv) {
	system("reset");
	system("clear");
	int err;

	/* setup signal handler so Control-C will gracefully exit */
	signal(SIGINT, ctrl_c);

	// Get the System-Endianess
	check_system_endianess(&system_endianess);

	args=argv;
	//Just to set up, if Ollerus should use ANSI Escape on the Output Console. For Things like Colored Output...
	//defined as a macro in remainder.h
	SET_ansi_escape_use;
	//----------------------------------------------------------------------------------------------

//=======================================================================================================================================
//=======================================================================================================================================
//=======================================================================================================================================
//				Temporary Testing Section
//=======================================================================================================================================
//=======================================================================================================================================
#ifdef DEBUG
#ifndef RELEASE_VERSION //Just to get sure... ;o) that nothing got forgotten within the final Build
//=======================================================================================================================================









//exit(1);





//=======================================================================================================================================
#endif
#endif
//=======================================================================================================================================
//=======================================================================================================================================
//				End  -  Temporary Testing Section
//=======================================================================================================================================
//=======================================================================================================================================
//=======================================================================================================================================

	struct CommandContainer cmd;
	cmd.prepatt = NULL;
	char SingleRun = 1;
	unsigned long rundelay;// Do it with a long, because the time-struct and function, which uses it are also implemented with long

	//allocate socket, generic connect, put driver ID in Container
	struct nl80211_state sktctr; //contains the nl_socket
	err = nl80211_init_socket(&sktctr);

	//Store the driver ID in global Var. Makes it easy and smooth with Callbacks
	expectedId = sktctr.nl80211_id;


	argc--;
	char **argstartInitial;
	int argcInitial;
	int aktargindex;
	char **argstart;
	argstart = argv + 1;

	if(argc>0) {
	if (strcmp(*argstart, "dev") == 0) {
		if(argc>1){
			WhatWeWant.interfacename = *(argstart + 1);
			argstart += 2;
			argc -= 2;
			if(argc>0) {
				if(strcmp(*argstart, "cmds") == 0) {
					ifIndex = if_nametoindex(WhatWeWant.interfacename);
					argc -= 1;
					if(argc>0)
						argstart += 1;
					//Prints out the nl80211_cmds which your driver supports
					PrintSupportedCmds;
				}
			} else {
				printf("Nice, you gave me the device: %s\nBut what shall i do with it?\n", WhatWeWant.interfacename);
				return MAIN_ERR_FEW_CMDS;
			}
		} else {
			printf("To few Arguments!\n");
			return MAIN_ERR_FEW_CMDS;
		}
	} else {
		//use this as default, if no device is passed...
		printfc(green,"Ollerus Info: ");
		printf("No Device passed. I check for present WLAN-Devices and take the first found.\n");
//		WhatWeWant.interfacename = "wlan0";
		WhatWeWant.interfacename=malloc(IFNAMSIZ);
		{//A scope for this automatic Interface-Detection and WLAN Selection, so that the structs are freed after usage
			struct AbsintInterfaces ifcollect;
			uintptr_t ifps;
			err = getInterfacesAndAdresses(&(ifcollect.ifc),&ifps,&(ifcollect.ifacestart),&(ifcollect.ifmacstart),&(ifcollect.ifaddrstart));

			struct AbsintInterfacesWLAN wifcollect;
			wifcollect.wlanc=0;
			wifcollect.wlanidx=NULL;
			select_AI_wlan_dev(&wifcollect,&ifcollect,&sktctr,&cmd);
			if(wifcollect.wlanc==0){
				if(ifcollect.ifc==0){
					printfc(red,"ERROR: ");printf("This Hardware-Plattform doesn't have any Network Interfaces!\n\tWhy did you call me? o.O ;oP\n");
					exit(MAIN_ERR_BAD_RUN);
				}
				memcpy(WhatWeWant.interfacename,(ifcollect.ifacestart)[0],IFNAMSIZ);
			}else{
				memcpy(WhatWeWant.interfacename,(ifcollect.ifacestart)[(wifcollect.wlanidx)[0]],IFNAMSIZ);
			}
		}
	}
	ifIndex = if_nametoindex(WhatWeWant.interfacename);

	// if(argc>0) is here already secured from the above...
	if (strcmp(*argstart, "do") == 0) {
		SingleRun = 0;
		if(argc>1){
			argstart += 1;
				/*
				 * check if the given string after "do" is a valid integer number
				 * also consider the range check after strtol further down
				 * (done with the errno, set by the function)
				 */
				err = 0;
				char *strtocheck;
				strtocheck = *argstart;
				// Handle negative numbers
				if (*strtocheck== '-')
					goto InvalidInterval;
				// Handle empty string not necessary. Already ensured...
				// Check for non-digit chars
				while (*strtocheck)
				{
					if (!isdigit(*strtocheck))
						goto InvalidInterval;
					else
						++strtocheck;
				}
			//char *end;// Not really needed here inside the strtoul
			errno = 0;
			rundelay = strtoul(*argstart, NULL, 10);
			if (errno!=0)// Especially == EINVAL or ERANGE
				goto InvalidInterval;
			goto ValidInterval;//remember that this only procs if before no error-check thrown you behind.
			InvalidInterval:
			err=-1;
			ValidInterval:
			if (err==-1) {
				printf("Pleeease give me a valid number for the delay-Interval after the 'do' argument.\nI would like to get a positive number in the Range of Long.\n");
				return MAIN_ERR_BAD_CMDLINE;
			}
			argstart += 1;
			argc -= 2;
			if(argc>0) {
			} else {
				printf("Aaaalrighty then! The Loop stands with a delay of %d ms.\nBut shall i do something while looping or just squeeze the Hell out of the CPU?\nYou gave me nothing to do...\n", rundelay);
				return MAIN_ERR_FEW_CMDS;
			}
		} else {
			printf("To few Arguments!\n");
			return MAIN_ERR_FEW_CMDS;
		}
	}


	argstartInitial=argstart;
	argcInitial=argc;
	RunAgain:
	if (strcmp(*(argstart), "set") == 0) {
		err = handleSet(&sktctr, argc, argstart, &cmd);
		if (err < 0) {
			switch (err) {
			case -16:
				printf("\nCan't Execute! Device busy.\nError: %d\nERRNO: %d\n\n",err,errno);
				break;
			default:
				printf("\nInvalid Value!\nKernel can't work with it!\nError: %d\nERRNO: %d\n\n",err,errno);
				break;
			}
		} else {
			switch (err) {
			case 0:
				return 0;
				break;
			case MAIN_ERR_BAD_CMDLINE:
				printf("\nInvalid Value!\nCouldn't put it in a message!\n\n");
				return MAIN_ERR_BAD_CMDLINE;
				break;
			case MAIN_ERR_FEW_CMDS:
//				printf("\nNot enough Arguments after »set« passed\n\n");
				return MAIN_ERR_FEW_CMDS;
				break;
			default:
				fprintf(stderr, "\nUnsupported error-code from the set-Handler delivered!\n\n");
				return MAIN_ERR_STD;
				break;
			}
		}
	} else if (strcmp(*(argstart), "help") == 0) {
		err = handleHelp(&sktctr, argc, argstart, &cmd);
		if (err < 0) {
			printf("\nUnexpected Error on help!\n\n");
			return err;
		} else {
			switch (err) {
			case 0:
				return 0;
				break;
			case MAIN_ERR_BAD_CMDLINE:
//				printf("\n   \n\n");
				return MAIN_ERR_BAD_CMDLINE;
				break;
			case MAIN_ERR_FEW_CMDS:
//				printf("\nNot enough Arguments after »help« passed\n\n");
				return MAIN_ERR_FEW_CMDS;
				break;
			default:
				fprintf(stderr, "\nUnsupported error-code from the help-Handler delivered!\n\n");
				return MAIN_ERR_STD;
				break;
			}
		}
	} else if (strcmp(*(argstart), "son") == 0) {//self organizing network
		err = handleSON(&sktctr, &argc, argstart, &cmd);
		if (err < 0) {
			printf("\nUnexpected Error on Self organizing Network Function!\n\n");
			return err;
		} else {
			switch (err) {
			case 0:
				return 0;
				break;
			case MAIN_ERR_BAD_CMDLINE:
//				printf("\n   \n\n");
				return MAIN_ERR_BAD_CMDLINE;
				break;
			case MAIN_ERR_FEW_CMDS:
//				printf("\nNot enough Arguments after »help« passed\n\n");
				return MAIN_ERR_FEW_CMDS;
				break;
			default:
				fprintf(stderr, "\nUnsupported error-code from the selforganizing-Handler delivered!\n\n");
				return MAIN_ERR_STD;
				break;
			}
		}
	} else if (strcmp(*(argstart), "absint") == 0) {//Abstract Interface
		//Pass the Arguments from BEHIND the "absint"
		argc--;
		argstart++;
		err = absint(argc, argstart,&sktctr,&cmd);
		if (err < 0) {
			printf("\nUnexpected Error on Abstract Interface!\n");
			printf("\nOllerus-AbsInt: Exiting with Error-Code: %d\n\n",err);
			return err;
		} else {
			switch (err) {
			case 0:
				printf("Ollerus-AbsInt: Exiting with Error-Code: %d\n\n",err);
				return 0;
				break;
			case MAIN_ERR_BAD_CMDLINE:
//				printf("\n   \n\n");
				printf("Ollerus-AbsInt: Exiting with Error-Code: %d\n\n",MAIN_ERR_BAD_CMDLINE);
				return MAIN_ERR_BAD_CMDLINE;
				break;
			case MAIN_ERR_NONE:
				printf("Ollerus-AbsInt: Exiting properly without any Error. (Error-Code: %d)\n\n",err);
				return 0;
				break;
			default:
//				fprintf(stderr, "\nUnsupported error-code from the AbstractInterface-Handler delivered!\n\n");
				printf("Ollerus-AbsInt: Exiting with Error-Code: %d\n\n",MAIN_ERR_STD);
				return MAIN_ERR_STD;
				break;
			}
		}
	} else if (strcmp(*(argstart), "debug") == 0) {//Debugging or Monitoring
		argc--;
		argstart++;
		err = handleDebug(argc, argstart);
		if (err < 0) {
			printf("\nUnexpected Error on Debugging Function!\n\n");
			return err;
		} else {
			switch (err) {
			case 0:
				return 0;
				break;
			case MAIN_ERR_BAD_CMDLINE:
//				printf("\n   \n\n");
				return MAIN_ERR_BAD_CMDLINE;
				break;
			case MAIN_ERR_FEW_CMDS:
//				printf("\nNot enough Arguments after »help« passed\n\n");
				return MAIN_ERR_FEW_CMDS;
				break;
			case NETWORK_ERR_NO_CONNECTION:
				printf("Couldn't set up Network Connection. Terminating Program.\n\n");
				return NETWORK_ERR_NO_CONNECTION;
				break;
			default:
				fprintf(stderr, "\nUnsupported error-code from the debug-Handler delivered!\n\n");
				return MAIN_ERR_STD;
				break;
			}
		}
	} else for (aktargindex=0;aktargindex<argc;aktargindex++) {
		if (strcmp(*(argstart+aktargindex), "link") == 0) {
			//intercept the "db get", to not do an own link
			if (argc > (aktargindex+1)) {
				 if ((strcmp(*(argstart+aktargindex+1), "db") == 0)) {//For database-Access
					aktargindex++;
					if (argc == (aktargindex+1)) {
						printf("\nLast argument for >Database-Access< is missing\nShall i read from or send to Database?\n");
						return MAIN_ERR_FEW_CMDS;
					} else {
						aktargindex++;
						if ((strcmp(*(argstart+aktargindex), "send") == 0)) {
							/*
							 * Do nothing.
							 * This check here just to catch the "Invalid Argument" already here.
							 * The real send Operation happens somewhere down.
							 */
							aktargindex-=2;//Compensate the (in this case, on this place) wrong aktargindex++ above.
						} else if ((strcmp(*(argstart+aktargindex), "get") == 0)) {
//								Put function here, to read the scan from the database
//								Evtl. more commandline multiplex, for request
							printf("\nGet Data from Database: scan\n");
//							So do the Stuff here and then bypass the "standart" scan:
							goto NoOwnLink;
						}
						else {
							printf("\nInvalid argument for >database-Access<.\n");
//								aktargindex--;
							return MAIN_ERR_BAD_CMDLINE;
						}
					}
				}
			}

			//Do all the link stuff...
			cmd.identifier=0;
			err = handleLink(&sktctr, &argc, argstart, &cmd);


			//Get applied transmission power
			//TODO: When someday it is implemented in cfg80211, better do it over Netlink
			//TODO: Do something without ioctl. Now exclude this old sh...
			if (!WhatWeWant.owntranspower){ //If owntranspower == 0. In other words: if it wasn't set till now.
				//ioctl_get_txpower(&WhatWeWant.owntranspower);
			}


		//Output - Console
			link_print_console ();

			if (argc > (aktargindex+1)) {//No && check here, because in the second check you have to dereference the pointer. Could lead to an segmentation fault
				if ((strcmp(*(argstart+aktargindex+1), "db") == 0)) {//For database-Access
					aktargindex++;
					/*
					 * Not longer necessary here, already secured above
					if (argc == (aktargindex+1)) {
						printf("\nLast argument for >Database-Access< is missing\nShall i read from or send to Database?\n");
						return MAIN_ERR_FEW_CMDS;
					} else {*/
						aktargindex++;
						if ((strcmp(*(argstart+aktargindex), "send") == 0)) {
							//XML Stuff excluded...
//							xmlSend2Server();
							printf("\nLink-Data was sent to Database.\n\n");
						}
						/*
						 * Not longer necessary here, already secured above
						  else if ((strcmp(*(argstart+aktargindex), "get") == 0)) {
//								Put function here, to read the link from the database
//								Evtl. more commandline multiplex, for request
							printf("\nGet Data from Database: link\n");
						}
						*
						 * Not longer necessary here, already secured above
						else {
							printf("\nInvalid argument for >database-Access<.\n");
//							aktargindex--;
							return MAIN_ERR_BAD_CMDLINE;
						}
					}*/
				}//Here you could place additional "link" arguments with cascading if(strcmp()) checks
				else {aktargindex--;}//To compensate the wrong ++ above and do the normal link-print, if a simple link is requested plus something other like a scan.
			}
			NoOwnLink:
			printf("\n");
		} else if (strcmp(*(argstart+aktargindex), "scan") == 0) {
			//intercept the "db get", to not do an own scan
			if (argc > (aktargindex+1)) {
				 if ((strcmp(*(argstart+aktargindex+1), "db") == 0)) {//For database-Access
					aktargindex++;
					if (argc == (aktargindex+1)) {
						printf("\nLast argument for >Database-Access< is missing\nShall i read from or send to Database?\n");
						return MAIN_ERR_FEW_CMDS;
					} else {
						aktargindex++;
						if ((strcmp(*(argstart+aktargindex), "send") == 0)) {
							/*
							 * Do nothing.
							 * This check here just to catch the "Invalid Argument" already here.
							 * The real send Operation happens somewhere down.
							 */
							aktargindex-=2;//Compensate the (in this case, on this place) wrong aktargindex++ above.
						} else if ((strcmp(*(argstart+aktargindex), "get") == 0)) {
//								Put function here, to read the scan from the database
//								Evtl. more commandline multiplex, for request
							printf("\nGet Data from Database: scan\n");
//							So do the Stuff here and then bypass the "standart" scan:
							goto NoOwnScan;
						}
						else {
							printf("\nInvalid argument for >database-Access<.\n");
//								aktargindex--;
							return MAIN_ERR_BAD_CMDLINE;
						}
					}
				}
			}

			//Scan for available SSIDs
			scan_around(&sktctr, &argc, (argstart+aktargindex), &cmd, &err);


		//Output
			if (argc > (aktargindex+1)) {//No && check here, because in the second check you have to dereference the pointer. Could lead to an segmentation fault
				aktargindex++;
				if ((strcmp(*(argstart+aktargindex), "search") == 0)) {
					if (argc == (aktargindex+1)) {
						printf("\nLast argument for >scan search< is missing\nFor which SSID do you want to search?\nHere you have EVERYTHING:!!!\n");
//						return MAIN_ERR_FEW_CMDS;
						//Just let it print out everything in this case
						goto CompleteScanPrint;
					} else {
						aktargindex++;
						err = scan_chain_console_print_byssid(chainstart, *(argstart+aktargindex));
						goto NoCompleteScanPrint;
					}
				}//Here you could place additional "scan" arguments with cascading if(strcmp()) checks
				else {aktargindex--;}//To compensate the wrong ++ above and do the normal scan-print, if a simple scan is requested plus something other like a link.
			}
			CompleteScanPrint:
			err = scan_chain_console_print(chainstart);
			NoCompleteScanPrint:
			switch (err) {
			case 0:
				if (argc > (aktargindex+1)) {
					 if ((strcmp(*(argstart+aktargindex+1), "db") == 0)) {//For database-Access
						aktargindex++;
						 /*
						  * Not longer needed here, already intercepted above.
						if (argc == (aktargindex+1)) {
							printf("\nLast argument for >Database-Access< is missing\nShall i read from or send to Database?\n");
							return MAIN_ERR_FEW_CMDS;
						} else {*/
							aktargindex++;
							if ((strcmp(*(argstart+aktargindex), "send") == 0)) {
								chainstart->current = chainstart->start;
								chainstart->currentcnt = 1;
								while (1) {//(start->currentcnt <= start->count) {
									//Better to do the check inside the loop, to catch the right break-point
									//XML Stuff excluded...
//									xmlSend2Server();
									if (chainstart->currentcnt >= chainstart->count)
											break;
									if (!(chainstart->current->next))// && (chainstart->currentcnt < start->count))
											return 1;
									chainstart->current = chainstart->current->next;
									chainstart->currentcnt++;
						//			if (chainstart->currentcnt >= 1)
						//								break;
								}
							printf("\nScan-Data was sent to Database.\n\n");
							}
							/*
							 * Not longer needed here, already intercepted above.
							else if ((strcmp(*(argstart+aktargindex), "get") == 0)) {
//								Put function here, to read the link from the database
//								Evtl. more commandline multiplex, for request
								printf("\nGet Data from Database: scan\n");
							}
							else {
								printf("\nInvalid argument for >database-Access<.\n");
//								aktargindex--;
								return MAIN_ERR_BAD_CMDLINE;
							}
						}*/
					}
				}
				break;
			case STRUCT_ERR_DMG:
				fprintf(stderr, "\nScan-Chain is damaged!\nThere is a Gap in the Pointer-Chain\n(Count-Value would lead a longer way...)\n");
				break;
			case STRUCT_ERR_NOT_EXIST:
				printf("\nThere is no Scan-Chain\ni.e. chainstart doesn't Point to anything - Not even to the chain_start struct.\n");
				break;
			case STRUCT_ERR_INCOMPLETE:
				//Just to notify:
				//printf("\nThere was not even one Element to print inside Chain.");
				break;
			default:
				fprintf(stderr, "Undefined Error-Code from scan_chain_console_print!");
				break;
			}
			err = free_scan_chain(chainstart);
			switch (err) {
			case 0:
				break;
			case STRUCT_ERR_DMG:
				fprintf(stderr, "\nScan-Chain is damaged!\nThere is a Gap in the Pointer-Chain\n(Count-Value would lead a longer way...)\nSo maybe i couldn't cleanup everything.\n");
				break;
			case STRUCT_ERR_NOT_EXIST:
//		 		printf("Just to note: Nothing to clean.\nOr something broken, chainstart doesn't Point to chain_start struct");
				break;
			default:
				fprintf(stderr, "Undefined Error-Code from free_scan_chain!");
				break;
			}
			NoOwnScan:
			printf("\n");
		} else { printf("\nUnsupported Command!\n\n"); }
	}
	} else {
		printf("TODO: Info-Section\n");
	}





	if (!SingleRun) {
		struct timespec remainingdelay;
		remainingdelay.tv_sec = rundelay/1000;
		remainingdelay.tv_nsec = (rundelay%1000)*1000;
		do {
			err = nanosleep(&remainingdelay, &remainingdelay);
		} while (err<0);
		printf(ANSI_COLOR_BLUE);
		printf("\n\n---------------------------------------------\n|");
		printf(ANSI_COLOR_RED);
		printf("\t\tNext Iteration\t\t    ");
		printf(ANSI_COLOR_BLUE);
		printf("|\n|___________________________________________|\n");
		printf(ANSI_COLOR_RESET);
		printf("\n");
		argc = argcInitial;
		argstart = argstartInitial;
		WhatWeWant.owntranspower=0;//Because of the check beforce ioctl-call...
							//I know, it is a bit of workaround hack -.-
		goto RunAgain;
	}




	nl80211_cleanup_socket(&sktctr);


	//Output
//	printf("\n");
//	for (aktargindex=0;aktargindex<argc;aktargindex++) {
//		if (strcmp(*(argstart+aktargindex), "link") == 0) {
//		} else if (strcmp(*(argstart+aktargindex), "scan") == 0) {
//		}
//	}



//    err = fileOperation();
    //TODO: Error Handling
//    err = fileOperationChained();
    //TODO: Error Handling
	pthread_exit(NULL);
	printf("\nOllerus: Exiting with Error-Code: %d\n",err);
	return 0;
}




#undef NO_OLLERUS_C_FUNCTIONS
