// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  BPF in HID support for Linux
 *
 *  Copyright (c) 2021 Benjamin Tissoires
 */

/*
 */

#include <linux/filter.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include <uapi/linux/bpf_hid.h>
#include <linux/bpf_hid.h>
#include <linux/hid.h>

#define hid_bpf_rcu_dereference(p) \
        rcu_dereference_protected(p, lockdep_is_held(&hdev->bpf.lock))
#define hid_bpf_rcu_replace_pointer(p, new) \
        rcu_replace_pointer(p, new, lockdep_is_held(&hdev->bpf.lock))

#define BPF_MAX_PROGS 64

static int __hid_bpf_prog_attach_array(struct hid_device *hdev,
				       struct bpf_prog_array **array,
				       struct bpf_prog *prog)
{
	struct bpf_prog_array *old_array;
	struct bpf_prog_array *new_array;
	int ret;

	ret = mutex_lock_interruptible(&hdev->bpf.lock);
	if (ret)
		return ret;

	old_array = hid_bpf_rcu_dereference(*array);
	if (old_array && bpf_prog_array_length(old_array) >= BPF_MAX_PROGS) {
		ret = -E2BIG;
		goto unlock;
	}

	ret = bpf_prog_array_copy(old_array, NULL, prog, 0, &new_array);
	if (ret < 0)
		goto unlock;

	rcu_assign_pointer(*array, new_array);
	bpf_prog_array_free(old_array);

unlock:
	mutex_unlock(&hdev->bpf.lock);
	return ret;

}

static int __hid_bpf_prog_attach(struct hid_device *hdev,
				 struct bpf_prog **target,
				 struct bpf_prog *prog)
{
	int ret;
	struct bpf_prog *old_prog;

	ret = mutex_lock_interruptible(&hdev->bpf.lock);
	if (ret)
		return ret;

	old_prog = hid_bpf_rcu_replace_pointer(*target, prog);

	if (old_prog)
		bpf_prog_put(old_prog);

	mutex_unlock(&hdev->bpf.lock);
	return 0;
}

static int hid_bpf_prog_attach(struct hid_device *hdev, const union bpf_attr *attr, struct bpf_prog *prog)
{
	int ret;

	switch (attr->attach_type) {
	case BPF_HID_RAW_EVENT:
		return __hid_bpf_prog_attach_array(hdev, &hdev->bpf.event_progs, prog);
	case BPF_HID_KERNEL_EVENT:
		return __hid_bpf_prog_attach_array(hdev, &hdev->bpf.kevent_progs, prog);
	case BPF_HID_RDESC_FIXUP:
		ret = __hid_bpf_prog_attach(hdev, &hdev->bpf.rdesc_fixup_prog, prog);
		if (ret)
			return ret;

		return hid_reconnect(hdev);
	}

	return -EINVAL;
}

static int __hid_bpf_prog_detach_array(struct hid_device *hdev,
				       struct bpf_prog_array **array,
				       struct bpf_prog *prog)
{
	struct bpf_prog_array *old_array;
	struct bpf_prog_array *new_array;
	int ret;

	ret = mutex_lock_interruptible(&hdev->bpf.lock);
	if (ret)
		return ret;

	old_array = hid_bpf_rcu_dereference(*array);
	ret = bpf_prog_array_copy(old_array, prog, NULL, 0, &new_array);
	/*
	 * Do not use bpf_prog_array_delete_safe() as we would end up
	 * with a dummy entry in the array, and the we would free the
	 * dummy in hid_bpf_free()
	 */
	if (ret)
		goto unlock;

	rcu_assign_pointer(*array, new_array);
	bpf_prog_array_free(old_array);
	bpf_prog_put(prog);

unlock:
	mutex_unlock(&hdev->bpf.lock);
	return ret;
}

static int __hid_bpf_prog_detach(struct hid_device *hdev,
				 struct bpf_prog **prog)
{
	int ret;
	struct bpf_prog *old_prog;

	ret = mutex_lock_interruptible(&hdev->bpf.lock);
	if (ret)
		return ret;

	old_prog = hid_bpf_rcu_replace_pointer(*prog, NULL);

	if (old_prog)
		bpf_prog_put(old_prog);


	mutex_unlock(&hdev->bpf.lock);
	return 0;
}

