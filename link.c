/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#include "ollerus.h"



//Not finished yet, waiting for compatible wlan-chip/driver
int Do_CMD_GET_SURVEY_cb(struct nl_msg *msg, void *arg)
{
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];
	struct genlmsghdr *genlhdr = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *survinf[NL80211_SURVEY_INFO_MAX + 1];
	static struct nla_policy survey_policy[NL80211_SURVEY_INFO_MAX + 1] = {
		[NL80211_SURVEY_INFO_FREQUENCY] = { .type = NLA_U32 },
		[NL80211_SURVEY_INFO_NOISE] = { .type = NLA_U8 },
	};
	char dev[20];

	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(genlhdr, 0), genlmsg_attrlen(genlhdr, 0), NULL);

	if_indextoname(nla_get_u32(got_attr[NL80211_ATTR_IFINDEX]), dev);
	printf("Survey data from %s\n", dev);

	if (!got_attr[NL80211_ATTR_SURVEY_INFO]) {
		fprintf(stderr, "survey data missing!\n");
		return NL_SKIP;
	}

	if (nla_parse_nested(survinf, NL80211_SURVEY_INFO_MAX, got_attr[NL80211_ATTR_SURVEY_INFO], survey_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	if (survinf[NL80211_SURVEY_INFO_FREQUENCY])
		printf("\tfrequency:\t\t\t%u MHz%s\n",
			nla_get_u32(survinf[NL80211_SURVEY_INFO_FREQUENCY]),
			survinf[NL80211_SURVEY_INFO_IN_USE] ? " [in use]" : "");
	if (survinf[NL80211_SURVEY_INFO_NOISE])
		printf("\tnoise:\t\t\t\t%d dBm\n",
			(int8_t)nla_get_u8(survinf[NL80211_SURVEY_INFO_NOISE]));
	if (survinf[NL80211_SURVEY_INFO_CHANNEL_TIME])
		printf("\tchannel active time:\t\t%llu ms\n",
			(unsigned long long)nla_get_u64(survinf[NL80211_SURVEY_INFO_CHANNEL_TIME]));
	if (survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY])
		printf("\tchannel busy time:\t\t%llu ms\n",
			(unsigned long long)nla_get_u64(survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_BUSY]));
	if (survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY])
		printf("\textension channel busy time:\t%llu ms\n",
			(unsigned long long)nla_get_u64(survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_EXT_BUSY]));
	if (survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_RX])
		printf("\tchannel receive time:\t\t%llu ms\n",
			(unsigned long long)nla_get_u64(survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_RX]));
	if (survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_TX])
		printf("\tchannel transmit time:\t\t%llu ms\n",
			(unsigned long long)nla_get_u64(survinf[NL80211_SURVEY_INFO_CHANNEL_TIME_TX]));
	return NL_SKIP;
}







//int debuggothrough2 = 0;
static int Do_CMD_GET_INTERFACE_cb(struct nl_msg* msg, void* arg) {
//	debuggothrough2++;
//	printf("Debug-Times through Do_CMD_GET_INTERFACE_cb: %d\n\n",debuggothrough2);
	struct nlmsghdr *got_hdr = nlmsg_hdr(msg);
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];

	if (got_hdr->nlmsg_type != expectedId) {
		// what is this??
		return NL_STOP;
	}

//    struct genlmsghdr *gnlh = (struct genlmsghdr*) nlmsg_data(got_hdr);
	struct genlmsghdr *gnlh = nlmsg_data(got_hdr);

	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);


	if (got_attr[NL80211_ATTR_IFTYPE])
		WhatWeWant.type = nla_get_u32(got_attr[NL80211_ATTR_IFTYPE]);

	if (got_attr[NL80211_ATTR_WIPHY_FREQ])
		WhatWeWant.frequency = nla_get_u32(got_attr[NL80211_ATTR_WIPHY_FREQ]);
//    nla_get_u32(bss[NL80211_BSS_FREQUENCY])

	return 0;
}

