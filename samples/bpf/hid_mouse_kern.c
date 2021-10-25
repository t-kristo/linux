/* Copyright (c) 2016 PLUMgrid
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <uapi/linux/bpf.h>
#include <uapi/linux/bpf_hid.h>
#include <bpf/bpf_helpers.h>

SEC("hid/raw_event")
int hid_y_event(struct hid_bpf_ctx *ctx)
{
	s16 y;

	bpf_printk("event: %02x size: %d", ctx->type, ctx->event.size);
	bpf_printk("incoming event: %02x %02x %02x",
		   ctx->event.data[0],
		   ctx->event.data[1],
		   ctx->event.data[2]);
	bpf_printk("                %02x %02x %02x",
		   ctx->event.data[3],
		   ctx->event.data[4],
		   ctx->event.data[5]);
	bpf_printk("                %02x %02x %02x",
		   ctx->event.data[6],
		   ctx->event.data[7],
		   ctx->event.data[8]);

	y = ctx->event.data[3] | (ctx->event.data[4] << 8);

	y = -y;

	ctx->event.data[3] = y & 0xFF;
	ctx->event.data[4] = (y >> 8) & 0xFF;

	bpf_printk("modified event: %02x %02x %02x",
		   ctx->event.data[0],
		   ctx->event.data[1],
		   ctx->event.data[2]);
	bpf_printk("                %02x %02x %02x",
		   ctx->event.data[3],
		   ctx->event.data[4],
		   ctx->event.data[5]);
	bpf_printk("                %02x %02x %02x",
		   ctx->event.data[6],
		   ctx->event.data[7],
		   ctx->event.data[8]);

	return 0;
}

SEC("hid/raw_event")
int hid_x_event(struct hid_bpf_ctx *ctx)
{
	s16 x;

	x = ctx->event.data[1] | (ctx->event.data[2] << 8);

	x = -x;

	ctx->event.data[1] = x & 0xFF;
	ctx->event.data[2] = (x >> 8) & 0xFF;
	return 0;
}

SEC("hid/event")
int test_event(struct hid_bpf_ctx *ctx)
{
	switch (ctx->type) {
	case HID_BPF_RAW_REQUEST:
		bpf_printk("raw_request %d: ", ctx->event.request);
		bpf_printk("raw_request : report id %d(%d) ", ctx->event.data[0], ctx->event.report_type);
		break;
	case HID_BPF_REQUEST:
		bpf_printk("request %d: report id %d(%d) ", ctx->event.request, ctx->event.data[0], ctx->event.report_type);
		break;
	default:
		bpf_printk("unknown request type %d report id %d(%d)", ctx->type, ctx->event.data[0], ctx->event.report_type);
	}

	bpf_printk("data: %02x %02x %02x",
		   ctx->event.data[0],
		   ctx->event.data[1],
		   ctx->event.data[2]);
	bpf_printk("      %02x %02x %02x",
		   ctx->event.data[3],
		   ctx->event.data[4],
		   ctx->event.data[5]);
	bpf_printk("      %02x %02x %02x ...",
		   ctx->event.data[6],
		   ctx->event.data[7],
		   ctx->event.data[8]);
	return 0;
}

SEC("hid/rdesc_fixup")
int hid_rdesc_fixup(struct hid_bpf_ctx *ctx)
{
	if (ctx->type != HID_BPF_RDESC_FIXUP)
		return 0;

	bpf_printk("rdesc: %02x %02x %02x",
		   ctx->event.data[0],
		   ctx->event.data[1],
		   ctx->event.data[2]);
	bpf_printk("       %02x %02x %02x",
		   ctx->event.data[3],
		   ctx->event.data[4],
		   ctx->event.data[5]);
	bpf_printk("       %02x %02x %02x ...",
		   ctx->event.data[6],
		   ctx->event.data[7],
		   ctx->event.data[8]);

	ctx->event.data[39] = 0x31;
	ctx->event.data[41] = 0x30;

	return 0;
}

char _license[] SEC("license") = "GPL";
u32 _version SEC("version") = LINUX_VERSION_CODE;
