// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2021 Intel Corporation. */
#include <linux/version.h>
#include <uapi/linux/bpf.h>
#include <uapi/linux/hid.h>
#include <uapi/linux/bpf_hid.h>
#include <bpf/bpf_helpers.h>

#include "hid_usi.h"

static const char param_names[USI_NUM_PARAMS][6] = {
	"id",
	"flags",
	"color",
	"width",
	"style",
};

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 4096);
} ringbuf SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__type(key, int);
	__type(value, int);
	__uint(max_entries, USI_NUM_PARAMS);
} cache SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__type(key, int);
	__type(value, int);
	__uint(max_entries, USI_NUM_PARAMS);
} wr_cache SEC(".maps");

struct rep_data {
	int offset;
	int size;
	int idx;
};

static struct rep_data inputs[USI_NUM_PARAMS];
static struct rep_data features[USI_NUM_PARAMS];

SEC("hid/raw_event")
int hid_raw_event(struct hid_bpf_ctx *ctx)
{
	u32 i;
	u32 tmp;
	int val;
	int *wrc, *c;
	u8 *buf;
	u32 flags = 0;
	int offset;
	int size;
	int in_range;
	int touching;
	bool cache_valid;

	if (ctx->event.data[0] != inputs[USI_PEN_IN_RANGE].idx)
		return 0;

	in_range = bpf_hid_get_data(ctx, inputs[USI_PEN_IN_RANGE].offset,
				    inputs[USI_PEN_IN_RANGE].size);
	touching = bpf_hid_get_data(ctx, inputs[USI_PEN_TOUCHING].offset,
				    inputs[USI_PEN_TOUCHING].size);

	bpf_printk("flags: in_range=%d, touching=%d", in_range, touching);

	if (!touching && cache_valid) {
		for (i = USI_PEN_COLOR; i < USI_NUM_PARAMS; i++) {
			val = 0;

			/*
			 * Make a local copy of i so that we can get a
			 * pointer reference to it. i itself must remain
			 * inside a register so that BPF verifier can
			 * calculate its value properly.
			 */
			tmp = i;
			bpf_map_update_elem(&wr_cache, &tmp, &val, 0);
			bpf_map_update_elem(&cache, &tmp, &val, 0);
		}
		cache_valid = false;
	}

	if (!touching)
		return 0;

	for (i = USI_PEN_COLOR; i < USI_NUM_PARAMS; i++) {
		offset = inputs[i].offset;
		size = inputs[i].size;
		val = bpf_hid_get_data(ctx, offset, size);

		/*
		 * Again, make a local copy of i which we can refer via a
		 * pointer to satisfy BPF verifier.
		 */
		tmp = i;

		wrc = bpf_map_lookup_elem(&wr_cache, &tmp);
		c = bpf_map_lookup_elem(&cache, &tmp);
		if (!wrc || !c)
			continue;

		bpf_printk("raw[%d]: %s = %02x", i, param_names[i], val);
		bpf_printk(" (c=%02x, wr=%02x)", *c, *wrc);

		if (*wrc != *c) {
			val = *wrc;

			buf = bpf_ringbuf_reserve(&ringbuf, 16, 0);
			if (!buf)
				continue;

			buf[0] = features[i].idx;
			buf[1] = 1;
			buf[2] = val;

			bpf_hid_raw_request(ctx, buf, 3,
					    HID_FEATURE_REPORT,
					    HID_REQ_SET_REPORT);

			bpf_ringbuf_discard(buf, 0);
		}

		if (val != *c && !cache_valid) {
			bpf_map_update_elem(&cache, &tmp, &val, 0);
			bpf_map_update_elem(&wr_cache, &tmp, &val, 0);
			cache_valid = true;
		} else {
			bpf_hid_set_data(ctx, offset, size, *c);
		}
	}

	return 0;
}

SEC("hid/event")
int set_haptic(struct hid_bpf_ctx *ctx)
{
	return 0;
}

static void process_tag(struct hid_bpf_ctx *ctx, struct rep_data *data,
			struct hid_bpf_parser *parser, u64 *idx)
{
	u32 i;
	int id;
	u32 offset;

	for (i = 0; i < parser->local.usage_index && i < 16; i++) {
		offset = parser->report.current_offset +
			i * parser->global.report_size;

		switch (parser->local.usage[i]) {
		case HID_DG_PEN_COLOR:
			id = USI_PEN_COLOR;
			break;
		case HID_DG_PEN_LINE_WIDTH:
			id = USI_PEN_LINE_WIDTH;
			break;
		case HID_DG_PEN_LINE_STYLE_INK:
			id = USI_PEN_LINE_STYLE;
			break;
		case HID_DG_INRANGE:
			if (parser->local.usage_index == 1)
				continue;

			id = USI_PEN_IN_RANGE;
			break;
		case HID_DG_TIPSWITCH:
			if (parser->local.usage_index == 1)
				continue;

			id = USI_PEN_TOUCHING;
			break;
		default:
			continue;
		}

		data[id].offset = offset + 8;
		data[id].size = parser->global.report_size;
		data[id].idx = parser->report.id;

		bpf_printk("parsed id=%d, offset=%d, idx=%d",
			   id, data[id].offset, data[id].idx);
	}
}

static u64 process_hid_rdesc_item(struct hid_bpf_ctx *ctx,
				  struct hid_bpf_parser *parser, u64 *idx,
				  void *data)
{
	struct hid_bpf_item *item = &parser->item;

	switch (item->type) {
	case HID_ITEM_TYPE_MAIN:
		if (item->tag == HID_MAIN_ITEM_TAG_INPUT)
			process_tag(ctx, inputs, parser, idx);
		if (item->tag == HID_MAIN_ITEM_TAG_FEATURE)
			process_tag(ctx, features, parser, idx);
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