//int debuggothrough1 = 0;
static int Do_CMD_GET_SCAN_connected_cb(struct nl_msg* msg, void* arg) {
//	debuggothrough1++;
//	printf("Debug-Times through Do_CMD_GET_SCAN_cb: %d\n\n",debuggothrough1);
	struct nlmsghdr *got_hdr = nlmsg_hdr(msg);
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];
	struct nlattr *got_bss[NL80211_BSS_MAX + 1];
	static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
			[NL80211_BSS_TSF] = { .type = NLA_U64 },
			[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
			[NL80211_BSS_BSSID] = { },
			[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
			[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
			[NL80211_BSS_INFORMATION_ELEMENTS] = { },
			[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
			[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
			[NL80211_BSS_STATUS] = { .type = NLA_U32 }, };
//	char device[sizeof(WhatWeWant.interfacename)];
	char device[IFNAMSIZ];

	if (got_hdr->nlmsg_type != expectedId) {
		// what is this??
		return NL_STOP;
	}

//    struct genlmsghdr *gnlh = (struct genlmsghdr*) nlmsg_data(got_hdr);
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);

	if (!got_attr[NL80211_ATTR_BSS]) {
		fprintf(stderr, "bss info missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(got_bss, NL80211_BSS_MAX, got_attr[NL80211_ATTR_BSS],
			bss_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	if (!got_bss[NL80211_BSS_BSSID]) {
//		fprintf(stderr, "No BSSID delivered!\n");
		return NL_SKIP;
	}
	if (!got_bss[NL80211_BSS_STATUS]) {
//		fprintf(stderr, "Kind of status not specified!\n");
		return NL_SKIP;
	}

	// All the snprintf-stuff is to get sure that everything behind the end of string is zero and not something random
//	char seconddevice[sizeof(WhatWeWant.interfacename)];
	char seconddevice[sizeof(device)];
//	memset(seconddevice,0,sizeof(seconddevice));
	if_indextoname(nla_get_u32(got_attr[NL80211_ATTR_IFINDEX]), device);
	snprintf(seconddevice, sizeof(device), "%s", device);
//	memset(device,0,sizeof(device));
	snprintf(device, sizeof(device), "%s", WhatWeWant.interfacename);
	if (strcmp(device,seconddevice)) {
		fprintf(stderr, "Delivered Information is not about the enquired Interface!\n");
		return NL_SKIP;
	}

	/*
	 * Output
	 * Write the stuff in the Storage-struct
	 */
	if (got_attr[NL80211_ATTR_IFTYPE])
		WhatWeWant.type = nla_get_u32(got_attr[NL80211_ATTR_IFTYPE]);

	if (got_attr[NL80211_ATTR_WIPHY_FREQ])
		WhatWeWant.frequency = nla_get_u32(got_attr[NL80211_ATTR_WIPHY_FREQ]);
	if (got_bss[NL80211_BSS_FREQUENCY])
		WhatWeWant.frequency = nla_get_u32(got_bss[NL80211_BSS_FREQUENCY]);
//    nla_get_u32(bss[NL80211_BSS_FREQUENCY])

	if (got_bss[NL80211_BSS_BSSID]) {
		memset(WhatWeWant.MAC_ConnectedTo, 0, 20);
		memcpy(WhatWeWant.MAC_ConnectedTo, nla_data(got_bss[NL80211_BSS_BSSID]), 20);
	}
	switch (nla_get_u32(got_bss[NL80211_BSS_STATUS])) {
	case NL80211_BSS_STATUS_ASSOCIATED:
//		if (got_bss[NL80211_BSS_BSSID]) {
//			memset(WhatWeWant.MAC_ConnectedTo, 0, 20);
//			memcpy(WhatWeWant.MAC_ConnectedTo, nla_data(got_bss[NL80211_BSS_BSSID]), 20);
//		}
		break;
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
// Section prepared for future Implementation
// Not finished
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
	case NL80211_BSS_STATUS_AUTHENTICATED:
//		printf("Authenticated with %s (on %s)\n", mac_addr, device);
		return NL_SKIP;
	case NL80211_BSS_STATUS_IBSS_JOINED:
//		printf("Joined IBSS %s (on %s)\n", mac_addr, device);
		break;
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
	default:
		return NL_SKIP;
	}

	/*
	 * Getting SSID:
	 * For Explanation, if someday, someone reads this crazy Stuff:
	 * The SSID stands on a Location where NL80211_BSS_INFORMATION_ELEMENTS leads us to.
	 * There is standing a whole Bunch of Bits and Bytes
	 * At the Beginning we can find the SSID
	 * So got_bss[NL80211_BSS_INFORMATION_ELEMENTS] gives us the all-containing struct out of our struct-array got_bss
	 * With nla_data we get the Payload (the actual Data) out of the struct.
	 * The Pointer, coming from nla_data points to the first Address of the Payload.
	 * On the second Address stands the length of the delivered SSID
	 * On the third Address starts the SSID-String, char-by-char
	 */
	if (got_bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
		char *SSIDPointer = nla_data(got_bss[NL80211_BSS_INFORMATION_ELEMENTS]);
		SSIDPointer++;
		unsigned char SSIDLength = *SSIDPointer;
		//To get sure everything is alright
		if (SSIDLength <= 32
				&& SSIDLength
						<= nla_len(got_bss[NL80211_BSS_INFORMATION_ELEMENTS])
								- 2) {
			memcpy(WhatWeWant.essid, SSIDPointer + 1, SSIDLength);
		} else {
			printf(stderr,
					"Something wrong with SSID: Does not fit inside Payload!\n");
			return NL_SKIP;
		}
	} else {
		printf(stderr, "SSID not present!\n");
		return NL_SKIP;
	}

	return 0;
}




void extract_bitrate(struct StationInfo *dest, struct nlattr *bitrate_attr)//, char *bitrbuffer, int buflen)
{
//	char *pos = bitrbuffer;
	struct nlattr *got_bitrate[NL80211_RATE_INFO_MAX + 1];
	static struct nla_policy rate_policy[NL80211_RATE_INFO_MAX + 1] = {
		[NL80211_RATE_INFO_BITRATE] = { .type = NLA_U16 },
		[NL80211_RATE_INFO_BITRATE32] = { .type = NLA_U32 },
		[NL80211_RATE_INFO_MCS] = { .type = NLA_U8 },
		[NL80211_RATE_INFO_40_MHZ_WIDTH] = { .type = NLA_FLAG },
		[NL80211_RATE_INFO_SHORT_GI] = { .type = NLA_FLAG },
	};

	if (nla_parse_nested(got_bitrate, NL80211_RATE_INFO_MAX, bitrate_attr, rate_policy)) {
		fprintf(stderr, "failed to parse nested rate attributes!");
		return;
	}

	if (got_bitrate[NL80211_RATE_INFO_BITRATE32])
		dest->bitrate = nla_get_u32(got_bitrate[NL80211_RATE_INFO_BITRATE32]);
	else if (got_bitrate[NL80211_RATE_INFO_BITRATE])
		dest->bitrate = nla_get_u16(got_bitrate[NL80211_RATE_INFO_BITRATE]);
//
	if (got_bitrate[NL80211_RATE_INFO_MCS]) {
		dest->MCS = nla_get_u8(got_bitrate[NL80211_RATE_INFO_MCS]);
	} else { dest->MCS = -1; } // "= -1" in signed is equal to "= 255" in unsigned (max Value, independent of Bitwidth)
	if (got_bitrate[NL80211_RATE_INFO_VHT_MCS]) {
		dest->flags = (dest->flags) | STA_INF_VHT_MCS;
		dest->MCS = nla_get_u8(got_bitrate[NL80211_RATE_INFO_VHT_MCS]);
	} else { //erases the flag, don't know if this ever could be necessary. Just to get sure
		dest->flags = (dest->flags) & (~STA_INF_VHT_MCS);
	}
	if (got_bitrate[NL80211_RATE_INFO_40_MHZ_WIDTH])
		dest->width = STA_WIDTH_40_MHZ;
	if (got_bitrate[NL80211_RATE_INFO_80_MHZ_WIDTH])
		dest->width = STA_WIDTH_80_MHZ;
	if (got_bitrate[NL80211_RATE_INFO_80P80_MHZ_WIDTH])
		dest->width = STA_WIDTH_80P80_MHZ;
	if (got_bitrate[NL80211_RATE_INFO_160_MHZ_WIDTH])
		dest->width = STA_WIDTH_160_MHZ;
	if (got_bitrate[NL80211_RATE_INFO_SHORT_GI]) {
		dest->flags = (dest->flags) | STA_INF_SHORT_GI;
	} else { //erases the flag, don't know if this ever could be necessary. Just to get sure
		dest->flags = (dest->flags) & (~STA_INF_SHORT_GI);
	}
	if (got_bitrate[NL80211_RATE_INFO_VHT_NSS])
		dest->VHT_NSS = nla_get_u8(got_bitrate[NL80211_RATE_INFO_VHT_NSS]);
}
//int debuggothrough3 = 0;
static int Do_CMD_GET_STATION_cb(struct nl_msg* msg, void* arg) {
//	debuggothrough3++;
//	printf("Debug-Times through Do_CMD_GET_STATION_cb: %d\n\n",debuggothrough3);
	struct nlmsghdr *got_hdr = nlmsg_hdr(msg);
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];
	struct nlattr *got_sta_info[NL80211_STA_INFO_MAX + 1];
	struct nlattr *got_sta_bss[NL80211_STA_BSS_PARAM_MAX + 1];
	static struct nla_policy stats_policy[NL80211_STA_INFO_MAX + 1] = {
		[NL80211_STA_INFO_INACTIVE_TIME] = { .type = NLA_U32 },
		[NL80211_STA_INFO_RX_BYTES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_BYTES] = { .type = NLA_U32 },
		[NL80211_STA_INFO_RX_PACKETS] = { .type = NLA_U32 },
		[NL80211_STA_INFO_TX_PACKETS] = { .type = NLA_U32 },
		[NL80211_STA_INFO_SIGNAL] = { .type = NLA_U8 },
		[NL80211_STA_INFO_TX_BITRATE] = { .type = NLA_NESTED },
		[NL80211_STA_INFO_LLID] = { .type = NLA_U16 },
		[NL80211_STA_INFO_PLID] = { .type = NLA_U16 },
		[NL80211_STA_INFO_PLINK_STATE] = { .type = NLA_U8 },
	};
	static struct nla_policy bss_policy[NL80211_STA_BSS_PARAM_MAX + 1] = {
		[NL80211_STA_BSS_PARAM_CTS_PROT] = { .type = NLA_FLAG },
		[NL80211_STA_BSS_PARAM_SHORT_PREAMBLE] = { .type = NLA_FLAG },
		[NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME] = { .type = NLA_FLAG },
		[NL80211_STA_BSS_PARAM_DTIM_PERIOD] = { .type = NLA_U8 },
		[NL80211_STA_BSS_PARAM_BEACON_INTERVAL] = { .type = NLA_U16 },
	};

	if (got_hdr->nlmsg_type != expectedId) {
		// what is this??
		return NL_STOP;
	}

//    struct genlmsghdr *gnlh = (struct genlmsghdr*) nlmsg_data(got_hdr);
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));

	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0), genlmsg_attrlen(gnlh, 0), NULL);

	if (!got_attr[NL80211_ATTR_STA_INFO]) {
		fprintf(stderr, "sta stats missing!\n");
		return NL_SKIP;
	}
	if (nla_parse_nested(got_sta_info, NL80211_STA_INFO_MAX, got_attr[NL80211_ATTR_STA_INFO], stats_policy)) {
		fprintf(stderr, "failed to parse nested attributes!\n");
		return NL_SKIP;
	}

	// Output
	if (got_sta_info[NL80211_STA_INFO_TX_BITRATE]) {
		extract_bitrate(&(WhatWeWant.stainf), got_sta_info[NL80211_STA_INFO_TX_BITRATE]);
	}

	if (got_sta_info[NL80211_STA_INFO_RX_BYTES] && got_sta_info[NL80211_STA_INFO_RX_PACKETS]) {
		WhatWeWant.stainf.RX_Bytes = nla_get_u32(got_sta_info[NL80211_STA_INFO_RX_BYTES]);
		WhatWeWant.stainf.RX_Packets = nla_get_u32(got_sta_info[NL80211_STA_INFO_RX_PACKETS]);
	}
	if (got_sta_info[NL80211_STA_INFO_TX_BYTES] && got_sta_info[NL80211_STA_INFO_TX_PACKETS]) {
		WhatWeWant.stainf.TX_Bytes = nla_get_u32(got_sta_info[NL80211_STA_INFO_TX_BYTES]);
		WhatWeWant.stainf.TX_Packets = nla_get_u32(got_sta_info[NL80211_STA_INFO_TX_PACKETS]);
	}
	if (got_sta_info[NL80211_STA_INFO_SIGNAL]) {
		WhatWeWant.stainf.SigLvl = (int8_t)nla_get_u8(got_sta_info[NL80211_STA_INFO_SIGNAL]);
	}

	if (got_sta_info[NL80211_STA_INFO_BSS_PARAM]) {
		if (nla_parse_nested(got_sta_bss, NL80211_STA_BSS_PARAM_MAX,got_sta_info[NL80211_STA_INFO_BSS_PARAM],bss_policy)) {
			fprintf(stderr, "failed to parse nested bss parameters!\n");
		} else {
			if (got_sta_bss[NL80211_STA_BSS_PARAM_CTS_PROT])
				WhatWeWant.stainf.flags = WhatWeWant.stainf.flags | STA_BSS_CTS_PROT;
			else { //resets the flag, don't know if this ever could be necessary. Just to get sure
				WhatWeWant.stainf.flags = WhatWeWant.stainf.flags & (~STA_INF_VHT_MCS);
			}
			if (got_sta_bss[NL80211_STA_BSS_PARAM_SHORT_PREAMBLE])
				WhatWeWant.stainf.flags = WhatWeWant.stainf.flags | STA_BSS_SHORT_PREAMBLE;
			else { //resets the flag, don't know if this ever could be necessary. Just to get sure
				WhatWeWant.stainf.flags = WhatWeWant.stainf.flags & (~STA_INF_VHT_MCS);
			}
			if (got_sta_bss[NL80211_STA_BSS_PARAM_SHORT_SLOT_TIME])
				WhatWeWant.stainf.flags = WhatWeWant.stainf.flags | STA_BSS_SHORT_SLOT_TIME;
			else { //resets the flag, don't know if this ever could be necessary. Just to get sure
				WhatWeWant.stainf.flags = WhatWeWant.stainf.flags & (~STA_INF_VHT_MCS);
			}

		   WhatWeWant.stainf.dtim = nla_get_u8(got_sta_bss[NL80211_STA_BSS_PARAM_DTIM_PERIOD]);
		   WhatWeWant.stainf.beacon = nla_get_u16(got_sta_bss[NL80211_STA_BSS_PARAM_BEACON_INTERVAL]);
		}
	}
	return 0;
}



