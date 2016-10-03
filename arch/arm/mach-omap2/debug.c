#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/debugfs.h>

struct debug_item;

#define DBG_STRING 0x1

static void misc_debug_populate(struct debug_item *item, struct dentry *d);

struct debug_item {
	const char *name;
	struct debug_item *children;
	int (*func)(bool write, struct debug_item *item, void *data);
	int flags;
	struct dentry *dir;
	void *data;
};

static int dbg_clk_enable(bool write, struct debug_item *item,
			  void *data)
{
	if (write) {
		if (*(u64 *)data)
			clk_enable(item->data);
		else
			clk_disable(item->data);
	}

	return 0;
}

static int dbg_clk_prepare(bool write, struct debug_item *item, void *data)
{
	if (write) {
		if (*(u64 *)data)
			clk_prepare(item->data);
		else
			clk_unprepare(item->data);
	}

	return 0;
}

static int dbg_clk_rate(bool write, struct debug_item *item, void *data)
{
	if (write)
		clk_set_rate(item->data, *(u64 *)data);
	else
		*(u64 *)data = clk_get_rate(item->data);

	return 0;
}

static int dbg_clk_parent(bool write, struct debug_item *item, void *new_parent)
{
	return 0;
}

static struct debug_item dbg_clk_items[] = {
	{ .name = "enable", .func = dbg_clk_enable },
	{ .name = "prepare", .func = dbg_clk_prepare },
	{ .name = "rate", .func = dbg_clk_rate },
	{ .name = "parent", .func = dbg_clk_parent, .flags = DBG_STRING },
	{ NULL },
};

struct clk_item {
	struct clk *clk;
	struct dentry *dentry;
	struct list_head node;
	const char *name;
	int dev_id;
	int clk_id;
	struct debug_item *dbg_item;
};

static LIST_HEAD(dbg_clk_list);

static int parse_clk_name(char *name, int *dev_id, int *clk_id)
{
	char *c;
	char *dip = NULL, *cip = NULL;
	int ret;

	*dev_id = 0;
	*clk_id = 0;

	c = name;
	while (*c) {
		if (*c == ':') {
			*c = 0;
			c++;
			if (!dip)
				dip = c;
			else
				cip = c;
		}
		c++;
	}

	if (dip) {
		ret = kstrtoint(dip, 10, dev_id);
		if (ret) {
			pr_err("%s: bad index: %s\n", __func__, dip);
			return -EINVAL;
		}
	}

	if (cip) {
		ret = kstrtoint(cip, 10, clk_id);
		if (ret) {
			pr_err("%s: bad index: %s\n", __func__, cip);
			return -EINVAL;
		}
	}

	pr_info("%s: parsed: '%s', did=%d, cid=%d\n", __func__,
		name, *dev_id, *clk_id);

	return 0;
}

struct clk_item *find_clk_item(const char *name, int dev_id, int clk_id)
{
	struct clk_item *e;

	pr_info("%s: looking up %s:%d:%d\n", __func__, name, dev_id, clk_id);

	list_for_each_entry(e, &dbg_clk_list, node) {
		pr_info("%s: checking %s:%d:%d\n", __func__, e->name,
			e->dev_id, e->clk_id);
		if (e->dev_id == dev_id && e->clk_id == clk_id &&
		    !strcmp(e->name, name)) {
			return e;
		}
	}

	return NULL;
}

static int dbg_clk_root_destroy(bool write, struct debug_item *item,
				void *data)
{
	int ret;
	char *node_name = data;
	int dev_id, clk_id;
	struct clk_item *e;

	if (!write)
		return -EINVAL;

	pr_info("%s: looking up %s...\n", __func__, node_name);

	ret = parse_clk_name(node_name, &dev_id, &clk_id);
	if (ret)
		return ret;

	e = find_clk_item(node_name, dev_id, clk_id);
	if (!e)
		return -ENOENT;

	debugfs_remove_recursive(e->dentry);
	clk_put(e->clk);
	kfree(e->name);
	list_del(&e->node);
	kfree(e->dbg_item);
	kfree(e);

	return 0;
}

