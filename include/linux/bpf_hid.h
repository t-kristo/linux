/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _BPF_HID_H
#define _BPF_HID_H

#include <linux/bpf.h>
#include <uapi/linux/bpf.h>

#ifdef CONFIG_BPF_HID
int hid_prog_attach(const union bpf_attr *attr, struct bpf_prog *prog);
int hid_prog_detach(const union bpf_attr *attr);
int hid_prog_query(const union bpf_attr *attr, union bpf_attr __user *uattr);
#else
static inline int hid_prog_attach(const union bpf_attr *attr,
				  struct bpf_prog *prog)
{
	return -EINVAL;
}

static inline int hid_prog_detach(const union bpf_attr *attr)
{
	return -EINVAL;
}

static inline int hid_prog_query(const union bpf_attr *attr,
				 union bpf_attr __user *uattr)
{
	return -EINVAL;
}
#endif

#endif /* _BPF_HID_H */
