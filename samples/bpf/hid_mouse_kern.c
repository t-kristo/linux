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

char _license[] SEC("license") = "GPL";
u32 _version SEC("version") = LINUX_VERSION_CODE;
