
/*
 * Authored by
 * Dennis Krummacker (03.06.14-)
 * Christoph Fischer (03.06.14-)
 * Based on Experience from iw (Johannes Berg)
 */



#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <linux/if.h>
#include <net/if.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include <netlink/object.h>
#include <netlink/utils.h>
#include <netlink/socket.h>
//#include <netlink/route/link.h>
#include <netlink/cache.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

//#include "nl80211.h"
#include "ollerus.h"
#include "scan_chain.h"


//For a scan you have to run this programm as a super-user


#define ARRAY_SIZE(ar) (sizeof(ar)/sizeof(ar[0]))

static int expectedId2;


static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err, void *arg)
{
	int *ret = arg;
	*ret = err->error;
	return NL_STOP;
}

static int finish_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_SKIP;
}

static int ack_handler(struct nl_msg *msg, void *arg)
{
	int *ret = arg;
	*ret = 0;
	return NL_STOP;
}
static int no_seq_check(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

struct handler_args {
	const char *group;
	int id;
};


static int family_handler(struct nl_msg *msg, void *arg)
{
	struct handler_args *grp = arg;
	struct nlattr *tb[CTRL_ATTR_MAX + 1];
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	struct nlattr *mcgrp;
	int rem_mcgrp;

	nla_parse(tb, CTRL_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
		  genlmsg_attrlen(gnlh, 0), NULL);

	if (!tb[CTRL_ATTR_MCAST_GROUPS])
		return NL_SKIP;

	nla_for_each_nested(mcgrp, tb[CTRL_ATTR_MCAST_GROUPS], rem_mcgrp) {
		struct nlattr *tb_mcgrp[CTRL_ATTR_MCAST_GRP_MAX + 1];

		nla_parse(tb_mcgrp, CTRL_ATTR_MCAST_GRP_MAX,
			  nla_data(mcgrp), nla_len(mcgrp), NULL);

		if (!tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME] ||
		    !tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID])
			continue;
		if (strncmp(nla_data(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME]),
			    grp->group, nla_len(tb_mcgrp[CTRL_ATTR_MCAST_GRP_NAME])))
			continue;
		grp->id = nla_get_u32(tb_mcgrp[CTRL_ATTR_MCAST_GRP_ID]);
		break;
	}

	return NL_SKIP;
}



int nl_get_multicast_id(struct nl_sock *sock, const char *family, const char *group)
{
	struct nl_msg *msg;
	struct nl_cb *cb;
	int ret, ctrlid;
	struct handler_args grp = {
		.group = group,
		.id = -ENOENT,
	};

	msg = nlmsg_alloc();
	if (!msg)
		return -ENOMEM;

	cb = nl_cb_alloc(NL_CB_DEFAULT);
	if (!cb) {
		ret = -ENOMEM;
		goto out_fail_cb;
	}

	ctrlid = genl_ctrl_resolve(sock, "nlctrl");

	genlmsg_put(msg, 0, 0, ctrlid, 0,
		    0, CTRL_CMD_GETFAMILY, 0);

	ret = -ENOBUFS;
	NLA_PUT_STRING(msg, CTRL_ATTR_FAMILY_NAME, family);

	ret = nl_send_auto_complete(sock, msg);
	if (ret < 0)
		goto out;

	ret = 1;

	nl_cb_err(cb, NL_CB_CUSTOM, error_handler, &ret);
	nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, &ret);
	nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, family_handler, &grp);

	while (ret > 0)
		nl_recvmsgs(sock, cb);

	if (ret == 0)
		ret = grp.id;
 nla_put_failure:
 out:
	nl_cb_put(cb);
 out_fail_cb:
	nlmsg_free(msg);
	return ret;
}







struct wait_event {
	int n_cmds;
	const __u32 *cmds;
	__u32 cmd;
	struct print_event_args *pargs;
};



static int wait_event(struct nl_msg *msg, void *arg)
{

	struct wait_event *wait = arg;
	struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
	int i;

	for (i = 0; i < wait->n_cmds; i++) {
		if (gnlh->cmd == wait->cmds[i]) {
			wait->cmd = gnlh->cmd;
		}
	}

	return NL_SKIP;
}

int __prepare_listen_events(struct nl80211_state *state)
{

	int mcid, ret;

	/* Configuration multicast group */
	mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "config");
	if (mcid < 0)
		return mcid;

	ret = nl_socket_add_membership(state->nl_sock, mcid);
	if (ret)
		return ret;

	/* Scan multicast group */
	mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "scan");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(state->nl_sock, mcid);
		if (ret)
			return ret;
	}

	/* Regulatory multicast group */
	mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "regulatory");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(state->nl_sock, mcid);
		if (ret)
			return ret;
	}

	/* MLME multicast group */
	mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "mlme");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(state->nl_sock, mcid);
		if (ret)
			return ret;
	}

	mcid = nl_get_multicast_id(state->nl_sock, "nl80211", "vendor");
	if (mcid >= 0) {
		ret = nl_socket_add_membership(state->nl_sock, mcid);
		if (ret)
			return ret;
	}

	return 0;
}

