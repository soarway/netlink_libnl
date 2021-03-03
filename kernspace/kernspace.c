#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netlink.h>
#include <linux/timer.h>
#include <linux/export.h>
#include <net/genetlink.h>

#include "../common.h"

static struct timer_list      timer;
static struct genl_family     genl_test_family;


	
static void greet_group(unsigned int group)
{	
	void *hdr;
	int res, flags = GFP_ATOMIC;
	char msg[GENL_TEST_ATTR_MSG_MAX];
	struct sk_buff* skb = genlmsg_new(NLMSG_DEFAULT_SIZE, flags);

	if (!skb) {
		printk(KERN_ERR "%d: OOM!!", __LINE__);
		return;
	}

	hdr = genlmsg_put(skb, 0, 0, &genl_test_family, flags, GENL_TEST_C_MSG);
	if (!hdr) {
		printk(KERN_ERR "%d: Unknown err !", __LINE__);
		goto nlmsg_fail;
	}

	snprintf(msg, GENL_TEST_ATTR_MSG_MAX, "Hello group %s\n", genl_test_mcgrp_names[group]);

	res = nla_put_string(skb, GENL_TEST_ATTR_MSG, msg);
	if (res) {
		printk(KERN_ERR "%d: err %d ", __LINE__, res);
		goto nlmsg_fail;
	}

	genlmsg_end(skb, hdr);
	genlmsg_multicast(&genl_test_family, skb, 0, group, flags);
	return;

nlmsg_fail:
	genlmsg_cancel(skb, hdr);
	nlmsg_free(skb);
	return;
}

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,14,0)
void genl_test_periodic(unsigned long data)
#else
void genl_test_periodic(struct timer_list *unused)
#endif
{
	greet_group(GENL_TEST_MCGRP0);
	greet_group(GENL_TEST_MCGRP1);
	greet_group(GENL_TEST_MCGRP2);

	mod_timer(&timer, jiffies + msecs_to_jiffies(GENL_TEST_HELLO_INTERVAL));
}

    /*
    * 添加用户数据，及添加一个netlink addribute
    *@type : nlattr的type
    *@len : nlattr中的len
    *@data : 用户数据
    */
    static inline int genl_msg_mk_usr_msg(struct sk_buff *skb, int type, void *data, int len)
    {
        int rc;
        /* add a netlink attribute to a socket buffer */
        if ((rc = nla_put(skb, type, len, data)) != 0) {
            return rc;
        }
        return 0;
    }
	
    /*
    * : 构建netlink及gennetlink首部
    * @cmd : genl_ops的cmd
    * @size : gen_netlink用户数据的长度（包括用户定义的首部）
    */
    static inline int build_usr_msg(struct genl_info *info, u8 cmd, size_t size, pid_t pid, struct sk_buff **skbp, int attr, void *data, int len)
    {
        struct sk_buff *skb = 0;
		void* hdr;
		int rc;
        /* create a new netlink msg */
        skb = genlmsg_new(size, GFP_KERNEL);
        if (skb == NULL) {
            return -ENOMEM;
        }
        /* Add a new netlink message to an skb */
        //hdr = genlmsg_put(skb, pid, 0, &genl_test_family, 0, cmd);
		hdr = genlmsg_put(skb, info->snd_portid, info->snd_seq, &genl_test_family,  0, cmd);
		
		
        /* add a netlink attribute to a socket buffer */
        if ((rc = nla_put(skb, attr, len, data)) != 0) {
			//genlmsg_cancel(skb, hdr);
            return rc;
        }
		
		genlmsg_end(skb, hdr);
		
		*skbp = skb;
        return 0;
    }
	
   /**
    * - 通过generic netlink发送数据到netlink
    *
    * @data: 发送数据缓存
    * @len: 数据长度 单位：byte
    * @pid: 发送到的客户端pid
    */
    int send_msg_to_userspace(void *data, int len,  struct genl_info* info, pid_t pid)
    {
        struct sk_buff *skb = 0;
        size_t size;
        //void *head;
        int rc;
        
        size = nla_total_size(len); /* total length of attribute including padding */
        rc = build_usr_msg(info, GENL_TEST_C_MSG, size, pid, &skb, GENL_TEST_ATTR_MSG, data, len);
        
        if (rc) {
            return rc;
        }
        //rc = genl_msg_mk_usr_msg(skb, GENL_TEST_ATTR_MSG, data, len);
        
        //if (rc) {
        //    kfree_skb(skb);
        //    return rc;
        //}
        
		//rc = genlmsg_unicast(genl_info_net(info), skb, pid);
        rc = genlmsg_unicast(&init_net, skb, pid);
        
        if (rc < 0) {
			printk("1 send end..FAIL.PID = %u.\n", pid);
            return rc;
        }
		printk("send end..OK. pid = %u.\n", pid);
        return 0;
    }
	