static int dbg_clk_root_create(bool write, struct debug_item *item,
			       void *data)
{
	struct device_node *np = NULL;
	struct clk *clk;
	struct dentry *d;
	struct debug_item *new_clk_items = NULL;
	int i;
	char *node_name = data;
	int dev_id, clk_id;
	int ret;
	char *name = NULL;
	struct clk_item *e = NULL;

	if (!write)
		return -EINVAL;

	pr_info("%s: looking up %s...\n", __func__, node_name);

	ret = parse_clk_name(node_name, &dev_id, &clk_id);
	if (ret)
		return ret;

	if (find_clk_item(node_name, dev_id, clk_id)) {
		ret = -EEXIST;
		goto err;
	}

	name = kasprintf(GFP_KERNEL, "%s:%d:%d", node_name, dev_id, clk_id);
	if (!name)
		return -ENOMEM;

	np = NULL;

	i = dev_id;
	while (i >= 0) {
		np = of_find_node_by_name(np, node_name);
		if (!np) {
			ret = -ENOENT;
			goto err;
		}
		i--;
	}

	pr_info("%s: node=%p\n", __func__, np);

	clk = of_clk_get(np, clk_id);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		goto err;
	}

	pr_info("%s: clk=%p\n", __func__, clk);

	new_clk_items =
		kmemdup(dbg_clk_items, sizeof(*new_clk_items) *
			ARRAY_SIZE(dbg_clk_items), GFP_KERNEL);
	if (!new_clk_items) {
		ret = -ENOMEM;
		goto err;
	}

	e = kzalloc(sizeof(*e), GFP_KERNEL);
	if (!e) {
		ret = -ENOMEM;
		goto err;
	}

	d = debugfs_create_dir(name, item->dir);

	e->name = kstrdup(node_name, GFP_KERNEL);
	e->dev_id = dev_id;
	e->clk_id = clk_id;
	e->clk = clk;
	e->dentry = d;
	e->dbg_item = new_clk_items;

	for (i = 0; i < 3; i++)
		new_clk_items[i].data = clk;

	misc_debug_populate(new_clk_items, d);

	list_add(&e->node, &dbg_clk_list);

	kfree(name);
	return 0;

err:
	kfree(name);
	kfree(e);
	kfree(new_clk_items);
	return ret;
}

static struct debug_item dbg_clk_root_items[] = {
	{ .name = "get", .func = dbg_clk_root_create, .flags = DBG_STRING },
	{ .name = "put", .func = dbg_clk_root_destroy, .flags = DBG_STRING },
	{ NULL },
};

static struct debug_item dbg_root_items[] = {
	{ .name = "clocks", .children = dbg_clk_root_items },
	{ NULL },
};

static int option_get(void *data, u64 *val)
{
	struct debug_item *item = data;

	return item->func(false, item, val);
}

static int option_set(void *data, u64 val)
{
	struct debug_item *item = data;

	return item->func(true, item, &val);
}

DEFINE_SIMPLE_ATTRIBUTE(dbg_int_file_fops, option_get, option_set, "%llu\n");

static int string_file_show(struct seq_file *s, void *data)
{
	struct debug_item *item = data;

	item->func(false, item, NULL);
	return 0;
}

static int string_file_open(struct inode *inode, struct file *file)
{
	return single_open(file, string_file_show, inode->i_private);
}

static ssize_t string_file_write(struct file *file, const char __user *user_buf,
				 size_t count, loff_t *ppos)
{
	char *buf;
	struct debug_item *item =
		((struct seq_file *)file->private_data)->private;
	int ret;

	buf = kmalloc(count, GFP_KERNEL);

	ret = copy_from_user(buf, user_buf, count);
	if (ret)
		goto cleanup;

	buf[count - 1] = 0;

	ret = item->func(true, item, buf);
	if (ret)
		pr_info("%s: err: func returned %d\n", __func__, ret);

cleanup:
	kfree(buf);

	if (ret)
		return ret;

	return count;
}

static const struct file_operations dbg_str_file_fops = {
	.open	= string_file_open,
	.read	= seq_read,
	.write	= string_file_write,
	.llseek	= seq_lseek,
	.release = single_release,
};

static void misc_debug_populate(struct debug_item *item, struct dentry *d)
{
	while (item->name) {
		if (item->func) {
			item->dir = d;
			if (item->flags & DBG_STRING)
				debugfs_create_file(item->name,
						    S_IRUGO | S_IWUSR, d,
						    item, &dbg_str_file_fops);
			else
				debugfs_create_file(item->name,
						    S_IRUGO | S_IWUSR, d,
						    item, &dbg_int_file_fops);
			pr_info("%s: created file %s\n", __func__, item->name);
		} else {
			item->dir = debugfs_create_dir(item->name, d);
			pr_info("%s: created dir %s\n", __func__, item->name);
			misc_debug_populate(item->children, item->dir);
		}
		item++;
	}
}

static int __init misc_debug_init(void)
{
	struct dentry *d;

	pr_info("%s\n", __func__);

	d = debugfs_create_dir("misc_debug", NULL);

	misc_debug_populate(dbg_root_items, d);

	pr_info("%s: done\n", __func__);

	return 0;
}
late_initcall(misc_debug_init);