__u32 __do_listen_events(struct nl80211_state *state,
			 const int n_waits, const __u32 *waits,
			 struct print_event_args *args)
{

	struct nl_cb *cb = nl_cb_alloc(do_debug ? NL_CB_DEBUG : NL_CB_DEFAULT);
	struct wait_event wait_ev;

	if (!cb) {
		fprintf(stderr, "failed to allocate netlink callbacks\n");
		return -ENOMEM;
	}

	/* no sequence checking for multicast messages */
	nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, no_seq_check, NULL);

	if (n_waits && waits) {

		wait_ev.cmds = waits;
		wait_ev.n_cmds = n_waits;
		wait_ev.pargs = args;
		nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, wait_event, &wait_ev);
	}
	wait_ev.cmd = 0;

	while (!wait_ev.cmd)
		nl_recvmsgs(state->nl_sock, cb);


	nl_cb_put(cb);

	return wait_ev.cmd;
}

__u32 listen_events(struct nl80211_state *state,
		    const int n_waits, const __u32 *waits)
{
	int ret;
	int an_waits = n_waits;

	ret = __prepare_listen_events(state);
	if (ret)
		return ret;

	return __do_listen_events(state, an_waits, waits, NULL);
}




void print_ssid_escaped(const uint8_t len, const uint8_t *data)
{
	int i;

	for (i = 0; i < len; i++) {
		if (isprint(data[i]) && data[i] != ' ' && data[i] != '\\')
			printf("%c", data[i]);
		else if (data[i] == ' ' &&
			 (i != 0 && i != len -1))
			printf(" ");
		else
			printf("\\x%.2x", data[i]);
	}
}





static int Do_CMD_TRIGGER_SCAN_cb (struct nl_msg* msg, void* arg){
}

