/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#include "ollerus.h"





static int Do_CMD_SET_WIPHY_cb (struct nl_msg* msg, void* arg){
}


int handleSet(struct nl80211_state *sktctr, int argc, char **argstart, struct CommandContainer *cmd) {
	int err=0;
	int loopcnt;
	if (argc < 3) {
		printf("To few arguments for:\n");
		for(loopcnt=0;loopcnt<argc;loopcnt++)
			printf(" %s", *(argstart+loopcnt));
		printf("\n");
		return MAIN_ERR_FEW_CMDS;
	}
	cmd->cmd = NL80211_CMD_SET_WIPHY;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);

	if (strcmp(*(argstart+1), "txpower") == 0) {
	// Some Kernel needs different Values for the Transmission Power Set
	// i.e. To set the tx power to 20 dBm, some Kernel want to get 20, others want 2000
	// thus ascertain the needed factor
		int txpowersetfactor=1;
		setTXPowerGetFactor(&txpowersetfactor,sktctr);
		static enum nl80211_tx_power_setting txsettype;
		cmd->nl_msg_flags = 0;
		cmd->callbackToUse = NULL;
		if (strcmp(*(argstart+2), "auto") == 0) {
			txsettype = NL80211_TX_POWER_AUTOMATIC;
		} else {
			if (argc < 4) {
				printf("To few arguments for:\n");
				for(loopcnt=0;loopcnt<argc;loopcnt++)
					printf(" %s", *(argstart+loopcnt));
				printf("\nI would like to get:\nset txpower <auto|fixed|limited> [value in mBm]\n");
				return MAIN_ERR_FEW_CMDS;
			}
			if (strcmp(*(argstart+2), "fixed") == 0) {
				txsettype = NL80211_TX_POWER_FIXED;
			} else if (strcmp(*(argstart+2), "limited") == 0) {
				txsettype = NL80211_TX_POWER_LIMITED;
			} else {
				printf("Invalid parameter after txpower: %s\nExpected <auto | fixed | limited>\n", *(argstart+2));
				return MAIN_ERR_BAD_CMDLINE;
			}

			char *endpoint;
			static long power;
			power = strtol(*(argstart+3), &endpoint, 10);
			if (*endpoint)
				return MAIN_ERR_BAD_CMDLINE;
			power=power*txpowersetfactor;
			prepareAttribute(cmd, NL80211_ATTR_WIPHY_TX_POWER_LEVEL, &power);
		}
		prepareAttribute(cmd, NL80211_ATTR_WIPHY_TX_POWER_SETTING, &txsettype);

//		*aktargindex += 3;
	}

	// set channel

	else if (strcmp(*(argstart+1), "channel") == 0) {

			cmd->nl_msg_flags = 0;
			cmd->callbackToUse = NULL;

			char *endpoint;
			int chan;
			chan = strtol(*(argstart+2), &endpoint, 10);
			enum nl80211_band band;
			band = chan <= 14 ? NL80211_BAND_2GHZ : NL80211_BAND_5GHZ;
			int freq = ieee80211_channel_to_frequency(chan, band);

			static enum nl80211_channel_type chanwidth;
			chanwidth = NL80211_CHAN_NO_HT;
			if(argc > 3){

				if (strcmp(*(argstart+3), "HT20") == 0) {
					chanwidth = NL80211_CHAN_HT20;
				} else if (strcmp(*(argstart+3), "HT40-") == 0) {
					chanwidth =  NL80211_CHAN_HT40MINUS;
				} else if (strcmp(*(argstart+3), "HT40+") == 0) {
					chanwidth = NL80211_CHAN_HT40PLUS;
				}else {
					printf("Invalid parameter after channel [channr.]: %s\nExpected <HT20 | HT40- | HT40+>", *(argstart+2));
					return MAIN_ERR_BAD_CMDLINE;
				}
			}

				prepareAttribute(cmd, NL80211_ATTR_WIPHY_FREQ, &freq);

			prepareAttribute(cmd, NL80211_ATTR_CHANNEL_WIDTH, &chanwidth);

	//		*aktargindex += 3;
		}

	// set Operation Mode aka Infrastructure Type (station / ap / monitor)

	else if ((strcmp(*(argstart+1),"mode")==0) ||
				(strcmp(*(argstart+1),"type")==0) ||
				(strcmp(*(argstart+1),"iftype")==0)) {
		static enum nl80211_iftype iftype;

		if (strcmp(*(argstart+2), "adhoc") == 0) {
			iftype=NL80211_IFTYPE_ADHOC;
		} else if (strcmp(*(argstart+2), "station") == 0) {
			iftype=NL80211_IFTYPE_STATION;
		} else if (strcmp(*(argstart+2), "ap") == 0) {
			iftype=NL80211_IFTYPE_AP;
		} else if (strcmp(*(argstart+2), "monitor") == 0) {
			iftype=NL80211_IFTYPE_MONITOR;
		}//TODO: this few others

		cmd->cmd = NL80211_CMD_SET_INTERFACE;
		prepareAttribute(cmd, NL80211_ATTR_IFTYPE, &iftype);
	}
	err = send_with_cmdContainer(sktctr, argc, argstart, cmd);

	return err;
}
