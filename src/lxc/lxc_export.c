/*
 * lxc: linux Container library
 *
 * (C) Copyright Qingwei Lan. 2016
 *
 * Authors:
 * Qingwei Lan <qingweilan at gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include <lxc/lxccontainer.h>

#include "arguments.h"

static char *lxc_export_path = "/var/lib/lxcexport/";

static char *export_name = NULL;
static char *snapshot_name = NULL;

static int my_parser(struct lxc_arguments* args, int c, char* arg)
{
	switch (c) {
	case 'e': export_name = arg; break;
	case 's': snapshot_name = arg; break;
	}
	return 0;
}

static const struct option my_longopts[] = {
	{"export", required_argument, 0, 'e'},
	{"snapshot", required_argument, 0, 's'},
	LXC_COMMON_OPTIONS,
};

static struct lxc_arguments my_args = {
	.progname = "lxc-export",
	.help     = "\
--name=NAME\n\
\n\
lxc-export exports a container or snapshot\n\
\n\
Options :\n\
  -n, --name=NAME       NAME of the container\n\
  -s  --snapshot=NAME   NAME of the snapshot of container\n\
  -e, --export=NAME     NAME of the output container\n",
	.name     = NULL,
	.options  = my_longopts,
	.parser   = my_parser,
	.checker  = NULL,
};

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;

	if (lxc_arguments_parse(&my_args, argc, argv))
		goto out;

	if (!my_args.log_file)
		my_args.log_file = "none";

	if (!export_name) {
		printf("%s: missing output name, use --export option\n", my_args.progname);
		goto out;
	}

	const char *name = my_args.name;
	const char *lxcpath = my_args.lxcpath[0];

	struct lxc_container *c;
	c = lxc_container_new(name, lxcpath);
	if (!c) {
		printf("Error: cannot create internal lxc container\n");
		goto out;
	}

	if (!c->is_defined(c)) {
		printf("Error: cannot find container `%s', does not exist or permission denied\n", name);
		goto out;
	}

	if (snapshot_name) {
		struct lxc_snapshot *s;
		int i, n;
		n = c->snapshot_list(c, &s);
		if (n < 0) {
			printf("Error getting snapshots for container%s\n", name);
			goto out;
		}
		if (n == 0) {
			printf("Container %s has no snapshots\n", name);
			goto out;
		}

		int exists = 0;
		for (i = 0; i < n; ++i) {
			if (!strcmp(snapshot_name, s[i].name)) {
				printf("%s (%s) %s\n", s[i].name, s[i].lxcpath, s[i].timestamp);
				exists = 1;
			}
			s[i].free(&s[i]);
		}
		if (!exists) {
			printf("Snapshot `%s' for container %s does not exist\n", snapshot_name, name);
			goto out;
		}
	}

	int mret = mkdir(lxc_export_path, 0700);
	if (mret) {
		if (errno == EACCES) {
			printf("Permission denied, please run as root\n");
			goto out;
		} else if (errno != EEXIST) {
			printf("Error in creating directory %s, %d\n", lxc_export_path, mret);
			goto out;
		}
	} else {
		printf("Successfully created directory %s\n", lxc_export_path);
	}
	

	// temporary, remove later
	if (snapshot_name)
		printf("name: %s, snapshot: %s, export name: %s\n", my_args.name, snapshot_name, export_name);
	else
		printf("name: %s, export name: %s\n", my_args.name, export_name);

out:
	return ret;
}
