/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#include "ollerus.h"
#include "remainder_extended.h"





//to predefine the void, to be used in the handleHelp
//But better define the complete void down there, well, because its optical better
//to let the handleHelp stand at top
static int get_CMDATTS_cb();
int handleHelp(struct nl80211_state *sktctr, int argc, char **argstart, struct CommandContainer *cmd) {
	int err = 0;
	if (3 <= argc) {
		if (strcmp(*(argstart+1), "print") == 0) {
			if (strcmp(*(argstart+2), "iftype") == 0) {
				printf("\nPossible Infrastructure Types & their enum counts:\n");
				printf("\tNL80211_IFTYPE_UNSPECIFIED: %d\n",NL80211_IFTYPE_UNSPECIFIED);
				printf("\t\tunspecified type, driver decides\n");
				printf("\tNL80211_IFTYPE_ADHOC: %d\n",NL80211_IFTYPE_ADHOC);
				printf("\t\tindependent BSS member\n");
				printf("\tNL80211_IFTYPE_STATION: %d\n",NL80211_IFTYPE_STATION);
				printf("\t\tmanaged BSS member\n");
				printf("\tNL80211_IFTYPE_AP: %d\n",NL80211_IFTYPE_AP);
				printf("\t\taccess point\n");
				printf("\tNL80211_IFTYPE_AP_VLAN: %d\n",NL80211_IFTYPE_AP_VLAN);
				printf("\t\tVLAN interface for access points;\n\t\tVLAN interfaces are a bit special\n\t\tin that they must always be tied to a\n\t\tpre-existing AP type interface.\n");
				printf("\tNL80211_IFTYPE_WDS: %d\n",NL80211_IFTYPE_WDS);
				printf("\t\twireless distribution interface\n");
				printf("\tNL80211_IFTYPE_MONITOR: %d\n",NL80211_IFTYPE_MONITOR);
				printf("\t\tmonitor interface receiving all frames\n");
				printf("\tNL80211_IFTYPE_MESH_POINT: %d\n",NL80211_IFTYPE_MESH_POINT);
				printf("\t\tmesh point\n");
				printf("\tNL80211_IFTYPE_P2P_CLIENT: %d\n",NL80211_IFTYPE_P2P_CLIENT);
				printf("\t\tP2P client\n");
				printf("\tNL80211_IFTYPE_P2P_GO: %d\n",NL80211_IFTYPE_P2P_GO);
				printf("\t\tP2P group owner\n");
				printf("\tNL80211_IFTYPE_P2P_DEVICE: %d\n",NL80211_IFTYPE_P2P_DEVICE);
				printf("\t\tP2P device interface type, this is not a netdev\n\t\tand therefore can't be created in the normal ways,\n\t\tuse the %NL80211_CMD_START_P2P_DEVICE and\n\t\t%NL80211_CMD_STOP_P2P_DEVICE\n\t\tcommands to create and destroy one\n\t\t@NL80211_IFTYPE_MAX: highest interface type number\n\t\tcurrently defined\n");
				printf("\tNL80211_IFTYPE_MAX: %d\n",NL80211_IFTYPE_MAX);
				printf("\t\thighest interface type number currently defined\n");
				printf("\tNUM_NL80211_IFTYPES: %d\n",NUM_NL80211_IFTYPES);
				printf("\t\tnumber of defined interface types\n");
				printf("\n");
				printf("These values are used with the\nNL80211_ATTR_IFTYPE\nto set the type of an interface.\n");
				printf("\n");
			}
			else if (strcmp(*(argstart+2), "cmdatts") == 0) {
				cmd->cmd = NL80211_CMD_GET_SCAN;
				prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
				cmd->nl_msg_flags = NLM_F_ROOT | NLM_F_MATCH;
				cmd->callbackToUse = get_CMDATTS_cb;
				printf("To Compare: NL80211_ATTR_WIPHY_TX_POWER_LEVEL\n\t(int: %d)\n",NL80211_ATTR_WIPHY_TX_POWER_LEVEL);
				printf("\nRequested command:\NL80211_CMD_GET_SCAN\n\t(int: %d)\n\n",NL80211_CMD_GET_SCAN);
				err = send_with_cmdContainer(sktctr, argc, argstart, cmd);
				puts("");
				puts("");
				cmd->cmd = NL80211_CMD_GET_INTERFACE;
				prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
				cmd->nl_msg_flags = NLM_F_ROOT | NLM_F_MATCH;
				cmd->callbackToUse = get_CMDATTS_cb;
				printf("\nRequested command:\NL80211_CMD_GET_INTERFACE\n\t(int: %d)\n\n",NL80211_CMD_GET_INTERFACE);
				err = send_with_cmdContainer(sktctr, argc, argstart, cmd);
				puts("");
				puts("");
	//			cmd->cmd = NL80211_CMD_GET_STATION;
	//			prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	//			prepareAttribute(cmd, NL80211_ATTR_MAC, WhatWeWant.MAC_ConnectedTo);
	//			cmd->nl_msg_flags = NLM_F_ROOT | NLM_F_MATCH;
	//			cmd->callbackToUse = get_CMDATTS_cb;
	//			printf("\nRequested command:\NL80211_CMD_GET_INTERFACE\n\t(int: %d)\n\n",NL80211_CMD_GET_INTERFACE);
	//			err = send_with_cmdContainer(sktctr, argc, argstart, cmd);
			}
	//		else if (strcmp(*(argstart+2), "") == 0) { }
			else {
				printf("Invalid parameter after print: %s\nExpected <iftype | >", *(argstart+2));
				return MAIN_ERR_BAD_CMDLINE;
			}
		}
	}else if(2<=argc){
		if (strcmp(*(argstart+1), "endian") == 0) {
			int i;i=1;
			SwitchEndianAgain:
			switch(system_endianess){
			case ENDIANESS_LITTLE:
				puts("");
				printf("System Endianess is ");
				printfc(yellow,"little");
				printf(" Endian.\n");
				puts("");
				err=0;
				break;
			case ENDIANESS_BIG:
				puts("");
				printf("System Endianess is ");
				printfc(yellow,"big");
				printf(" Endian.\n");
				puts("");
				err=0;
				break;
			case ENDIANESS_UNKNOWN:
				puts("");
				printf("System Endianess is not known...\n");
				printf("Trying to get it again...\n");
				check_system_endianess(&system_endianess);
				if(0<i){
					i--;
					goto SwitchEndianAgain;
				}else{
					printf("Couldn't get the System Endianess for any reason o.O\n\tShouldn't ever occur... -.-\n");
					err=MAIN_ERR_BAD_RUN;
				}
				puts("");
				break;
			default:
				break;
			}
		}else{
			printf("Wrong arguments for:\n");
			//misuse the err as loop-counter.
			for(err=0;err<argc;err++)
				printf(" %s", *(argstart+err));
			printf("\n");
			return MAIN_ERR_BAD_CMDLINE;
		}
	}else{
		printf("To few arguments for:\n");
		//misuse the err as loop-counter.
		for(err=0;err<argc;err++)
			printf(" %s", *(argstart+err));
		printf("\n");
		return MAIN_ERR_FEW_CMDS;
	}
//	err = send_with_cmdContainer(sktctr, argc, argstart, cmd);

	return err;
}


