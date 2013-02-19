/* 
 * kretprobe_simple.c.c
 * Copyright (C) 2013 Chaitanya H. <C@24.IO>
 * Version 1.0: Tue Feb 12 09:40:58 PST 2013
 * 
 * This file is a simple "Hello World" implementation of kretprobe.
 * I am using it to study the Linux TCP/IP stack flow through the Linux kernel.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2 of the License.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * 
 * Code from: samples/kprobes/kretprobe_example.c
 * Machine: 3.2.0-37-generic
 *
 */

#include<linux/module.h> 
#include<linux/version.h> 
#include<linux/kernel.h> 
#include<linux/init.h> 
#include<linux/kprobes.h> 
#include<net/ip.h> 
#include <linux/limits.h>
#include <linux/sched.h>
 
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Chaitanya H <C@24.IO>");
MODULE_DESCRIPTION("Hello World implementation of kprobe jprobe");
MODULE_ALIAS("kprobe_simple");

//Bringing this back just so that this can compile and I can see things. 

#define NIPQUAD(addr) \
	((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[3]

#define NIPQUAD_FMT "%u.%u.%u.%u"

static char func_name[NAME_MAX] = "do_fork";
module_param_string(func, func_name, NAME_MAX, S_IRUGO);
MODULE_PARM_DESC(func, "Function to kretprobe; this module will report the"
                        " function's execution time");


/* per-instance private data */
struct my_data {
        ktime_t entry_stamp;
};

/* Here we use the entry_hanlder to timestamp function entry */
static int entry_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
        struct my_data *data;

        if (!current->mm)
                return 1;       /* Skip kernel threads */

        data = (struct my_data *)ri->data;
        data->entry_stamp = ktime_get();
        return 0;
}

/*
 * Return-probe handler: Log the return value and duration. Duration may turn
 * out to be zero consistently, depending upon the granularity of time
 * accounting on the platform.
 */
static int ret_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
        int retval = regs_return_value(regs);
        struct my_data *data = (struct my_data *)ri->data;
        s64 delta;
        ktime_t now;

        now = ktime_get();
        delta = ktime_to_ns(ktime_sub(now, data->entry_stamp));
        printk(KERN_INFO "%s returned %d and took %lld ns to execute\n",
                        func_name, retval, (long long)delta);
        return 0;
}


static struct kretprobe my_kretprobe = {
        .handler                = ret_handler,
        .entry_handler          = entry_handler,
        .data_size              = sizeof(struct my_data),
        /* Probe up to 20 instances concurrently. */
        .maxactive              = 20,
};

static int __init kretprobe_init(void)
{
        int ret;

        my_kretprobe.kp.symbol_name = func_name;
        ret = register_kretprobe(&my_kretprobe);
        if (ret < 0) {
                printk(KERN_INFO "register_kretprobe failed, returned %d\n",
                                ret);
                return -1;
        }
        printk(KERN_INFO "Planted return probe at %s: %p\n",
                        my_kretprobe.kp.symbol_name, my_kretprobe.kp.addr);
        return 0;
}

static void __exit kretprobe_exit(void)
{
        unregister_kretprobe(&my_kretprobe);
        printk(KERN_INFO "kretprobe at %p unregistered\n",
                        my_kretprobe.kp.addr);

        /* nmissed > 0 suggests that maxactive was set too low. */
        printk(KERN_INFO "Missed probing %d instances of %s\n",
                my_kretprobe.nmissed, my_kretprobe.kp.symbol_name);
}

module_init(kretprobe_init)
module_exit(kretprobe_exit)
MODULE_LICENSE("GPL");





