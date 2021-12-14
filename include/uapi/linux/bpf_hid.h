/* SPDX-License-Identifier: GPL-2.0-or-later WITH Linux-syscall-note */

/*
 *  HID BPF public headers
 *
 *  Copyright (c) 2021 Benjamin Tissoires
 */

#ifndef _UAPI__LINUX_BPF_HID_H__
#define _UAPI__LINUX_BPF_HID_H__

#include <linux/types.h>
#include <linux/workqueue.h>

#define HID_BPF_MAX_BUFFER_SIZE		16384		/* 16kb */

struct hid_device;

enum hid_bpf_event {
	HID_BPF_UNDEF = 0,
	HID_BPF_RAW_EVENT,
	HID_BPF_RDESC_FIXUP,
	HID_BPF_REQUEST,
	HID_BPF_RAW_REQUEST,
	HID_BPF_OUTPUT_REPORT,
	HID_BPF_PROBED,
	HID_BPF_REMOVE,
};

struct hid_bpf_ctx {
	enum hid_bpf_event type;
	struct hid_device *hdev;
	struct {
		__u8 data[HID_BPF_MAX_BUFFER_SIZE];
		unsigned long size;
		unsigned char report_type;
		unsigned char request;
		int retval;
	} event;
	struct delayed_work work;
};

/* in sync with struct hid_item */
struct hid_bpf_item {
	unsigned  format;
	__u8      size;
	__u8      type;
	__u8      tag;
	union {
	    __u8   u8;
	    __s8   s8;
	    __u16  u16;
	    __s16  s16;
	    __u32  u32;
	    __s32  s32;
	    __u8  *longdata;
	} data;
};

/* in sync with struct hid_global */
struct hid_bpf_global {
	unsigned usage_page;
	__s32    logical_minimum;
	__s32    logical_maximum;
	__s32    physical_minimum;
	__s32    physical_maximum;
	__s32    unit_exponent;
	unsigned unit;
	unsigned report_id;
	unsigned report_size;
	unsigned report_count;
};

#define HID_BPF_MAX_USAGES			12288

/* in sync with struct hid_local */
struct hid_bpf_local {
	unsigned usage[HID_BPF_MAX_USAGES]; /* usage array */
	__u8 usage_size[HID_BPF_MAX_USAGES]; /* usage size array */
	unsigned collection_index[HID_BPF_MAX_USAGES]; /* collection index array */
	unsigned usage_index;
	unsigned usage_minimum;
	unsigned delimiter_depth;
	unsigned delimiter_branch;
};

struct hid_bpf_report {
	unsigned int id;				/* id of this report */
	unsigned int type;				/* report type */
	unsigned int application;			/* application usage for this report */
	unsigned int size;				/* size of the report (bits) */
	unsigned int current_offset;			/* current offset of the parsed element */
};


struct hid_bpf_parser {
	struct hid_bpf_item     item;
	struct hid_bpf_global   global;
	struct hid_bpf_local    local;
	struct hid_bpf_report   report;
	unsigned int            collection;
};

#endif /* _UAPI__LINUX_BPF_HID_H__ */

