// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2021 Benjamin Tissoires
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

#include "bpf_util.h"
#include <bpf/bpf.h>
#include <bpf/libbpf.h>

static char *sysfs_path;
static int sysfs_fd;
static int prog_count = 0;

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
	exit(0);
}

static void usage(const char *prog)
{
	fprintf(stderr,
		"usage: %s /dev/HIDRAW\n\n",
		prog);
}

int main(int argc, char **argv)
{
	struct bpf_prog_info info = {};
	__u32 info_len = sizeof(info);
	const char *optstr = "";
	struct bpf_object *obj;
	struct bpf_program *prog;
	int opt;
	char filename[256];
	int err;

	while ((opt = getopt(argc, argv, optstr)) != -1) {
		switch (opt) {
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
		prog_count++;
	}

	signal(SIGINT, int_exit);
	signal(SIGTERM, int_exit);

	err = bpf_obj_get_info_by_fd(progs[0].fd, &info, &info_len);
	if (err) {
		printf("can't get prog info - %s\n", strerror(errno));
		goto cleanup;
	}

	while(1);

 cleanup:
	for (prog_count--; prog_count >= 0; prog_count--) {
		if (bpf_prog_detach2(progs[prog_count].fd, sysfs_fd, progs[prog_count].type))
			fprintf(stderr, "bpf_prog_detach2: returned %m\n");
	}

	bpf_object__close(obj);
	return err;
}
