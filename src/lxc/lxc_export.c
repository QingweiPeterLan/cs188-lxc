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

#include <lxc/lxccontainer.h>

#include "arguments.h"

static char *export_name = NULL;

static int my_parser(struct lxc_arguments* args, int c, char* arg)
{
	switch (c) {
	case 'e': export_name = arg; break;
	}
	return 0;
}

static const struct option my_longopts[] = {
	{"export", required_argument, 0, 'e'},
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

	printf("name: %s, export name: %s\n", my_args.name, export_name);

out:
	return ret;
}
