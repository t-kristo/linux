// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2021, Intel Corporation
 */
#include <linux/bpf.h>
#include <linux/if_link.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/resource.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "bpf_util.h"
#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include <sys/stat.h>

#include "hid_usi.h"

static char *sysfs_path;
static char *sockname = "/tmp/usi";
static int sysfs_fd;
static int sock_fd;
static int prog_count;
static int cache, wr_cache;

static const struct option long_options[] = {
	{ "help", no_argument, NULL, 'h' },
	{ "sock", required_argument, NULL, 's' },
};

struct prog {
	int fd;
	enum bpf_attach_type type;
};

static struct prog progs[10];

static void int_exit(int sig)
{
	int ret;

	for (prog_count--; prog_count >= 0 ; prog_count--) {
		ret = bpf_prog_detach2(progs[prog_count].fd, sysfs_fd, progs[prog_count].type);
		if (ret)
			fprintf(stderr, "bpf_prog_detach2: returned %m\n");
	}

	close(sysfs_fd);
	close(sock_fd);
	remove(sockname);
	exit(0);
}

static void usage(const char *prog)
{
	fprintf(stderr,
		"usage: %s [-s <sockname>] /dev/HIDRAW\n\n",
		prog);
}

static int param_to_idx(const char *param)
{
	if (!strcmp(param, "color"))
		return USI_PEN_COLOR;
	if (!strcmp(param, "width"))
		return USI_PEN_LINE_WIDTH;
	if (!strcmp(param, "style"))
		return USI_PEN_LINE_STYLE;

	return -EINVAL;
}

static int write_value(const char *param, int value)
{
	int err;
	int idx = param_to_idx(param);

	printf("%s: param=%s (%d), value=%d\n", __func__, param, idx, value);
	err = bpf_map_update_elem(wr_cache, &idx, &value, BPF_ANY);
	if (err) {
		printf("Update failed for %d, err=%d\n", idx, err);
		return err;
	}

	return 0;
}

static int read_value(const char *param)
{
	int value = -ENOENT;
	int idx = param_to_idx(param);

	printf("%s: param=%s (%d)\n", __func__, param, idx);

	if (bpf_map_lookup_elem(cache, &idx, &value))
		printf("Value missing for %d\n", idx);
	else
		printf("Value for %d = %d\n", idx, value);

	return value;
}

static int attach_progs(int argc, char **argv)
{
	char filename[256];
	struct bpf_prog_info info = {};
	__u32 info_len = sizeof(info);
	struct bpf_object *obj;
	struct bpf_program *prog;
	int err = 0;
	char buf[BUFSIZ];
	char param[16];
	char op[8];
	int value;
	int m, n;
	struct sockaddr_un addr;
	struct sockaddr_un from;
	socklen_t from_len = sizeof(from);

	snprintf(filename, sizeof(filename), "%s_kern.o", argv[0]);
	obj = bpf_object__open_file(filename, NULL);
	err = libbpf_get_error(obj);
	if (err) {
		fprintf(stderr, "ERROR: opening BPF object file failed\n");
		obj = NULL;
		err = 1;
		goto cleanup;
	}

	/* load BPF program */
	err = bpf_object__load(obj);
	if (err) {
		fprintf(stderr, "ERROR: loading BPF object file failed\n");
		goto cleanup;
	}

	sysfs_fd = open(sysfs_path, O_RDONLY);

	bpf_object__for_each_program(prog, obj) {
		progs[prog_count].fd = bpf_program__fd(prog);
		progs[prog_count].type = bpf_program__get_expected_attach_type(prog);

		err = bpf_prog_attach(progs[prog_count].fd,
				      sysfs_fd,
				      progs[prog_count].type,
				      0);
		if (err) {
			fprintf(stderr, "bpf_prog_attach: err=%m\n");
			progs[prog_count].fd = 0;
			goto cleanup;
		}
		printf("attached BPF program with FD=%d, type=%d\n",
		       progs[prog_count].fd, progs[prog_count].type);
		prog_count++;
	}

	signal(SIGINT, int_exit);
	signal(SIGTERM, int_exit);

	err = bpf_obj_get_info_by_fd(progs[0].fd, &info, &info_len);
	if (err) {
		printf("can't get prog info - %s\n", strerror(errno));
		goto cleanup;
	}

	cache = bpf_object__find_map_fd_by_name(obj, "cache");
	if (cache < 0) {
		printf("can't get 'cache' shared mem from object - %m\n");
		goto cleanup;
	}

	wr_cache = bpf_object__find_map_fd_by_name(obj, "wr_cache");
	if (wr_cache < 0) {
		printf("can't get 'wr_cache' shared mem from object - %m\n");
		goto cleanup;
	}

	sock_fd = socket(PF_UNIX, SOCK_DGRAM, 0);
	if (sock_fd < 0) {
		perror("socket open error.\n");
		err = sock_fd;
		goto cleanup;
	}

	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, sockname);
	unlink(sockname);

	if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("bind");
		goto cleanup;
	}

	while (1) {
		n = recvfrom(sock_fd, buf, BUFSIZ, 0, (struct sockaddr *)&from,
			     &from_len);
		if (n < 0)
			break;
		buf[n] = 0;

		printf("%s: received '%s'\n", __func__, buf);

		printf("%s: from_len=%d, from=%s\n", __func__, from_len,
		       from.sun_path);

		m = sscanf(buf, "%16s %8s %d", param, op, &value);
		if (m == 2 && strcmp(op, "get") == 0) {
			value = read_value(param);
			sprintf(buf, "%s: %d\n", param, value);
			printf("%s: sending '%s'\n", __func__, buf);
			sendto(sock_fd, buf, strlen(buf) + 1, 0,
			       (struct sockaddr *)&from, from_len);
		} else if (m == 3 && strcmp(op, "set") == 0) {
			err = write_value(param, value);
			sprintf(buf, "%s: %d, err=%d\n", param, value, err);
			printf("%s: sending '%s'\n", __func__, buf);
			sendto(sock_fd, buf, strlen(buf) + 1, 0,
			       (struct sockaddr *)&from, from_len);
		}
	}

	return 0;

 cleanup:
	for (prog_count--; prog_count >= 0; prog_count--) {
		if (bpf_prog_detach2(progs[prog_count].fd, sysfs_fd, progs[prog_count].type))
			fprintf(stderr, "bpf_prog_detach2: returned %m\n");
	}

	bpf_object__close(obj);
	return err;
}

int main(int argc, char **argv)
{
	int opt;

	while ((opt = getopt_long(argc, argv, "s:", long_options,
				  NULL)) != -1) {
		switch (opt) {
		case 's':
			sockname = optarg;
			break;
		default:
			usage(basename(argv[0]));
			return 1;
		}
	}

	if (optind == argc) {
		usage(basename(argv[0]));
		return 1;
	}

	sysfs_path = argv[optind];
	if (!sysfs_path) {
		perror("hidraw");
		return 1;
	}

	printf("sockname: %s\n", sockname);
	printf("sysfs_path: %s\n", sysfs_path);

	return attach_progs(argc, argv);
}