int hid_bpf_prog_detach(struct hid_device *hdev, struct bpf_prog *prog)
{
	int ret;

	switch(prog->expected_attach_type) {
	case BPF_HID_RAW_EVENT:
		return __hid_bpf_prog_detach_array(hdev, &hdev->bpf.event_progs, prog);
	case BPF_HID_KERNEL_EVENT:
		return __hid_bpf_prog_detach_array(hdev, &hdev->bpf.kevent_progs, prog);
	case BPF_HID_RDESC_FIXUP:
		if (prog != hdev->bpf.rdesc_fixup_prog)
			return -EINVAL;

		ret = __hid_bpf_prog_detach(hdev, &hdev->bpf.rdesc_fixup_prog);
		if (ret)
			return ret;

		return hid_reconnect(hdev);
	default:
		return -EINVAL;
	}

	return -EINVAL;
}

static int hid_bpf_prog_query(struct hid_device *hdev, const union bpf_attr *attr, union bpf_attr __user *uattr)
{
	__u32 __user *prog_ids = u64_to_user_ptr(attr->query.prog_ids);
	struct bpf_prog_array *progs;
	u32 cnt, flags = 0;
	int ret;

	if (attr->query.query_flags)
		return -EINVAL;

	ret = mutex_lock_interruptible(&hdev->bpf.lock);
	if (ret)
		return ret;

	switch (attr->expected_attach_type) {
	case BPF_HID_RAW_EVENT:
		progs = hid_bpf_rcu_dereference(hdev->bpf.event_progs);
		break;
	case BPF_HID_KERNEL_EVENT:
		progs = hid_bpf_rcu_dereference(hdev->bpf.kevent_progs);
		break;

	default:
		ret = -EINVAL;
		goto unlock;
	}

	cnt = progs ? bpf_prog_array_length(progs) : 0;

	if (copy_to_user(&uattr->query.prog_cnt, &cnt, sizeof(cnt))) {
		ret = -EFAULT;
		goto unlock;
	}

	if (copy_to_user(&uattr->query.attach_flags, &flags, sizeof(flags))) {
		ret = -EFAULT;
		goto unlock;
	}

	if (attr->query.prog_cnt != 0 && prog_ids && cnt)
		ret = bpf_prog_array_copy_to_user(progs, prog_ids, attr->query.prog_cnt);

unlock:
	mutex_unlock(&hdev->bpf.lock);
	return ret;

}

const struct bpf_prog_ops hid_prog_ops = {
};

struct hid_bpf_report_enum {
	unsigned numbered;
	struct hid_bpf_report *report_id_hash[HID_MAX_IDS];
};


struct hid_bpf_parser_and_data {
	struct hid_parser parser;
	struct hid_bpf_parser callback_data;
	struct hid_bpf_report_enum report_enum[HID_REPORT_TYPES];
};

struct hid_bpf_report *hid_bpf_add_report(struct hid_bpf_parser_and_data *data,
					  unsigned int type, unsigned int id,
					  unsigned int application)
{
	struct hid_bpf_report_enum *report_enum = data->report_enum + type;
	struct hid_bpf_report *report;

	if (id >= HID_MAX_IDS)
		return NULL;
	if (report_enum->report_id_hash[id])
		return report_enum->report_id_hash[id];

	report = kzalloc(sizeof(struct hid_bpf_report), GFP_KERNEL);
	if (!report)
		return NULL;

	if (id != 0)
		report_enum->numbered = 1;

	report->id = id;
	report->type = type;
	report->size = 0;
	report->application = application;
	report_enum->report_id_hash[id] = report;

	return report;
}

void hid_bpf_free_reports(struct hid_bpf_parser_and_data *data)
{
	unsigned i, j;

	for (i = 0; i < HID_REPORT_TYPES; i++) {
		struct hid_bpf_report_enum *report_enum = data->report_enum + i;

		for (j = 0; j < HID_MAX_IDS; j++) {
			struct hid_bpf_report *report = report_enum->report_id_hash[j];
			kfree(report);
		}
		memset(report_enum, 0, sizeof(*report_enum));
	}
}

