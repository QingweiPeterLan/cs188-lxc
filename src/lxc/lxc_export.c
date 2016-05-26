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
#include <errno.h>
#include <dirent.h>

#include <lxc/lxccontainer.h>

#include "log.h"
#include "arguments.h"
#include "utils.h"

static char *lxc_export_path = "/var/lib/lxcexport/";
static bool list_only = false;
static bool delete_only = false;

static int my_parser(struct lxc_arguments* args, int c, char* arg)
{
	switch (c) {
	case 'e': args->exportname = arg; break;
	case 'c': args->createname = arg; break;
	case 'L': list_only = true; break;
	case 'D': delete_only = true; break;
	}
	return 0;
}

static const struct option my_longopts[] = {
	{"export", required_argument, 0, 'e'},
	{"create", required_argument, 0, 'c'},
	{"list", no_argument, 0, 'L'},
	{"delete", no_argument, 0, 'D'},
	LXC_COMMON_OPTIONS,
};

static struct lxc_arguments my_args = {
	.progname = "lxc-export",
	.help     = "\
--name=NAME\n\
\n\
lxc-export exports a container\n\
\n\
Options :\n\
  -n, --name=NAME       NAME of the container\n\
  -c, --create=NAME     NAME of the newly created container\n\
  -e, --export=NAME     NAME of the output container\n\
  -D, --delete          DELETE the specified export\n\
  -L, --list            LIST all exported containers\n",
	.options  = my_longopts,
	.parser   = my_parser,
	.checker  = NULL,
};

static int do_export_container(void);
static int do_export_create_container(void);
static int do_export_list(const char *name, int level);
static int do_export_destroy(void);
static void do_print_container(struct lxc_container *c);

int main(int argc, char *argv[])
{
	int ret = EXIT_FAILURE;

	my_args.name = "";
	if (lxc_arguments_parse(&my_args, argc, argv))
		exit(EXIT_FAILURE);

	printf("global lxcpath: %s\n", my_args.lxcpath[0]);

	if (!my_args.log_file)
		my_args.log_file = "none";

	if (lxc_log_init(NULL, my_args.log_file, my_args.log_priority,
			 my_args.progname, my_args.quiet, my_args.lxcpath[0]))
		exit(EXIT_FAILURE);
	lxc_log_options_no_override();

	if (!list_only && !delete_only && !strcmp(my_args.name, "")
		&& !my_args.exportname && !my_args.createname) {
		printf("%s: please specify name with --name or use --list\n", my_args.progname);
		exit(EXIT_FAILURE);
	}

	if (!list_only) {
		if (!strcmp(my_args.name, "")) {
			printf("%s: missing container name, use --name option\n", my_args.progname);
			exit(EXIT_FAILURE);
		}
		if ((my_args.exportname && my_args.createname) ||
			(my_args.exportname && delete_only) ||
			(my_args.createname && delete_only)) {
			printf("%s: please use only one of --export, --create, --delete\n", my_args.progname);
			exit(EXIT_FAILURE);
		} else if (my_args.exportname) {
			printf("EXPORT CONTAINER\n");
			ret = do_export_container();
		} else if (my_args.createname) {
			printf("CREATE CONTAINER\n");
			ret = do_export_create_container();
		} else if (delete_only) {
			printf("DELETE CONTAINER\n");
			ret = do_export_destroy();
		} else {
			printf("%s: please use at least one of --export, --create, --delete\n", my_args.progname);
			exit(EXIT_FAILURE);
		}
	} else {
		if (strcmp(my_args.name, "") || my_args.exportname || my_args.createname || delete_only) {
			printf("%s: to list exports, please do not use other options\n", my_args.progname);
			exit(EXIT_FAILURE);
		}
		if (do_export_list(lxc_export_path, 0)) {
			printf("* Failed to list all exports\n");
			exit(EXIT_FAILURE);
		} else {
			printf("\n");
			exit(EXIT_SUCCESS);
		}
	}

	// temporary, remove later
	if (my_args.exportname)
		printf("name: %s, export name: %s\n", my_args.name, my_args.exportname);
	else if (my_args.createname)
		printf("name: %s, create name: %s\n", my_args.name, my_args.createname);

	if (ret == EXIT_FAILURE)
		printf("Program failed\n");

	return ret;
}

