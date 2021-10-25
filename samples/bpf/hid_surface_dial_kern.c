/* Copyright (c) 2016 PLUMgrid
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <uapi/linux/bpf.h>
#include <uapi/linux/hid.h>
#include <uapi/linux/bpf_hid.h>
#include <bpf/bpf_helpers.h>

#define max(a, b) ((a) > (b) ? (a) : (b))

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 4096 * 64);
} ringbuf SEC(".maps");

SEC("hid/raw_event")
int hid_x_event(struct hid_bpf_ctx *ctx)
{
	bpf_hid_set_data(ctx, (1 << 3) + 1, 1, 0); /* Touch */
	bpf_hid_set_data(ctx, 4 << 3, 16, 0); /* X */
	bpf_hid_set_data(ctx, 6 << 3, 16, 0); /* Y */
	return 0;
}

/* 72 == 360 / 5 -> 1 report every 5 degrees */
#define NEW_RES 72

SEC("hid/event")
int set_haptic(struct hid_bpf_ctx *ctx)
{
	u8 *buf;
	u16 *res;
	const size_t buflen = 8;
	int ret;

	if (ctx->type != HID_BPF_PROBED && ctx->type != HID_BPF_REMOVE)
		return 0;

	buf = bpf_ringbuf_reserve(&ringbuf, buflen, 0);
	if (!buf)
		return 0;

	bpf_hid_hw_open(ctx);

	buf[0] = 1;  /* report ID */

	ret = bpf_hid_raw_request(ctx, buf, buflen, HID_FEATURE_REPORT, HID_REQ_GET_REPORT);

	bpf_printk("probed/remove event ret value: %d", ret);
	bpf_printk("buf: %02x %02x %02x",
		   buf[0],
		   buf[1],
		   buf[2]);
	bpf_printk("     %02x %02x %02x",
		   buf[3],
		   buf[4],
		   buf[5]);
	bpf_printk("     %02x %02x",
		   buf[6],
		   buf[7]);

	/* whenever resolution multiplier is 72, we have the fixed report descriptor */
	res = (u16*)&buf[1];
	if (*res == 72 && ctx->type == HID_BPF_PROBED) {
//		buf[1] = 72; /* resolution multiplier */
//		buf[2] = 0;  /* resolution multiplier */
//		buf[3] = 0;  /* Repeat Count */
		buf[4] = 3;  /* haptic Auto Trigger */
//		buf[5] = 5;  /* Waveform Cutoff Time */
//		buf[6] = 80; /* Retrigger Period */
//		buf[7] = 0;  /* Retrigger Period */
	} else {
		buf[4] = 0;
	}

	ret = bpf_hid_raw_request(ctx, buf, buflen, HID_FEATURE_REPORT, HID_REQ_SET_REPORT);

	bpf_hid_hw_close(ctx);

	bpf_printk("probed event ret value: %d -> %d", ret, buf[4]);

	bpf_ringbuf_discard(buf, 0);

	return 0;
}

static u64 process_hid_rdesc_item(struct hid_bpf_ctx *ctx, struct hid_bpf_parser *parser_data, u64 *idx, void *data)
{
	static unsigned int cur_page = 0;
	static unsigned int cur_usage = 0;
	static bool convert_rel = false;
	const unsigned phys_mul = max(NEW_RES / 72, 1);
	struct hid_bpf_item *item = &parser_data->item;

	switch(item->type) {
	case HID_ITEM_TYPE_GLOBAL:
		switch (item->tag) {

		/* Change Resolution Multiplier */
		case HID_GLOBAL_ITEM_TAG_PHYSICAL_MAXIMUM:
			if (item->data.u16 == 3600)
				bpf_hid_set_data(ctx, (*idx + 1) << 3, item->size << 3, phys_mul);
			break;
		case HID_GLOBAL_ITEM_TAG_LOGICAL_MAXIMUM:
			if (item->data.u16 == 3600)
				bpf_hid_set_data(ctx, (*idx + 1) << 3, item->size << 3, NEW_RES);
			break;

		/* Store usage page */
		case HID_GLOBAL_ITEM_TAG_USAGE_PAGE:
			cur_page = item->data.u8 << 16;
			break;
		}
		break;

	case HID_ITEM_TYPE_LOCAL:
		switch (item->tag) {
		case HID_LOCAL_ITEM_TAG_USAGE:
			cur_usage = cur_page | item->data.u8;

			switch (cur_usage) {
			case HID_GD_DIAL:
				/* Convert REL_DIAL into REL_WHEEL */
				bpf_hid_set_data(ctx, (*idx + 1) << 3, item->size << 3, HID_GD_WHEEL & 0xffff);
				break;
			case HID_DG_TOUCH:
				/* Convert TOUCH into a button */
				bpf_hid_set_data(ctx, (*idx - 1) << 3, 8, HID_UP_BUTTON >> 16);
				bpf_hid_set_data(ctx, (*idx + 1) << 3, item->size << 3, 2);
				break;
			case HID_GD_X:
			case HID_GD_Y:
				convert_rel = true;
				break;
			}
			break;
		}
		break;

	case HID_ITEM_TYPE_MAIN:
		switch (item->tag) {
		case HID_MAIN_ITEM_TAG_INPUT:
			if (convert_rel) {
				bpf_hid_set_data(ctx, (*idx + 1) << 3, item->size << 3, HID_MAIN_ITEM_RELATIVE | HID_MAIN_ITEM_VARIABLE);
				convert_rel = false;
			}
			break;
		}
		break;
	}

	return 0;
}

SEC("hid/rdesc_fixup")
int hid_rdesc_fixup(struct hid_bpf_ctx *ctx)
{
	int ret;

	if (ctx->type != HID_BPF_RDESC_FIXUP)
		return 0;

	ret = bpf_hid_foreach_rdesc_item(ctx, process_hid_rdesc_item, (void *)0, 0);
	bpf_printk("ret: %d", ret);

	return 0;
}

char _license[] SEC("license") = "GPL";
u32 _version SEC("version") = LINUX_VERSION_CODE;