static int genl_test_rx_msg(struct sk_buff* skb, struct genl_info* info)
{
	char* data ;
    struct nlmsghdr *nlhdr ;
    struct genlmsghdr *genlhdr;
	struct nlattr *nla ;
	char* str ;
	int state;
	
	data = "I am a kernel!";
    nlhdr = nlmsg_hdr(skb);;
    genlhdr = nlmsg_data(nlhdr);
	nla = genlmsg_data(genlhdr);;
	str = (char *)NLA_DATA(nla);
	
	
	if (!info->attrs[GENL_TEST_ATTR_MSG]) {
		printk(KERN_ERR "empty message from %d!!\n", info->snd_portid);
		printk(KERN_ERR "%p\n", info->attrs[GENL_TEST_ATTR_MSG]);
		return -EINVAL;
	}

	printk(KERN_ERR "recved msg from userspace, snd_portid = %u, pid = %u, MSG: %s, str=%s \n", 
				info->snd_portid, nlhdr->nlmsg_pid, (char*)nla_data(info->attrs[GENL_TEST_ATTR_MSG]), str);
				

	//state = send_msg_to_userspace(data, strlen(data)+1, info, nlhdr->nlmsg_pid);
	state = send_msg_to_userspace(data, strlen(data)+1, info, info->snd_portid);

	return 0;
}

static const struct genl_ops genl_test_ops[] = {
	{
		.cmd = GENL_TEST_C_MSG,
#ifdef COMPAT_CANNOT_INDIVIDUAL_NETLINK_OPS_POLICY
		.policy = genl_test_policy,
#endif
		.doit = genl_test_rx_msg,
		.dumpit = NULL,
	},
};

static const struct genl_multicast_group genl_test_mcgrps[] = {
	[GENL_TEST_MCGRP0] = { .name = GENL_TEST_MCGRP0_NAME, },
	[GENL_TEST_MCGRP1] = { .name = GENL_TEST_MCGRP1_NAME, },
	[GENL_TEST_MCGRP2] = { .name = GENL_TEST_MCGRP2_NAME, },
};

static struct genl_family genl_test_family = {
	.name = GENL_TEST_FAMILY_NAME,
	.version = 1,
	.maxattr = GENL_TEST_ATTR_MAX,
	.netnsok = false,
	.module = THIS_MODULE,
	.ops = genl_test_ops,
	.n_ops = ARRAY_SIZE(genl_test_ops),
	.mcgrps = genl_test_mcgrps,
	.n_mcgrps = ARRAY_SIZE(genl_test_mcgrps),
};

static int __init genl_test_init(void)
{
	int rc;

	printk(KERN_INFO "genl_test: initializing netlink\n");

	rc = genl_register_family(&genl_test_family);
	if (rc)
		goto failure;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,14,0)
	init_timer(&timer);
	timer.data = 0;
	timer.function = genl_test_periodic;
	timer.expires = jiffies + msecs_to_jiffies(GENL_TEST_HELLO_INTERVAL);
	add_timer(&timer);
#else
	timer_setup(&timer, genl_test_periodic, 0);
	mod_timer(&timer, jiffies + msecs_to_jiffies(GENL_TEST_HELLO_INTERVAL));
#endif

	return 0;

failure:
	printk(KERN_DEBUG "genl_test: error occurred in %s\n", __func__);
	return -EINVAL;
}
module_init(genl_test_init);

static void genl_test_exit(void)
{
	del_timer(&timer);
	genl_unregister_family(&genl_test_family);
}
module_exit(genl_test_exit);

MODULE_AUTHOR("Ahmed Zaki <anzaki@gmail.com>");
MODULE_LICENSE("GPL");