static int hid_bpf_add_field(struct hid_parser *parser, unsigned report_type, unsigned flags)
{
	struct hid_bpf_parser_and_data *data = container_of(parser, struct hid_bpf_parser_and_data, parser);
	struct hid_bpf_report *report;
	unsigned int application = 0;

	report = hid_bpf_add_report(data, report_type, parser->global.report_id, application);
	if (!report) {
		hid_err(parser->device, "hid_bpf_add_report failed\n");
		return -1;
	}

	report->current_offset = report->size;
	report->size += parser->global.report_size * parser->global.report_count;

	/* Total size check: Allow for possible report index byte */
	if (report->size > (HID_MAX_BUFFER_SIZE - 1) << 3) {
		hid_err(parser->device, "report is too long\n");
		return -1;
	}

	data->callback_data.report = *report;

	return 0;
}

static u32 hid_bpf_item_udata(struct hid_item *item)
{
	switch (item->size) {
	case 1: return item->data.u8;
	case 2: return item->data.u16;
	case 4: return item->data.u32;
	}
	return 0;
}

static int hid_bpf_parser_main(struct hid_parser *parser, struct hid_item *item)
{
	__u32 data;
	int ret;

	hid_concatenate_last_usage_page(parser);

	data = hid_bpf_item_udata(item);

	switch (item->tag) {
	case HID_MAIN_ITEM_TAG_BEGIN_COLLECTION:
		/* FIXME: handle open collection */
		break;
	case HID_MAIN_ITEM_TAG_END_COLLECTION:
		/* FIXME: handle end collection */
		break;
	case HID_MAIN_ITEM_TAG_INPUT:
		ret = hid_bpf_add_field(parser, HID_INPUT_REPORT, data);
		break;
	case HID_MAIN_ITEM_TAG_OUTPUT:
		ret = hid_bpf_add_field(parser, HID_OUTPUT_REPORT, data);
		break;
	case HID_MAIN_ITEM_TAG_FEATURE:
		ret = hid_bpf_add_field(parser, HID_FEATURE_REPORT, data);
		break;
	default:
		hid_warn(parser->device, "unknown main item tag 0x%x\n", item->tag);
		ret = 0;
	}

	return ret;
}

static void hid_bpf_store_bpf_parser(struct hid_bpf_parser *data,
		struct hid_item *item, struct hid_parser *parser)
{
	data->item = *(struct hid_bpf_item *)item;
	data->global = *(struct hid_bpf_global *)&parser->global;
	data->local = *(struct hid_bpf_local *)&parser->local;
	if (parser->collection_stack_size)
		data->collection = parser->collection_stack[parser->collection_stack_ptr];
}

BPF_CALL_4(bpf_hid_foreach_rdesc_item, void*, ctx, bpf_callback_t, callback_fn, void *, callback_ctx, u64, flags)
{
	struct hid_bpf_ctx *bpf_ctx = ctx;
	struct hid_bpf_parser_and_data *parser_and_data;
	struct hid_parser *parser;
	struct hid_bpf_parser *callback_data;
	struct hid_item item;
	u8 *data_copy, *start, *end, *next;
	u64 cur_index;
	u64 ret = 0;
	u32 num_elems = 0;

	static int (*dispatch_type[])(struct hid_parser *parser,
				      struct hid_item *item) = {
		hid_bpf_parser_main,
		hid_parser_global,
		hid_parser_local,
		hid_parser_reserved
	};


	if (flags != 0)
		return -EINVAL;

	parser_and_data = vzalloc(sizeof(*parser_and_data));
	if (!parser_and_data) {
		ret = -ENOMEM;
		goto exit;
	}

	parser = &parser_and_data->parser;
	callback_data = &parser_and_data->callback_data;

	parser->device = bpf_ctx->hdev;

	data_copy = kmemdup(bpf_ctx->event.data, HID_BPF_MAX_BUFFER_SIZE, GFP_KERNEL);
	if (!data_copy) {
		ret = -ENOMEM;
		goto exit;
	}

	start = data_copy;
	end = start + bpf_ctx->event.size;

	while((next = hid_rdesc_fetch_item(start, end, &item)) != NULL) {
		cur_index = start - data_copy;
		num_elems++;

		dispatch_type[item.type](parser, &item);

		hid_bpf_store_bpf_parser(callback_data, &item, parser);

		ret = callback_fn((u64)(long)ctx, (u64)(long)callback_data,
				  (u64)(long)&cur_index,
				  (u64)(long)callback_ctx, 0);
		/* return value: 0 - continue, 1 - stop and return */
		if (ret)
			break;

		/* Reset the local parser environment on main items */
		if (item.type == HID_ITEM_TYPE_MAIN)
			memset(&parser->local, 0, sizeof(parser->local));

		/* clear item content */
		memset(&item, 0, sizeof(item));

		start = next;
	}

	hid_bpf_free_reports(parser_and_data);

	kfree(data_copy);

	ret = num_elems;

 exit:
	kfree(parser->collection_stack);
	vfree(parser_and_data);
	return ret;
}