static int Do_CMD_GET_SCAN_cb (struct nl_msg* msg, void* arg){

	//see  print_bss_handler in the scan.c of iw
		struct nlattr *tb[NL80211_ATTR_MAX + 1];
		struct genlmsghdr *gnlh = nlmsg_data(nlmsg_hdr(msg));
		struct nlattr *bss[NL80211_BSS_MAX + 1];
		char dev[20];
		static struct nla_policy bss_policy[NL80211_BSS_MAX + 1] = {
			[NL80211_BSS_TSF] = { .type = NLA_U64 },
			[NL80211_BSS_FREQUENCY] = { .type = NLA_U32 },
			[NL80211_BSS_BSSID] = { },
			[NL80211_BSS_BEACON_INTERVAL] = { .type = NLA_U16 },
			[NL80211_BSS_CAPABILITY] = { .type = NLA_U16 },
			[NL80211_BSS_INFORMATION_ELEMENTS] = { },
			[NL80211_BSS_SIGNAL_MBM] = { .type = NLA_U32 },
			[NL80211_BSS_SIGNAL_UNSPEC] = { .type = NLA_U8 },
			[NL80211_BSS_STATUS] = { .type = NLA_U32 },
			[NL80211_BSS_SEEN_MS_AGO] = { .type = NLA_U32 },
			[NL80211_BSS_BEACON_IES] = { },
		};
		static struct chain_element tostore;
		memset(&tostore, 0, sizeof(struct chain_element));

		bool is_dmg = false;

		nla_parse(tb, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
			  genlmsg_attrlen(gnlh, 0), NULL);

		if (!tb[NL80211_ATTR_BSS]) {
			tostore.flags = tostore.flags | SCAN_CHAIN_ELE_NO_BSS_INF;
//			fprintf(stderr, "bss info missing!\n");
			return NL_SKIP;
		}
		if (nla_parse_nested(bss, NL80211_BSS_MAX, tb[NL80211_ATTR_BSS], bss_policy)) {
			tostore.flags = tostore.flags | SCAN_CHAIN_ELE_NO_BSS_INF;
//			fprintf(stderr, "failed to parse nested attributes!\n");
			return NL_SKIP;
		}

		if (!bss[NL80211_BSS_BSSID])
			return NL_SKIP;


		if (bss[NL80211_BSS_INFORMATION_ELEMENTS]) {
			char *SSIDPointer = nla_data(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
			SSIDPointer++;
			unsigned char SSIDLength = *SSIDPointer;
			//To get sure everything is alright
			if (SSIDLength <= 32 && SSIDLength <= nla_len(bss[NL80211_BSS_INFORMATION_ELEMENTS]) - 2) {
				memcpy(tostore.ssid, SSIDPointer + 1, SSIDLength);
				memset(tostore.ssid+SSIDLength, 0, 32-SSIDLength);
			} else {
				tostore.flags = tostore.flags | SCAN_CHAIN_ELE_SSID_TO_LONG;
				return NL_SKIP;
			}
		} else {
			tostore.flags = tostore.flags | SCAN_CHAIN_ELE_NO_SSID;
			return NL_SKIP;
		}
		if (bss[NL80211_BSS_BSSID]) {
			memcpy(tostore.mac, nla_data(bss[NL80211_BSS_BSSID]), 20);
			if (bss[NL80211_BSS_STATUS]) {
				switch (nla_get_u32(bss[NL80211_BSS_STATUS])) {
				case NL80211_BSS_STATUS_ASSOCIATED:
					tostore.flags = tostore.flags | SCAN_CHAIN_ELE_ASSOCIATED;
					break;
				case NL80211_BSS_STATUS_AUTHENTICATED:
					tostore.flags = tostore.flags | SCAN_CHAIN_ELE_AUTHENTICATED;
					break;
				case NL80211_BSS_STATUS_IBSS_JOINED:
					tostore.flags = tostore.flags | SCAN_CHAIN_ELE_IBSS_JOINED;
					break;
				default:
					break;
				}
			}
		}
		if (bss[NL80211_BSS_TSF]) {
			tostore.tsf = (unsigned long long)nla_get_u64(bss[NL80211_BSS_TSF]);
		}
		if (bss[NL80211_BSS_FREQUENCY]) {
			tostore.freq = nla_get_u32(bss[NL80211_BSS_FREQUENCY]);
			if (tostore.freq > 45000)
				tostore.flags = tostore.flags | SCAN_CHAIN_ELE_CAPA_DMG;
		}
		if (bss[NL80211_BSS_BEACON_INTERVAL])
			tostore.beaconinterval = nla_get_u16(bss[NL80211_BSS_BEACON_INTERVAL]);
		if (bss[NL80211_BSS_CAPABILITY]) {
			tostore.capability = nla_get_u16(bss[NL80211_BSS_CAPABILITY]);
		}
		if (bss[NL80211_BSS_SIGNAL_UNSPEC]) {
			tostore.signallevel = (int) nla_get_u8(bss[NL80211_BSS_SIGNAL_UNSPEC]);
			tostore.flags = tostore.flags | SCAN_CHAIN_ELE_SIGNAL_LEVEL;
		}
		if (bss[NL80211_BSS_SIGNAL_MBM]) {
			tostore.signallevel = nla_get_u32(bss[NL80211_BSS_SIGNAL_MBM]);
		}
		if (bss[NL80211_BSS_SEEN_MS_AGO]) {
			tostore.lastseen = nla_get_u32(bss[NL80211_BSS_SEEN_MS_AGO]);
		}
		chainstart->append (&tostore, chainstart);

		return NL_SKIP;
	}



void scan_around (struct nl80211_state *sktctr, int *argc, char *argv[], struct CommandContainer *cmd, int *err) {


	//create a new socket for listening to the trigger-scan
	//allocate socket, generic connect, put driver ID in Container
	struct nl80211_state sktscan; //contains the nl_socket
	*err = nl80211_init_socket(&sktscan);

	//Store the driver ID in global Var. Makes it easy and smooth with Callbacks
	expectedId2 = sktscan.nl80211_id;


	setupScanChainStart();


	cmd->cmd = NL80211_CMD_TRIGGER_SCAN;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	cmd->nl_msg_flags = 0;
	cmd->callbackToUse = Do_CMD_TRIGGER_SCAN_cb;
	*err = send_with_cmdContainer(sktctr, argc, argv, cmd);
	//
	static const __u32 cmds[] = {
		NL80211_CMD_NEW_SCAN_RESULTS,
		NL80211_CMD_SCAN_ABORTED,
	};

	listen_events(&sktscan ,ARRAY_SIZE(cmds) , cmds);
	//
	cmd->cmd = NL80211_CMD_GET_SCAN;
	prepareAttribute(cmd, NL80211_ATTR_IFINDEX, &ifIndex);
	cmd->nl_msg_flags = NLM_F_ROOT | NLM_F_MATCH;
	cmd->callbackToUse = Do_CMD_GET_SCAN_cb;
	*err = send_with_cmdContainer(&sktscan, argc, argv, cmd);

	nl80211_cleanup_socket(&sktscan);
}
