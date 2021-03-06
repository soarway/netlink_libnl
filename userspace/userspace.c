#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netlink/msg.h>
#include <netlink/attr.h>

#include "../common.h"

static char message[GENL_TEST_ATTR_MSG_MAX]; //save the message for send to kernel
static int send_to_kernel = 0;
static unsigned int mcgroups ;		/* Mask of groups */

static void usage(char* name)
{
	printf("Usage: %s\n"
		"	-h : this help message\n"
		"	-l : listen on one or more groups, comma separated\n"
		"	-m : the message to send\n",
		name);
}

static void add_group(char* group)
{
	unsigned int grp = strtoul(group, NULL, 10);

	if (grp > GENL_TEST_MCGRP_MAX-1) {
		fprintf(stderr, "Invalid group number %u. Values allowed 0:%u\n", grp, GENL_TEST_MCGRP_MAX-1);
		exit(EXIT_FAILURE);
	}

	mcgroups |= 1 << (grp);
}

static void parse_cmd_line(int argc, char** argv)
{
	char* opt_val;

	while (1) {
		int opt = getopt(argc, argv, "hl:m:s");

		if (opt == EOF)
			break;

		switch (opt) {
		case 'h':
			usage(argv[0]);
			exit(EXIT_SUCCESS);

		case 'l':
			opt_val = strtok(optarg, ",");
			add_group(opt_val); 
			while ((opt_val = strtok(NULL, ",")))
				add_group(opt_val); 

			break;

		case 'm':
			strncpy(message, optarg, GENL_TEST_ATTR_MSG_MAX);
			message[GENL_TEST_ATTR_MSG_MAX - 1] = '\0';
			send_to_kernel = 1;
			break;

		default:
			fprintf(stderr, "Unkown option %c !!\n", opt);
			exit(EXIT_FAILURE);
		}

	}


}




static int skip_seq_check(struct nl_msg *msg, void *arg)
{
	return NL_OK;
}

static int print_rx_msg(struct nl_msg *msg, void* arg)
{
	struct nlattr *attr[GENL_TEST_ATTR_MAX+1];

	genlmsg_parse(nlmsg_hdr(msg), 0, attr, GENL_TEST_ATTR_MAX, genl_test_policy);

	if (!attr[GENL_TEST_ATTR_MSG]) {
		fprintf(stdout, "Kernel sent empty message!!\n");
		return NL_OK;
	}

	fprintf(stdout, "Kernel says: %s \n", nla_get_string(attr[GENL_TEST_ATTR_MSG]));

	return NL_OK;
}

static void prep_nl_sock(struct nl_sock** nlsock)
{
	int family_id, grp_id;
	unsigned int bit = 0;
	
	*nlsock = nl_socket_alloc();
	if(!*nlsock) {
		fprintf(stderr, "Unable to alloc nl socket!\n");
		exit(EXIT_FAILURE);
	}

	/* disable seq checks on multicast sockets */
	nl_socket_disable_seq_check(*nlsock);
	//nl_socket_disable_auto_ack(*nlsock);

	/* connect to genl */
	if (genl_connect(*nlsock)) {
		fprintf(stderr, "Unable to connect to genl!\n");
		goto exit_err;
	}

	/* resolve the generic nl family id*/
	family_id = genl_ctrl_resolve(*nlsock, GENL_TEST_FAMILY_NAME);
	if(family_id < 0){
		fprintf(stderr, "Unable to resolve family name!\n");
		goto exit_err;
	}

	if (!mcgroups)
		return;

	while (bit < sizeof(unsigned int)) {
		if (!(mcgroups & (1 << bit)))
			goto next;

		//获取组ID
		grp_id = genl_ctrl_resolve_grp(*nlsock, GENL_TEST_FAMILY_NAME,	genl_test_mcgrp_names[bit]);

		if (grp_id < 0)	{
			fprintf(stderr, "Unable to resolve group name for %u!\n",	(1 << bit));
            goto exit_err;
		}
		
		//Join to group 添加到广播组，成为组播成员才能受到改组的消息
		if (nl_socket_add_membership(*nlsock, grp_id)) {
			fprintf(stderr, "Unable to join group %u!\n", 	(1 << bit));
            goto exit_err;
		}
next:
		bit++;
	}

    return;

exit_err:
    nl_socket_free(*nlsock); // this call closes the socket as well
    exit(EXIT_FAILURE);
}



static int send_msg_to_kernel(struct nl_sock *sock, pid_t pid)
{
	struct nl_msg* msg;
	int family_id, err = 0;

	family_id = genl_ctrl_resolve(sock, GENL_TEST_FAMILY_NAME);
	if(family_id < 0){
		fprintf(stderr, "Unable to resolve family name!\n");
		exit(EXIT_FAILURE);
	}

	msg = nlmsg_alloc();
	if (!msg) {
		fprintf(stderr, "failed to allocate netlink message\n");
		exit(EXIT_FAILURE);
	}

	if(!genlmsg_put(msg, pid, /*NL_AUTO_PID,*/ NL_AUTO_SEQ, family_id, 0, NLM_F_REQUEST, GENL_TEST_C_MSG, 0)) {
		fprintf(stderr, "failed to put nl hdr!\n");
		err = -ENOMEM;
		goto out;
	}

	err = nla_put_string(msg, GENL_TEST_ATTR_MSG, message);
	if (err) {
		fprintf(stderr, "failed to put nl string!\n");
		goto out;
	}

	
	//自动发送， 也可以选择手动发送函数
	err = nl_send_auto(sock, msg);
	//err = nl_send_auto_complete(sock, msg);
	if (err < 0) {
		fprintf(stderr, "failed to send nl message!\n");
	}

	for(;;) {
		nl_recvmsgs_default(sock);
	}
out:
	nlmsg_free(msg);
	return err;
}


int main(int argc, char** argv)
{
	struct nl_sock* nlsock = NULL;
	struct nl_cb *cb = NULL;
	int ret;
	pid_t pid = getpid();
	fprintf(stderr, "My pid = %u!\n", 	pid);
	parse_cmd_line(argc, argv);

	prep_nl_sock(&nlsock);

	/* prep the cb */
	cb = nl_cb_alloc(NL_CB_DEFAULT);
	nl_cb_set(cb, NL_CB_SEQ_CHECK, NL_CB_CUSTOM, skip_seq_check, NULL);
	//nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, print_rx_msg, NULL);
	nl_socket_modify_cb(nlsock, NL_CB_VALID, NL_CB_CUSTOM, print_rx_msg, NULL);
	
	if (send_to_kernel) {
		ret = send_msg_to_kernel(nlsock, pid);
		//do {
			//ret = nl_recvmsgs(nlsock, cb);
		//} while (!ret);
		exit(EXIT_SUCCESS);
	}


	do {
		ret = nl_recvmsgs(nlsock, cb);
	} while (!ret);
	
	nl_cb_put(cb);
    nl_socket_free(nlsock);
	return 0;
}