int handleLink(struct nl80211_state *sktctr, int *argc, char **argstart, struct CommandContainer *cmd) {
/*
 * use the identifier inside the CommandContainer to identify which partial function you want to use,
 * if not all has to be executed.
 * In this case ignore that the identifier actually is a pointer and use it as a
 * flag-holder (for now... maybe, if this function here grows you can use it as a pointer and
 * store more flags at destination, though the size of a pointer should last out...
 * So mind the bitsize which the expected Machine to run on uses for pointers.
 * For now i'd say int is more than enough...)
 * The flag-int is used to determine, which partial function to bypass.
 * flags are defined in ollerus.h
 */
	int bypassflags = (int)(cmd->identifier);
	int err;
	//Here we want to get SSID, MAC, Frequency
	if(bypassflags & BYPASS_HANDLELINK_NL80211_CMD_GET_SCAN)
		goto BypassNL80211_CMD_GET_SCAN;
	cmd->cmd = NL80211_CMD_GET_SCAN;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	cmd->nl_msg_flags = NLM_F_ROOT | NLM_F_MATCH;
//	cmd->nl_msg_flags = NLM_F_DUMP; //Would also do the same
	cmd->callbackToUse = Do_CMD_GET_SCAN_connected_cb;
	err = send_with_cmdContainer(sktctr, *argc, argstart, cmd);
	BypassNL80211_CMD_GET_SCAN:


	//This gives us the Infrastructure-Type
	if(bypassflags & BYPASS_HANDLELINK_NL80211_CMD_GET_INTERFACE)
		goto BypassNL80211_CMD_GET_INTERFACE;
	cmd->cmd = NL80211_CMD_GET_INTERFACE;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	cmd->nl_msg_flags = 0;
	cmd->callbackToUse = Do_CMD_GET_INTERFACE_cb;
	err = send_with_cmdContainer(sktctr, *argc, argstart, cmd);
	BypassNL80211_CMD_GET_INTERFACE:


	//To get Transmission/Station Info
	if(bypassflags & BYPASS_HANDLELINK_NL80211_CMD_GET_STATION)
		goto BypassNL80211_CMD_GET_STATION;
	cmd->cmd = NL80211_CMD_GET_STATION;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	prepareAttribute(cmd, NL80211_ATTR_MAC, WhatWeWant.MAC_ConnectedTo);
	cmd->nl_msg_flags = 0;
	cmd->callbackToUse = Do_CMD_GET_STATION_cb;
	err = send_with_cmdContainer(sktctr, *argc, argstart, cmd);
	BypassNL80211_CMD_GET_STATION:


	//To get the survey info (noise for SNR)
	//Not finished yet, waiting for compatible wlan-chip/driver
	if(bypassflags & BYPASS_HANDLELINK_NL80211_CMD_GET_SURVEY)
		goto BypassNL80211_CMD_GET_SURVEY;
	cmd->cmd = NL80211_CMD_GET_SURVEY;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
//	prepareAttribute(&cmd, NL80211_ATTR_MAC, WhatWeWant.MAC_ConnectedTo);
	cmd->nl_msg_flags = NLM_F_DUMP;
	cmd->callbackToUse = Do_CMD_GET_SURVEY_cb;
	err = send_with_cmdContainer(sktctr, *argc, argstart, cmd);
	BypassNL80211_CMD_GET_SURVEY:


	return err;
}
