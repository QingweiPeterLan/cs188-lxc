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

#include <lxc/lxccontainer.h>

#include "conf.h"
#include "arguments.h"

static bool ips;
static bool state;
static bool pid;
static bool stats;
static bool humanize = true;
static char **key = NULL;
static int keys = 0;
static int filter_count = 0;

static int my_parser(struct lxc_arguments* args, int c, char* arg)
{
	char **newk;
	switch (c) {
	case 'c':
		newk = realloc(key, (keys + 1) * sizeof(key[0]));
		if (!newk)
			return -1;
		key = newk;
		key[keys] = arg;
		keys++;
		break;
	case 'i': ips = true; filter_count += 1; break;
	case 's': state = true; filter_count += 1; break;
	case 'p': pid = true; filter_count += 1; break;
	case 'S': stats = true; filter_count += 5; break;
	case 'H': humanize = false; break;
	}
	return 0;
}

static const struct option my_longopts[] = {
	{"config", required_argument, 0, 'c'},
	{"ips", no_argument, 0, 'i'},
	{"state", no_argument, 0, 's'},
	{"pid", no_argument, 0, 'p'},
	{"stats", no_argument, 0, 'S'},
	{"no-humanize", no_argument, 0, 'H'},
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
  -c, --config=KEY      show configuration variable KEY from running container\n\
  -i, --ips             shows the IP addresses\n\
  -p, --pid             shows the process id of the init container\n\
  -S, --stats           shows usage stats\n\
  -H, --no-humanize     shows stats as raw numbers, not humanized\n\
  -s, --state           shows the state of the container\n",
	.name     = NULL,
	.options  = my_longopts,
	.parser   = my_parser,
	.checker  = NULL,
};

int main(int argc, char *argv[])
{
	printf("Within lxc-pd\n");

	int ret = EXIT_FAILURE;

	if (lxc_arguments_parse(&my_args, argc, argv))
		return ret;

	if (!my_args.log_file)
		my_args.log_file = "none";

	const char *name = my_args.name;
	const char *lxcpath = my_args.lxcpath[0];

	printf("Container name: %s\n", name);

	struct lxc_container *c;

	c = lxc_container_new(name, lxcpath);
	printf("rootfs: %s\n", c->lxc_conf->rootfs.path);

	return 0;
}