static int get_CMDATTS_cb(struct nl_msg* msg, void* arg) {
	struct nlmsghdr *got_hdr = nlmsg_hdr(msg);
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];
	struct nlattr *got_sta_info[NL80211_STA_INFO_MAX + 1];
	struct nlattr *got_sta_bss[NL80211_STA_BSS_PARAM_MAX + 1];
//	static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
//		[NL80211_STA_INFO_INACTIVE_TIME] = { .type = NLA_U32 },
//		[NL80211_STA_INFO_RX_BYTES] = { .type = NLA_U32 },
//		[NL80211_STA_INFO_TX_BYTES] = { .type = NLA_U32 },
//		[NL80211_STA_INFO_RX_PACKETS] = { .type = NLA_U32 },
//		[NL80211_STA_INFO_TX_PACKETS] = { .type = NLA_U32 },
//		[NL80211_STA_INFO_SIGNAL] = { .type = NLA_U8 },
//		[NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
//		[NL80211_STA_INFO_LLID] = { .type = NLA_U16 },
//		[NL80211_STA_INFO_PLID] = { .type = NLA_U16 },
//		[NL80211_STA_INFO_PLINK_STATE] = { .type = NLA_U8 },
//	};
//	static struct nla_policy bss_policy[NL80211_STA_BSS_PARAM_MAX + 1] = {
//		[NL80211_STA_BSS_PARAM_CTS_PROT] = { .type = NLA_FLAG },
//		[NL80211_STA_BSS_PARAM_SHORT_PREAMBLE] = { .type = NLA_FLAG },
//		[NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME] = { .type = NLA_FLAG },
//		[NL80211_STA_BSS_PARAM_DTIM_PERIOD] = { .type = NLA_U8 },
//		[NL80211_STA_BSS_PARAM_BEACON_INTERVAL] = { .type = NLA_U16 },
//	};

	if (got_attr[NL80211_ATTR_WIPHY_TX_POWER_LEVEL])
		printf("\nTX Power: %u\n",nla_get_u32(got_attr[NL80211_ATTR_WIPHY_TX_POWER_LEVEL]));
	PrintContainingAtts;

	return 0;
}