static const struct bpf_func_proto bpf_hid_foreach_rdesc_item_proto = {
	.func      = bpf_hid_foreach_rdesc_item,
	.gpl_only  = true,
	.ret_type  = RET_INTEGER,
	.arg1_type = ARG_PTR_TO_CTX,
	.arg2_type = ARG_PTR_TO_FUNC,
	.arg3_type = ARG_PTR_TO_STACK_OR_NULL,
	.arg4_type = ARG_ANYTHING,
};

BPF_CALL_3(bpf_hid_get_data, void*, ctx, u64, offset, u8, n)
{
	struct hid_bpf_ctx *bpf_ctx = ctx;

	if (n > 32 ||
	    ((offset + n) >> 3) >= HID_BPF_MAX_BUFFER_SIZE)
		return 0;

	return hid_field_extract(bpf_ctx->hdev, bpf_ctx->event.data, offset, n);
}

static const struct bpf_func_proto bpf_hid_get_data_proto = {
	.func      = bpf_hid_get_data,
	.gpl_only  = true,
	.ret_type  = RET_INTEGER,
	.arg1_type = ARG_PTR_TO_CTX,
	.arg2_type = ARG_ANYTHING,
	.arg3_type = ARG_ANYTHING,
};

BPF_CALL_1(bpf_hid_hw_open, void*, ctx)
{
	struct hid_bpf_ctx *bpf_ctx = ctx;

	return hid_hw_open(bpf_ctx->hdev);
}

static const struct bpf_func_proto bpf_hid_hw_open_proto = {
	.func      = bpf_hid_hw_open,
	.gpl_only  = true,
	.ret_type  = RET_INTEGER,
	.arg1_type = ARG_PTR_TO_CTX,
};

BPF_CALL_1(bpf_hid_hw_close, void*, ctx)
{
	struct hid_bpf_ctx *bpf_ctx = ctx;

	hid_hw_close(bpf_ctx->hdev);

	return 0;
}

static const struct bpf_func_proto bpf_hid_hw_close_proto = {
	.func      = bpf_hid_hw_close,
	.gpl_only  = true,
	.ret_type  = RET_VOID,
	.arg1_type = ARG_PTR_TO_CTX,
};

BPF_CALL_5(bpf_hid_raw_request, void*, ctx, void*, buf, u64, size,
		u8, rtype, u8, reqtype)
{
	struct hid_bpf_ctx *bpf_ctx = ctx;
	struct hid_device *hdev = bpf_ctx->hdev;
	u8 *dma_data;
	int ret;

	/* check arguments */
	switch (rtype) {
	case HID_INPUT_REPORT:
	case HID_OUTPUT_REPORT:
	case HID_FEATURE_REPORT:
		break;
	default:
		return -EINVAL;
	}

	switch (reqtype) {
	case HID_REQ_GET_REPORT:
	case HID_REQ_GET_IDLE:
	case HID_REQ_GET_PROTOCOL:
	case HID_REQ_SET_REPORT:
	case HID_REQ_SET_IDLE:
	case HID_REQ_SET_PROTOCOL:
		break;
	default:
		return -EINVAL;
	}

	dma_data = kmemdup(buf, size, GFP_KERNEL);
	if (!dma_data)
		return -ENOMEM;

	ret = hid_hw_raw_request(hdev,
				 dma_data[0],
				 dma_data,
				 size,
				 rtype,
				 reqtype);

	if (ret > 0)
		memcpy(buf, dma_data, ret);

	kfree(dma_data);
	return ret;
}

static const struct bpf_func_proto bpf_hid_raw_request_proto = {
	.func      = bpf_hid_raw_request,
	.gpl_only  = true, /* hid_raw_request is EXPORT_SYMBOL_GPL */
	.ret_type  = RET_INTEGER,
	.arg1_type = ARG_PTR_TO_CTX,
	.arg2_type = ARG_PTR_TO_MEM,
	.arg3_type = ARG_CONST_SIZE_OR_ZERO,
	.arg4_type = ARG_ANYTHING,
	.arg5_type = ARG_ANYTHING,
};

