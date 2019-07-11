/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */

#include <sys/socket.h>
#include </usr/include/linux/wireless.h>
#include </usr/include/linux/if_arp.h>
#include <ifaddrs.h>
#include <netinet/in.h>

#include "ollerus_globalsettings.h"
#include "ollerus.h"
#include "remainder.h"
#include "remainder_extended.h"
//-------END ioctl-------//

/*
 * Just a bunch of helpful functions/makros
 * Especially some used for debugging/developing
 * Or something nice like the ansi_escape_use setting
 */





int setTXPowerGetFactor(int *factor,struct nl80211_state *sktctr) {
	int err;
	*factor=1;
	struct CommandContainer cmdown;
	cmdown.prepatt = NULL;
	struct CommandContainer *cmd = &cmdown;//short workaround to try something...
//get the current TX Power
	static int txpower;
	static int txpower2;

	//TODO: Better without ioctl
//	ioctl_get_txpower (&txpower);

//Now try to set the TXPower exactly to that value. If we get an error or
//after the set the new TXPower differs from the old value we have to try another factor
	TryAgain:
	txpower2=txpower*(*factor);
	if (txpower2==0){
		fprintf(stderr, "Sorry, my whole Routine to evaluate the Factor for\nsetting the TXPower fucked up...\nMaybe i can't set the TXPower, because\nyou didn't start me as Sudo?\n");
	}
	cmd->cmd = NL80211_CMD_SET_WIPHY;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	static enum nl80211_tx_power_setting txsettype;
	cmd->nl_msg_flags = 0;
	cmd->callbackToUse = NULL;
	txsettype = NL80211_TX_POWER_FIXED;
	prepareAttribute(cmd, NL80211_ATTR_WIPHY_TX_POWER_LEVEL, &txpower2);
	prepareAttribute(cmd, NL80211_ATTR_WIPHY_TX_POWER_SETTING, &txsettype);
	err = send_with_cmdContainer(sktctr, 0, 0, cmd);
	if (err>0) {
		//Try next factor
		goto DoSame;
	} else if (err<0) {
		if (errno == 0) {
			//Try next factor
			DoSame:
			*factor=(*factor)*10;
			goto TryAgain;
		} else {
			//Kernel or network unreachable or something like that
			//Try again...
			goto TryAgain;
		}
	}

//Now check if it was set to the correct value
	//TODO: Better without ioctl
//	ioctl_get_txpower (&txpower2);
	if(txpower==txpower2){
		//Everything ok. Get out of here
	} else {
		//I expect then they could only differ by a factor of 10^x
		if(txpower>txpower2) {
			*factor = (*factor) * (txpower/txpower2);
			goto TryAgain;
		} else {
			*factor = (*factor) * (txpower2/txpower);
			goto TryAgain;
		}
	}

	printf("INFO: To set the TX Power this Kernel needs a factor of %d.\n",*factor);
	return err;
}






static int Do_CMD_GET_WIPHY_cb(struct nl_msg* msg, void* arg) {
	struct nlmsghdr *got_hdr = nlmsg_hdr(msg);
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];

//	if (got_hdr->nlmsg_type != expectedId) {
//		return NL_STOP;
//	}

    struct genlmsghdr *gnlh = (struct genlmsghdr*) nlmsg_data(got_hdr);

	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);


	if(got_attr[NL80211_ATTR_SUPPORTED_COMMANDS]) {
		struct nlattr *pos;
		int rem;// = nla_len(got_attr[NL80211_ATTR_SUPPORTED_COMMANDS]);
		printf("\nSupported Commands:\n");
		nla_for_each_nested(pos, got_attr[NL80211_ATTR_SUPPORTED_COMMANDS], rem) {
			printf("%u\n", nla_get_u32(pos));
		}
	} else {
		printf("Something has gone wrong!\n");
	}


	if (got_attr[NL80211_ATTR_WIPHY]) {
		char *ptr = nla_get_string(got_attr[NL80211_ATTR_WIPHY]);
		printf("on requested Wiphy: %s\n", ptr);
	}
	printf(">---------------------------------------------<\n");

	PrintContainingAtts;

	return 0;
}




