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

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 4096 * 64);
} ringbuf SEC(".maps");

static __u8 x_offset = 0;
static __u8 x_size = 0;
static __u8 y_offset = 0;
static __u8 y_size = 0;

SEC("hid/raw_event")
int hid_y_event(struct hid_bpf_ctx *ctx)
{
	s32 y;

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

	if (!y_size)
		return 0;

	y = (s32)bpf_hid_get_data(ctx, y_offset, y_size);
	bpf_hid_set_data(ctx, y_offset, y_size, -y);

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
	s32 x;

	if (!x_size)
		return 0;

	x = (s32)bpf_hid_get_data(ctx, x_offset, x_size);
	bpf_hid_set_data(ctx, x_offset, x_size, -x);

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

SEC("hid/event")
int probed_event(struct hid_bpf_ctx *ctx)
{
	u8 *buf;
	const size_t buflen = 50 * sizeof(u8);
	int ret;

	if (ctx->type != HID_BPF_PROBED)
		return 0;

	buf = bpf_ringbuf_reserve(&ringbuf, buflen, 0);
	if (!buf)
		return 0;

	__builtin_memset(buf, 0, buflen);

	bpf_hid_hw_open(ctx);
	buf[0] = 5;
	ret = bpf_hid_raw_request(ctx, buf, 50, HID_FEATURE_REPORT, HID_REQ_GET_REPORT);

	bpf_hid_hw_close(ctx);

	bpf_printk("probed event ret value: %d", ret);
	bpf_printk("buf: %02x %02x %02x",
		   buf[0],
		   buf[1],
		   buf[2]);
	bpf_printk("     %02x %02x %02x",
		   buf[3],
		   buf[4],
		   buf[5]);
	bpf_printk("     %02x %02x %02x ...",
		   buf[6],
		   buf[7],
		   buf[7]);

	bpf_ringbuf_discard(buf, 0);

	return 0;
}

static void process_input_tag(struct hid_bpf_ctx *ctx, struct hid_bpf_parser *parser, u64 *idx)
{
	u32 i;
	u32 offset;

	for (i = 0; i < parser->local.usage_index && i < 16; i++) {
		offset = parser->report.current_offset + i * parser->global.report_size;
		switch (parser->local.usage[i]) {
		case HID_GD_X:
			x_size = parser->global.report_size;
			x_offset = offset;
			break;
		case HID_GD_Y:
			y_size = parser->global.report_size;
			y_offset = offset;
			break;
		}
	}
}

static u64 process_hid_rdesc_item(struct hid_bpf_ctx *ctx, struct hid_bpf_parser *parser, u64 *idx, void *data)
{
	u32 i;
	struct hid_bpf_item *item = &parser->item;

	switch((item->type << 8 ) | item->tag) {
	case (HID_ITEM_TYPE_LOCAL << 8) | HID_LOCAL_ITEM_TAG_USAGE:
		for (i = 0; i < parser->local.usage_index && i < 16; i++) {
			if (i != parser->local.usage_index - 1)
				continue;
			switch (parser->local.usage[i]) {
			/* invert X and Y */
			case HID_GD_X:
				bpf_printk("X at %d", *idx);
				//bpf_hid_set_data(ctx, (*idx + 1) << 3, item->size << 3, HID_GD_Y & HID_USAGE);
				break;
			case HID_GD_Y:
				bpf_printk("Y at %d", *idx);
				//bpf_hid_set_data(ctx, (*idx + 1) << 3, item->size << 3, HID_GD_X & HID_USAGE);
				break;
			}
		}
		break;

	case (HID_ITEM_TYPE_MAIN << 8) | HID_MAIN_ITEM_TAG_INPUT:
		process_input_tag(ctx, parser, idx);
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

	ret = bpf_hid_foreach_rdesc_item(ctx, process_hid_rdesc_item, (void *)0, 0);
	bpf_printk("ret: %d", ret);

	return 0;
}

char _license[] SEC("license") = "GPL";
u32 _version SEC("version") = LINUX_VERSION_CODE;
