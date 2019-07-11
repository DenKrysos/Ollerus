#ifndef REMAINDER_EXTENDED_H
#define REMAINDER_EXTENDED_H
/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 */


#include "ollerus_globalsettings.h"
//#include "ollerus.h"

/*
 * Just a bunch of helpful functions/makros
 * Especially used for debugging/developing
 * Or something nice like the ansi_escape_use setting
 */

#include <pwd.h>





int senddetermined (int socket, char *msg, int msglen);






#define PrintContainingAtts printf("Message contains Attributes:\n"); \
	int i; \
	int outMatInd=1; \
	enum nl80211_attrs a; \
	for (i=0;i<=NL80211_ATTR_MAX;i++) { \
		a = (enum nl80211_attrs) i; \
		if(got_attr[a]){ \
			switch(outMatInd % 5){ \
			case 0: \
				printf("\t%d\n", a); \
			break; \
			case 1: \
				printf("%d",a); \
			break; \
			default: \
				printf("\t%d", a); \
				break; \
			} \
			outMatInd++; \
		} \
	} \
	printf("\n>---------------------------<\n");
/*
 * Not complete. Really wrong for now...
 * Maybe oneday goto finish...
 * Probably not...
#define PrintContainingAtts printf("Message contains Attributes:\n"); \
	int i; \
	enum nl80211_attrs a; \
	for (i=0;i<=NL80211_ATTR_MAX;i++) { \
		a = (enum nl80211_attrs) i; \
		char newline=0; \
		char emptyelement=0; \
		char emptyline=0; \
		if(got_attr[a]){ \
			switch(i % 4){ \
			case 0: \
				printf("\t%d", a); \
				newline=1; \
			break; \
			case 1: \
				printf("%d",a); \
			break; \
			default: \
				printf("\t%d", a); \
				break; \
			} \
		} \
		else { \
			switch(i % 4){ \
			case 0: \
				newline=1; \
				break; \
			case 1: \
				emptyelement=1; \
				break; \
			default: \
				if(emptyelement){ \
					emptyelement++; \
				} \
				break; \
			} \
			if(emptyelement >= 3){ \
				emptyline=1; \
				newline=0; \
			} \
		} \
		if(newline) \
			puts(""); \
	} \
	printf("\n>---------------------------------------<\n");
*/



static int Do_CMD_GET_WIPHY_cb(struct nl_msg* msg, void* arg);




static int Do_CMD_GET_WIPHY_supported_cmds_cb(struct nl_msg* msg, void* arg) {
	struct nlmsghdr *got_hdr = nlmsg_hdr(msg);
	struct nlattr *got_attr[NL80211_ATTR_MAX + 1];

//	if (got_hdr->nlmsg_type != expectedId) {
//		return NL_STOP;
//	}

    struct genlmsghdr *gnlh = (struct genlmsghdr*) nlmsg_data(got_hdr);

	nla_parse(got_attr, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			genlmsg_attrlen(gnlh, 0), NULL);


	printf("\t>------------------------------------<\n");
	if(got_attr[NL80211_ATTR_SUPPORTED_COMMANDS]) {
		struct nlattr *pos;
		int rem;// = nla_len(got_attr[NL80211_ATTR_SUPPORTED_COMMANDS]);
		printf("\t\tSupported Commands:\n");
		nla_for_each_nested(pos, got_attr[NL80211_ATTR_SUPPORTED_COMMANDS], rem) {
			printf("\t\t\t%u\n", nla_get_u32(pos));
		}
	} else {
		printf("\t\tSomething has gone wrong!\n");
	}

	if (got_attr[NL80211_ATTR_WIPHY_NAME]) {
		printf("\t\ton requested Wiphy: %s\n", nla_data(got_attr[NL80211_ATTR_WIPHY_NAME]));
		printf("\t\t(ATTR_WIPHY_NAME length: %d)\n", nla_len(got_attr[NL80211_ATTR_WIPHY_NAME]));
	}

	if(got_attr[NL80211_ATTR_WIPHY]) {
		printf("\n\t\tList of all Wiphys:\n\t\t(delivered by Index behind phy#)\n");
		int listlength = nla_len(got_attr[NL80211_ATTR_WIPHY]) / sizeof(int);
		int *indexstart = nla_data(got_attr[NL80211_ATTR_WIPHY]);
		int i=0;
		for(i=0;i<listlength;i++)
			printf("\t\t\tphy%d\n", *(indexstart+i*sizeof(int)));
		printf("\t\t(ATTR_WIPHY length: %d)\n", listlength);
	}
	printf("\t>------------------------------------<\n");

	return 0;
}
//Prints out the nl80211_cmds which your driver supports
#define PrintSupportedCmds cmd.cmd = NL80211_CMD_GET_WIPHY; \
	prepareAttribute(&cmd, NL80211_ATTR_IFINDEX, &ifIndex); \
	cmd.nl_msg_flags = 0; \
	cmd.callbackToUse = Do_CMD_GET_WIPHY_supported_cmds_cb; \
	err = send_with_cmdContainer(&sktctr, argc, argv, &cmd);






#endif /* REMAINDER_EXTENDED_H */