BPF_CALL_4(bpf_hid_set_data, void*, ctx, u64, offset, u8, n, u32, data)
{
	struct hid_bpf_ctx *bpf_ctx = ctx;

	if (n > 32 ||
	    ((offset + n) >> 3) >= HID_BPF_MAX_BUFFER_SIZE)
		return -EINVAL;

	implement(bpf_ctx->hdev, bpf_ctx->event.data, offset, n, data);
	return 0;
}

static const struct bpf_func_proto bpf_hid_set_data_proto = {
	.func      = bpf_hid_set_data,
	.gpl_only  = true,
	.ret_type  = RET_INTEGER,
	.arg1_type = ARG_PTR_TO_CTX,
	.arg2_type = ARG_ANYTHING,
	.arg3_type = ARG_ANYTHING,
	.arg4_type = ARG_ANYTHING,
};

static const struct bpf_func_proto *
hid_func_proto(enum bpf_func_id func_id, const struct bpf_prog *prog)
{
	switch (func_id) {
	case BPF_FUNC_hid_foreach_rdesc_item:
		return &bpf_hid_foreach_rdesc_item_proto;
	case BPF_FUNC_hid_get_data:
		return &bpf_hid_get_data_proto;
	case BPF_FUNC_hid_hw_open:
		return &bpf_hid_hw_open_proto;
	case BPF_FUNC_hid_hw_close:
		return &bpf_hid_hw_close_proto;
	case BPF_FUNC_hid_raw_request:
		return &bpf_hid_raw_request_proto;
	case BPF_FUNC_hid_set_data:
		return &bpf_hid_set_data_proto;
	default:
		return bpf_base_func_proto(func_id);
	}
}

static bool hid_is_valid_access(int off, int size,
				enum bpf_access_type access_type,
				const struct bpf_prog *prog,
				struct bpf_insn_access_aux *info)
{
	/* everything not in ctx is prohibited */
	if (off < 0 || off + size > sizeof(struct hid_bpf_ctx))
		return false;

	switch (off) {
	/* type, hdev are read-only */
	case bpf_ctx_range_till(struct hid_bpf_ctx, type, hdev):
		return access_type == BPF_READ;
	case bpf_ctx_range(struct hid_bpf_ctx, event):
		return true;
	/* everything else is read/write */
	}

	return true;
}

const struct bpf_verifier_ops hid_verifier_ops = {
	.get_func_proto  = hid_func_proto,
	.is_valid_access = hid_is_valid_access
};

static int __hid_bpf_match_sysfs(struct device *dev, const void *data)
{
	struct kernfs_node *kn = dev->kobj.sd;

	return kn == data;
}

static struct hid_device *__hid_bpf_fd_to_hdev(int fd)
{
	struct device *dev;
	struct hid_device *hdev;
	struct fd f = fdget(fd);
	struct inode *inode;
	struct kernfs_node *node;

	if (!f.file) {
		hdev = ERR_PTR(-EBADF);
		goto out;
	}

	inode = file_inode(f.file);
	node = inode->i_private;

	dev = bus_find_device(&hid_bus_type, NULL, kernfs_get_parent(node), __hid_bpf_match_sysfs);

	if (dev)
		hdev = to_hid_device(dev);
	else
		hdev = ERR_PTR(-EINVAL);

 out:
	fdput(f);
	return hdev;
}

static struct hid_bpf_ctx *hid_bpf_allocate(struct hid_device *hdev)
{
	struct hid_bpf_ctx *ctx;

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return ERR_PTR(-ENOMEM);

	ctx->hdev = hdev;

	return ctx;
}

int hid_prog_attach(const union bpf_attr *attr, struct bpf_prog *prog)
{
	struct hid_device *hdev;

	if (attr->attach_flags)
		return -EINVAL;

	hdev = __hid_bpf_fd_to_hdev(attr->target_fd);
	if (IS_ERR(hdev))
		return PTR_ERR(hdev);

	if (!hdev->bpf.ctx) {
		hdev->bpf.ctx = hid_bpf_allocate(hdev);

		if (IS_ERR(hdev->bpf.ctx))
			return PTR_ERR(hdev->bpf.ctx);
	}

	return hid_bpf_prog_attach(hdev, attr, prog);
}
EXPORT_SYMBOL_GPL(hid_prog_attach);

