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
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <lxc/lxccontainer.h>

#include "conf.h"
#include "arguments.h"

static bool dir_only;
static int dir_level;

static int my_parser(struct lxc_arguments* args, int c, char* arg)
{
	switch (c) {
	case 'L': dir_level = atoi(arg); break;
	case 'd': dir_only = true; break;
	}
	return 0;
}

static const struct option my_longopts[] = {
	{"dir-level", required_argument, 0, 'L'},
	{"dir-only", no_argument, 0, 'd'},
	LXC_COMMON_OPTIONS,
};

static struct lxc_arguments my_args = {
	.progname = "lxc-pd",
	.help     = "\
--name=NAME\n\
\n\
lxc-pd prints directories of a container with the identifier NAME\n\
\n\
Options :\n\
  -n, --name=NAME       NAME of the container\n\
  -L, --dir-level=LEVEL shows only up to LEVEL levels deep in the tree\n\
  -d  --dir-only        shows only the directories in the tree\n",
	.name     = NULL,
	.options  = my_longopts,
	.parser   = my_parser,
	.checker  = NULL,
};

void print_dir(const char *name, int level)
{
	if (dir_level != 0 && level >= dir_level)
		return;

	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(name)))
		return;
	if (!(entry = readdir(dir)))
		return;

	do {
		if (entry->d_type == DT_DIR) {
			char path[1024];
			int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
			path[len] = 0;
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;
			printf("%*s%s/\n", level*2, "", entry->d_name);
			print_dir(path, level+1);
		} else {
			if (dir_only)
				continue;
			printf("%*s%s\n", level*2, "", entry->d_name);
		}
	} while ((entry = readdir(dir)) != NULL);
	closedir(dir);
}

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;

	if (lxc_arguments_parse(&my_args, argc, argv))
		goto out;

	if (!my_args.log_file)
		my_args.log_file = "none";

	const char *name = my_args.name;
	const char *lxcpath = my_args.lxcpath[0];

	struct lxc_container *c;
	c = lxc_container_new(name, lxcpath);
	if (!c)
		goto out;

	if (c->lxc_conf) {
		char *rootpath = c->lxc_conf->rootfs.path;
		print_dir(rootpath, 0);
		ret = EXIT_SUCCESS;
	} else {
		printf("Error: Cannot read of container `%s', permission denied\n", name);
	}

out:
	return ret;
}
