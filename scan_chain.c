/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#include "scan_chain.h"

#include "ollerus.h"





int chainappend (struct chain_element *tostore, struct chain_start *self) {
	struct chain_element *new = malloc(sizeof(struct chain_element));
//Don't forget to free all this structs, if you do not need them anymore
//See function free_scan_chain
	memcpy(new, tostore, sizeof(struct chain_element));
	if (self->count == 0) {
		self->start = new;
		self->count = 1;
		self->currentcnt = 1;
	} else {
		(*(self->current)).next = new;
		self->count++;
		self->currentcnt++;
	}
	self->current = new;

	return 0;
}


struct chain_start *chainstart = NULL;

void setupScanChainStart() {
//	struct chain_start chainstart = {
//		.count = 0,
//		.currentcnt = 0,
//		.start = 0,
//		.current = 0,
//		.append = chainappend,
//	};
	chainstart = malloc(sizeof(struct chain_start));
	memset(chainstart, 0, sizeof(struct chain_start));
	chainstart->append = chainappend;
}





int scan_chain_console_print (struct chain_start *start) {
	if (!start)
		goto NoChain;
	printf("<--------------------------------------->");
	if (start->count < 1) {
		printf("\nChain doesn't contain anything!\n");
		goto NoChainElement;
	}
	start->current = start->start;
	start->currentcnt = 1;
	while (1) {//(start->currentcnt <= start->count) {
		//Better to do the check inside the loop, to catch the right break-point
		printf("\n");
		printf("\tEntry #%d", start->currentcnt);
		printf("\n");
		if (start->current->flags & SCAN_CHAIN_ELE_NO_BSS_INF) {
			printf("\tBad Message:\n\tNo BSS Info delivered.\nOr not parsable.");
			printf("\n");
		} else {
			printf("\tSSID: ");
			if (start->current->flags & SCAN_CHAIN_ELE_NO_SSID) {
				printf("No SSID delivered!");
			} else if (start->current->flags & SCAN_CHAIN_ELE_SSID_TO_LONG) {
				printf("Bad SSID delivered: To long!");
			} else {
				printf("%s",start->current->ssid);
			}
			printf("\n");
			printf("\tMAC: ");
			printMAC(start->current->mac, 6);
			printf("\n");
			if (start->current->flags & SCAN_CHAIN_ELE_ASSOCIATED) {
				printf("\t   (Associated)\n");
			}
			if (start->current->flags & SCAN_CHAIN_ELE_AUTHENTICATED) {
				printf("\t   (Authenticated with)\n");
			}
			if (start->current->flags & SCAN_CHAIN_ELE_IBSS_JOINED) {
				printf("\t   (Joined IBSS)\n");
			}
			printf("\tTSF: %llu usec", start->current->tsf);
			printf(" (%llud, %.2lld:%.2llu:%.2llu)",
				start->current->tsf/1000/1000/60/60/24, (start->current->tsf/1000/1000/60/60) % 24,
				(start->current->tsf/1000/1000/60) % 60, (start->current->tsf/1000/1000) % 60);
			printf("\n");
			printf("\tFrequency: %d.%.3d GHz (Channel %d)", (start->current->freq)/1000, (start->current->freq)%1000, ieee80211_frequency_to_channel(start->current->freq));
			printf("\n");
			printf("\tBeacon Interval: %d TUs", start->current->beaconinterval);
			printf("\n");
			printf("\tCapability:");
			if (start->current->flags & SCAN_CHAIN_ELE_CAPA_DMG) {
				switch (start->current->capability & WLAN_CAPABILITY_DMG_TYPE_MASK) {
				case WLAN_CAPABILITY_DMG_TYPE_AP:
					printf(" DMG_ESS");
					break;
				case WLAN_CAPABILITY_DMG_TYPE_PBSS:
					printf(" DMG_PCP");
					break;
				case WLAN_CAPABILITY_DMG_TYPE_IBSS:
					printf(" DMG_IBSS");
					break;
				}
				if (start->current->capability & WLAN_CAPABILITY_DMG_CBAP_ONLY)
					printf(" CBAP_Only");
				if (start->current->capability & WLAN_CAPABILITY_DMG_CBAP_SOURCE)
					printf(" CBAP_Src");
				if (start->current->capability & WLAN_CAPABILITY_DMG_PRIVACY)
					printf(" Privacy");
				if (start->current->capability & WLAN_CAPABILITY_DMG_ECPAC)
					printf(" ECPAC");
				if (start->current->capability & WLAN_CAPABILITY_DMG_SPECTRUM_MGMT)
					printf(" SpectrumMgmt");
				if (start->current->capability & WLAN_CAPABILITY_DMG_RADIO_MEASURE)
					printf(" RadioMeasure");
			} else {
				if (start->current->capability & WLAN_CAPABILITY_ESS)
					printf(" ESS");
				if (start->current->capability & WLAN_CAPABILITY_IBSS)
					printf(" IBSS");
				if (start->current->capability & WLAN_CAPABILITY_CF_POLLABLE)
					printf(" CfPollable");
				if (start->current->capability & WLAN_CAPABILITY_CF_POLL_REQUEST)
					printf(" CfPollReq");
				if (start->current->capability & WLAN_CAPABILITY_PRIVACY)
					printf(" Privacy");
				if (start->current->capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
					printf(" ShortPreamble");
				if (start->current->capability & WLAN_CAPABILITY_PBCC)
					printf(" PBCC");
				if (start->current->capability & WLAN_CAPABILITY_CHANNEL_AGILITY)
					printf(" ChannelAgility");
				if (start->current->capability & WLAN_CAPABILITY_SPECTRUM_MGMT)
					printf(" SpectrumMgmt");
				if (start->current->capability & WLAN_CAPABILITY_QOS)
					printf(" QoS");
				if (start->current->capability & WLAN_CAPABILITY_SHORT_SLOT_TIME)
					printf(" ShortSlotTime");
				if (start->current->capability & WLAN_CAPABILITY_APSD)
					printf(" APSD");
				if (start->current->capability & WLAN_CAPABILITY_RADIO_MEASURE)
					printf(" RadioMeasure");
				if (start->current->capability & WLAN_CAPABILITY_DSSS_OFDM)
					printf(" DSSS-OFDM");
				if (start->current->capability & WLAN_CAPABILITY_DEL_BACK)
					printf(" DelayedBACK");
				if (start->current->capability & WLAN_CAPABILITY_IMM_BACK)
					printf(" ImmediateBACK");
			}
			printf(" (0x%.4x)", start->current->capability);
			printf("\n");
			printf("\tSignal Level: ");
			if (start->current->flags & SCAN_CHAIN_ELE_SIGNAL_LEVEL) {
				printf("%d/100", start->current->signallevel);
			} else {
				printf("%d.%.2d dBm", (start->current->signallevel)/100, (start->current->signallevel)%100);
			}
			printf("\n");
			printf("\tLast seen: %d ms ago", start->current->lastseen);
			printf("\n");
		}
		if (start->currentcnt >= start->count)
			break;
		if (!(start->current->next))// && (start->currentcnt < start->count))
			return STRUCT_ERR_DMG;
		start->current = start->current->next;
		start->currentcnt++;
	}

	printf("<--------------------------------------->");
	printf("\n");
	return 0;

	NoChainElement:
	printf("<--------------------------------------->");
	printf("\n");
	return STRUCT_ERR_INCOMPLETE;

	NoChain:
	return STRUCT_ERR_NOT_EXIST;
}



int scan_chain_console_print_byssid (struct chain_start *start, char *select) {
	if (!start)
		goto NoChain;
	printf("<--------------------------------------->");
	if (start->count < 1) {
		printf("\nChain doesn't contain anything!\n");
		goto NoChainElement;
	}
	start->current = start->start;
	start->currentcnt = 1;
	int searchindex = 1;
	bool present = false;
	while (1) {//(start->currentcnt <= start->count) {
		//Do the check inside the loop, to catch the right break-point
		if (strcmp(start->current->ssid, select) == 0) {
			present = true;
			printf("\n");
			printf("\tSearch-Entry #%d", searchindex);
			printf("\n");
			if (start->current->flags & SCAN_CHAIN_ELE_NO_BSS_INF) {
				printf("\tBad Message:\n\tNo BSS Info delivered.\nOr not parsable.");
				printf("\n");
			} else {
				printf("\tSSID: ");
				if (start->current->flags & SCAN_CHAIN_ELE_NO_SSID) {
					printf("No SSID delivered!");
				} else if (start->current->flags & SCAN_CHAIN_ELE_SSID_TO_LONG) {
					printf("Bad SSID delivered: To long!");
				} else {
					printf("%s",start->current->ssid);
				}
				printf("\n");
				printf("\tMAC: ");
				printMAC(start->current->mac, 6);
				printf("\n");
				if (start->current->flags & SCAN_CHAIN_ELE_ASSOCIATED) {
					printf("\t   (Associated)\n");
				}
				if (start->current->flags & SCAN_CHAIN_ELE_AUTHENTICATED) {
					printf("\t   (Authenticated with)\n");
				}
				if (start->current->flags & SCAN_CHAIN_ELE_IBSS_JOINED) {
					printf("\t   (Joined IBSS)\n");
				}
				printf("\tTSF: %llu usec", start->current->tsf);
				printf(" (%llud, %.2lld:%.2llu:%.2llu)",
					start->current->tsf/1000/1000/60/60/24, (start->current->tsf/1000/1000/60/60) % 24,
					(start->current->tsf/1000/1000/60) % 60, (start->current->tsf/1000/1000) % 60);
				printf("\n");
				printf("\tFrequency: %d.%.3d GHz (Channel %d)", (start->current->freq)/1000, (start->current->freq)%1000, ieee80211_frequency_to_channel(start->current->freq));
				printf("\n");
				printf("\tBeacon Interval: %d TUs", start->current->beaconinterval);
				printf("\n");
				printf("\tCapability:");
				if (start->current->flags & SCAN_CHAIN_ELE_CAPA_DMG) {
					switch (start->current->capability & WLAN_CAPABILITY_DMG_TYPE_MASK) {
					case WLAN_CAPABILITY_DMG_TYPE_AP:
						printf(" DMG_ESS");
						break;
					case WLAN_CAPABILITY_DMG_TYPE_PBSS:
						printf(" DMG_PCP");
						break;
					case WLAN_CAPABILITY_DMG_TYPE_IBSS:
						printf(" DMG_IBSS");
						break;
					}
					if (start->current->capability & WLAN_CAPABILITY_DMG_CBAP_ONLY)
						printf(" CBAP_Only");
					if (start->current->capability & WLAN_CAPABILITY_DMG_CBAP_SOURCE)
						printf(" CBAP_Src");
					if (start->current->capability & WLAN_CAPABILITY_DMG_PRIVACY)
						printf(" Privacy");
					if (start->current->capability & WLAN_CAPABILITY_DMG_ECPAC)
						printf(" ECPAC");
					if (start->current->capability & WLAN_CAPABILITY_DMG_SPECTRUM_MGMT)
						printf(" SpectrumMgmt");
					if (start->current->capability & WLAN_CAPABILITY_DMG_RADIO_MEASURE)
						printf(" RadioMeasure");
				} else {
					if (start->current->capability & WLAN_CAPABILITY_ESS)
						printf(" ESS");
					if (start->current->capability & WLAN_CAPABILITY_IBSS)
						printf(" IBSS");
					if (start->current->capability & WLAN_CAPABILITY_CF_POLLABLE)
						printf(" CfPollable");
					if (start->current->capability & WLAN_CAPABILITY_CF_POLL_REQUEST)
						printf(" CfPollReq");
					if (start->current->capability & WLAN_CAPABILITY_PRIVACY)
						printf(" Privacy");
					if (start->current->capability & WLAN_CAPABILITY_SHORT_PREAMBLE)
						printf(" ShortPreamble");
					if (start->current->capability & WLAN_CAPABILITY_PBCC)
						printf(" PBCC");
					if (start->current->capability & WLAN_CAPABILITY_CHANNEL_AGILITY)
						printf(" ChannelAgility");
					if (start->current->capability & WLAN_CAPABILITY_SPECTRUM_MGMT)
						printf(" SpectrumMgmt");
					if (start->current->capability & WLAN_CAPABILITY_QOS)
						printf(" QoS");
					if (start->current->capability & WLAN_CAPABILITY_SHORT_SLOT_TIME)
						printf(" ShortSlotTime");
					if (start->current->capability & WLAN_CAPABILITY_APSD)
						printf(" APSD");
					if (start->current->capability & WLAN_CAPABILITY_RADIO_MEASURE)
						printf(" RadioMeasure");
					if (start->current->capability & WLAN_CAPABILITY_DSSS_OFDM)
						printf(" DSSS-OFDM");
					if (start->current->capability & WLAN_CAPABILITY_DEL_BACK)
						printf(" DelayedBACK");
					if (start->current->capability & WLAN_CAPABILITY_IMM_BACK)
						printf(" ImmediateBACK");
				}
				printf(" (0x%.4x)", start->current->capability);
				printf("\n");
				printf("\tSignal Level: ");
				if (start->current->flags & SCAN_CHAIN_ELE_SIGNAL_LEVEL) {
					printf("%d/100", start->current->signallevel);
				} else {
					printf("%d.%.2d dBm", (start->current->signallevel)/100, (start->current->signallevel)%100);
				}
				printf("\n");
				printf("\tLast seen: %d ms ago", start->current->lastseen);
				printf("\n");
			}
			searchindex++;
		}
		if (start->currentcnt >= start->count)
			break;
		if (!(start->current->next))// && (start->currentcnt < start->count))
			return STRUCT_ERR_DMG;
		start->current = start->current->next;
		start->currentcnt++;
	}

	if (!present) {
		printf("\nNo WLAN Net with searched SSID\n\t%s\npresent.\n", select);
	}

	printf("<--------------------------------------->");
	printf("\n");
	return 0;

	NoChainElement:
	printf("<--------------------------------------->");
	printf("\n");
	return STRUCT_ERR_INCOMPLETE;

	NoChain:
	return STRUCT_ERR_NOT_EXIST;
}






int free_scan_chain (struct chain_start *start) {
	if (!start)
		return STRUCT_ERR_NOT_EXIST;
	if (start->count < 1) {
		goto NoElementInChain;
	}
	if (start->count > 1) {
		if (!(start->start->next))
			return STRUCT_ERR_DMG;
		start->current = start->start->next;
		start->currentcnt = 1;
		while (start->currentcnt < start->count - 1) {
			free(start->start);
			if (!(start->current->next))
				return STRUCT_ERR_DMG;
			start->start = start->current;
			start->current = start->start->next;
			start->currentcnt++;
		}
		free(start->current);
	}
	free(start->start);

	NoElementInChain:
	free(start);
	chainstart = NULL;

	return 0;
}

