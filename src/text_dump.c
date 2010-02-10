/*
 * Copyright 2008, Intel Corporation
 *
 * This file is part of LatencyTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 *
 * Authors:
 * 	Arjan van de Ven <arjan@linux.intel.com>
 *	Chris Mason <chris.mason@oracle.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <time.h>
#include <wchar.h>
#include <ctype.h>

#include <glib.h>

#include "latencytop.h"

static GList *cursor_e = NULL;
static int done = 0;

static void print_global_list(void)
{
	GList *item;
	struct latency_line *line;
	int i = 1;

	printf("Globals: Cause Maximum Percentage\n");
	item = g_list_first(lines);
	while (item && i < 10) {
		line = item->data;
		item = g_list_next(item);

		if (line->max*0.001 < 0.1)
			continue;
		printf("%s", line->reason);
		printf("\t%5.1f msec        %5.1f %%\n",
				line->max * 0.001,
				(line->time * 100 +0.0001) / total_time);
		i++;
	}
}

static void print_one_backtrace(char *trace)
{
	char *p;
	int pos;
	int after;
	int tabs = 0;

	if (!trace || !trace[0])
		return;
	pos = 16;
	while(*trace && *trace == ' ')
		trace++;

	if (!trace[0])
		return;

	while(*trace) {
		p = strchr(trace, ' ');
		if (p) {
			pos += p - trace + 1;
			*p = '\0';
		}
		if (!tabs) {
			/* we haven't printed anything yet */
			printf("\t\t");
			tabs = 1;
		} else if (pos > 79) {
			/*
			 * we have printed something our line is going to be
			 * long
			 */
			printf("\n\t\t");
			pos = 16 + p - trace + 1;
		}
		printf("%s ", trace);
		if (!p)
			break;

		trace = p + 1;
		if (trace && pos > 70) {
			printf("\n");
			tabs = 0;
			pos = 16;
		}
	}
	printf("\n");
}

static void print_procs()
{
	struct process *proc;
	GList *item;
	double total;

	printf("Process details:\n");
	item = g_list_first(procs);
	while (item) {
		int printit = 0;
		GList *item2;
		struct latency_line *line;
		proc = item->data;
		item = g_list_next(item);

		total = 0.0;

		item2 = g_list_first(proc->latencies);
		while (item2) {
			line = item2->data;
			item2 = g_list_next(item2);
			total = total + line->time;
		}
		item2 = g_list_first(proc->latencies);
		while (item2) {
			char *p;
			char *backtrace;
			line = item2->data;
			item2 = g_list_next(item2);
			if (line->max*0.001 < 0.1)
				continue;
			if (!printit) {
				printf("Process %s (%i) ", proc->name, proc->pid);
				printf("Total: %5.1f msec\n", total*0.001);
				printit = 1;
			}
			printf("\t%s", line->reason);
			printf("\t%5.1f msec        %5.1f %%\n",
				line->max * 0.001,
				(line->time * 100 +0.0001) / total
				);
			print_one_backtrace(line->backtrace);
		}

	}
}

static int done_yet(int time, struct timeval *p1)
{
	int seconds;
	int usecs;
	struct timeval p2;
	gettimeofday(&p2, NULL);
	seconds = p2.tv_sec - p1->tv_sec;
	usecs = p2.tv_usec - p1->tv_usec;

	usecs += seconds * 1000000;
	if (usecs > time * 1000000)
		return 1;
	return 0;
}

void signal_func(int foobie)
{
	done = 1;
}

void start_text_dump(void)
{
	struct timeval now;
	struct tm *tm;
	signal(SIGINT, signal_func);
	signal(SIGTERM, signal_func);

	while (!done) {
		gettimeofday(&now, NULL);
		printf("=============== %s", asctime(localtime(&now.tv_sec)));
		update_list();
		print_global_list();
		print_procs();
		if (done)
			break;
		sleep(10);
	}
}