static int do_export_container(void)
{
	int r = EXIT_SUCCESS;
	struct lxc_container *c;

	// create directory for storing exports
	if (mkdir_p(lxc_export_path, 0700) && errno == EACCES) {
		printf("Permission denied, please run as root\n");
		return EXIT_FAILURE;
	} else {
		printf("Successfully created directory %s\n", lxc_export_path);
	}

	c = lxc_container_new(my_args.name, my_args.lxcpath[0]);

	if (!c) {
		printf("Error: cannot create internal lxc container\n");
		lxc_container_put(c);
		return EXIT_FAILURE;
	}
	if (!c->is_defined(c)) {
		printf("Error: cannot find container `%s', does not exist or permission denied\n", my_args.name);
		lxc_container_put(c);
		return EXIT_FAILURE;
	}

	do_print_container(c);

	printf("[00] RET %d\n", r);
	r = c->export_container(c, my_args.exportname, lxc_export_path, my_args.bdevtype, my_args.fssize);
	printf("[01] RET %d\n", r);

	return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int do_export_create_container(void)
{
	int r = EXIT_SUCCESS;
	struct lxc_container *c;

	c = lxc_container_new(my_args.name, lxc_export_path);

	if (!c) {
		printf("Error: cannot create internal lxc container\n");
		lxc_container_put(c);
		return EXIT_FAILURE;
	}
	if (!c->is_defined(c)) {
		printf("Error: cannot find container `%s', does not exist or permission denied\n", my_args.name);
		lxc_container_put(c);
		return EXIT_FAILURE;
	}

	do_print_container(c);

	printf("[10] RET %d\n", r);
	r = c->export_create_container(c, my_args.createname, my_args.lxcpath[0], my_args.bdevtype, my_args.fssize);
	printf("[11] RET %d\n", r);

	return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int do_export_destroy(void)
{
	int r = EXIT_SUCCESS;
	struct lxc_container *c;

	c = lxc_container_new(my_args.name, lxc_export_path);

	if (!c) {
		printf("Error: cannot create internal lxc container\n");
		lxc_container_put(c);
		return EXIT_FAILURE;
	}
	if (!c->is_defined(c)) {
		printf("Error: cannot find container `%s', does not exist or permission denied\n", my_args.name);
		lxc_container_put(c);
		return EXIT_FAILURE;
	}

	do_print_container(c);

	printf("[20] RET %d\n", r);
	r = c->export_destroy(c);
	printf("[21] RET %d\n", r);

	return r ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int do_export_list(const char *name, int level)
{
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(name)))
		return 1;
	if (!(entry = readdir(dir)))
		return 1;

	bool has_rootfs = false, has_config = false;

	do {
		if (entry->d_type == DT_DIR) {
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
					continue;
			if (level == 0) {
				char path[1024];
				int len = snprintf(path, sizeof(path)-1, "%s/%s", name, entry->d_name);
				path[len] = 0;
				if (do_export_list(path, level+1) == 2)
					printf("%s\t", entry->d_name);
				else
					return 1;
			} else if (level == 1) {
				if (strcmp(entry->d_name, "rootfs") == 0)
					has_rootfs = true;
				// printf("  %s\n", entry->d_name);
			}
		} else {
			if (strcmp(entry->d_name, "config") == 0)
				has_config = true;
			// printf("  %s\n", entry->d_name);
		}
	} while ((entry = readdir(dir)) != NULL);
	closedir(dir);

	if (level == 1 && has_config && has_rootfs)
		return 2;

	return 0;
}

static void do_print_container(struct lxc_container *c)
{
	printf("  * name: %s\n", c->name);
	printf("  * config path: %s\n", c->config_path);
	printf("  * config file: %s\n", c->configfile);
}