int hid_prog_detach(const union bpf_attr *attr)
{
	struct bpf_prog *prog;
	struct hid_device *hdev;
	int ret;

	if (attr->attach_flags)
		return -EINVAL;

	prog = bpf_prog_get_type(attr->attach_bpf_fd,
				 BPF_PROG_TYPE_HID);
	if (IS_ERR(prog))
		return PTR_ERR(prog);

	hdev = __hid_bpf_fd_to_hdev(attr->target_fd);
	if (IS_ERR(hdev)) {
		ret = PTR_ERR(hdev);
		goto out;
	}

	prog->expected_attach_type = attr->attach_type;

	ret = hid_bpf_prog_detach(hdev, prog);

 out:
	bpf_prog_put(prog);
	return ret;
}
EXPORT_SYMBOL_GPL(hid_prog_detach);

int hid_prog_query(const union bpf_attr *attr, union bpf_attr __user *uattr)
{
	struct hid_device *hdev;

	if (attr->query.query_flags)
		return -EINVAL;

	hdev = __hid_bpf_fd_to_hdev(attr->query.target_fd);
	if (IS_ERR(hdev))
		return PTR_ERR(hdev);

	return hid_bpf_prog_query(hdev, attr, uattr);

}
EXPORT_SYMBOL_GPL(hid_prog_query);

static int hid_bpf_prog_run(struct hid_device *hdev, struct hid_bpf_ctx *ctx,
		enum hid_bpf_event type)
{
	if (!hdev->bpf.kevent_progs)
		return 0;

	ctx->type = type;
	ctx->event.retval = 0;

	BPF_PROG_RUN_ARRAY(hdev->bpf.kevent_progs, ctx, bpf_prog_run);

	return ctx->event.retval;
}

u8 *hid_bpf_raw_event(struct hid_device *hdev, u8 *rd, int *size)
{
	if (!hdev->bpf.ctx)
		return rd;

	if (*size > sizeof(hdev->bpf.ctx->event.data))
		return rd;

	if (!hdev->bpf.event_progs)
		return rd;

	memset(&hdev->bpf.ctx->event, 0, sizeof(hdev->bpf.ctx->event));
	memcpy(hdev->bpf.ctx->event.data, rd, *size);
	hdev->bpf.ctx->event.size = *size;
	hdev->bpf.ctx->type = HID_BPF_RAW_EVENT;

	BPF_PROG_RUN_ARRAY(hdev->bpf.event_progs, hdev->bpf.ctx, bpf_prog_run);

	if (!hdev->bpf.ctx->event.size)
		return ERR_PTR(-EINVAL);

	*size = hdev->bpf.ctx->event.size;

	return hdev->bpf.ctx->event.data;
}


__u8 *hid_bpf_report_fixup(struct hid_device *hdev, __u8 *rdesc,
		unsigned int *size)
{
	struct hid_bpf_ctx *ctx;

	if (!hdev->bpf.ctx)
		goto ignore_bpf;

	if (*size > sizeof(hdev->bpf.ctx->event.data)) {
		hid_warn(hdev, "%s: report too big: %d, ignoring BPF",
			 __func__, *size);
		goto ignore_bpf;
	}

	mutex_lock(&hdev->bpf.lock);

	if (!hdev->bpf.rdesc_fixup_prog) {
		mutex_unlock(&hdev->bpf.lock);
		goto ignore_bpf;
	}

	ctx = hid_bpf_allocate(hdev);
	if (IS_ERR(ctx)){
		mutex_unlock(&hdev->bpf.lock);
		goto ignore_bpf;
	}

	memcpy(ctx->event.data, rdesc, *size);

	ctx->event.size = *size;
	ctx->type = HID_BPF_RDESC_FIXUP;

	bpf_prog_run(hdev->bpf.rdesc_fixup_prog, ctx);

	*size = ctx->event.size;

	if (!*size) {
		rdesc = NULL;
		goto unlock;
	}

	rdesc = kmemdup(ctx->event.data, *size, GFP_KERNEL);

 unlock:
	kfree(ctx);
	mutex_unlock(&hdev->bpf.lock);
	return rdesc;

 ignore_bpf:
	return kmemdup(rdesc, *size, GFP_KERNEL);
}

void hid_bpf_init(struct hid_device *hdev)
{
	mutex_init(&hdev->bpf.lock);
}

void hid_bpf_remove(struct hid_device *hdev)
{
	struct bpf_prog_array_item *item;
	struct bpf_prog_array *event_array, *init_array;

	mutex_lock(&hdev->bpf.lock);

	event_array = hid_bpf_rcu_dereference(hdev->bpf.event_progs);
	init_array = hid_bpf_rcu_dereference(hdev->bpf.kevent_progs);
	if (!event_array && !init_array)
		goto unlock;

	if (event_array) {
		for (item = event_array->items; item->prog; item++)
			bpf_prog_put(item->prog);

		bpf_prog_array_free(event_array);
	}

	if (init_array) {
		for (item = init_array->items; item->prog; item++)
			bpf_prog_put(item->prog);

		bpf_prog_array_free(init_array);
	}

	if (hdev->bpf.rdesc_fixup_prog)
		bpf_prog_put(hdev->bpf.rdesc_fixup_prog);

 unlock:
	mutex_unlock(&hdev->bpf.lock);

	kfree(hdev->bpf.ctx);

	mutex_destroy(&hdev->bpf.lock);
}

int hid_bpf_hw_raw_request(struct hid_device *hdev,
			   unsigned char reportnum, __u8 *buf,
			   size_t len, unsigned char rtype, int reqtype)
{
	struct hid_bpf_ctx *ctx;
	int size;

	if (!hdev->bpf.ctx)
		return len;

	if (len >= sizeof(hdev->bpf.ctx->event.data))
		return len;

	ctx = hid_bpf_allocate(hdev);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	memcpy(ctx->event.data, buf, len);

	ctx->event.size = len;
	ctx->event.request = reqtype;
	ctx->event.report_type = rtype;

	if (hid_bpf_prog_run(hdev, ctx, HID_BPF_RAW_REQUEST) ||
	    !ctx->event.size) {
		kfree(ctx);
		return -EIO;
	}

	size = ctx->event.size;
	memcpy(buf, ctx->event.data, size);

	kfree(ctx);
	return size;
}

int hid_bpf_hw_request(struct hid_device *hdev,
		       struct hid_report *report, int reqtype)
{
	struct hid_bpf_ctx *ctx;
	int len;

	if (!hdev->bpf.ctx)
		return 0;

	len = hid_report_len(report);
	if (len >= sizeof(hdev->bpf.ctx->event.data))
		return 0;

	ctx = hid_bpf_allocate(hdev);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	hid_output_report(report, ctx->event.data);

	ctx->event.size = len;
	ctx->event.request = reqtype;
	ctx->event.report_type = report->type;

	if (hid_bpf_prog_run(hdev, ctx, HID_BPF_REQUEST)) {
		kfree(ctx);
		return -EIO;
	}

	kfree(ctx);

	//FIXME: should convert buf -> report
	return 0;
}

int hid_bpf_hw_output_report(struct hid_device *hdev, __u8 *buf, size_t len)
{
	struct hid_bpf_ctx *ctx;
	int size;

	if (!hdev->bpf.ctx)
		return len;

	if (len >= sizeof(hdev->bpf.ctx->event.data))
		return len;

	ctx = hid_bpf_allocate(hdev);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	memcpy(ctx->event.data, buf, len);

	ctx->event.size = len;

	if (hid_bpf_prog_run(hdev, ctx, HID_BPF_OUTPUT_REPORT) ||
	    !ctx->event.size) {
		kfree(ctx);
		return -EIO;
	}

	size = ctx->event.size;
	memcpy(buf, ctx->event.data, size);
	kfree(ctx);
	return size;
}

int hid_bpf_event(struct hid_device *hdev, enum hid_bpf_event event)
{
	struct hid_bpf_ctx *ctx;
	int ret;

	if (!hdev->bpf.ctx)
		return 0;

	ctx = hid_bpf_allocate(hdev);
	if (IS_ERR(ctx))
		return PTR_ERR(ctx);

	if (hid_bpf_prog_run(hdev, ctx, event))
		ret = -EIO;
	else
		ret = ctx->event.retval;

	kfree(ctx);
	return ret;
}
