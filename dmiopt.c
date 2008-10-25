/*
 * Command line handling of dmidecode
 * This file is part of the dmidecode project.
 *
 *   Copyright (C) 2005-2008 Jean Delvare <khali@linux-fr.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <getopt.h>

#include "config.h"
#include "types.h"
#include "util.h"
#include "dmidecode.h"
#include "dmiopt.h"


/* Options are global */
struct opt opt;


/*
 * Handling of option --type
 */

struct type_keyword
{
	const char *keyword;
	const u8 *type;
};

static const u8 opt_type_bios[]={ 0, 13, 255 };
static const u8 opt_type_system[]={ 1, 12, 15, 23, 32, 255 };
static const u8 opt_type_baseboard[]={ 2, 10, 255 };
static const u8 opt_type_chassis[]={ 3, 255 };
static const u8 opt_type_processor[]={ 4, 255 };
static const u8 opt_type_memory[]={ 5, 6, 16, 17, 255 };
static const u8 opt_type_cache[]={ 7, 255 };
static const u8 opt_type_connector[]={ 8, 255 };
static const u8 opt_type_slot[]={ 9, 255 };

static const struct type_keyword opt_type_keyword[]={
	{ "bios", opt_type_bios },
	{ "system", opt_type_system },
	{ "baseboard", opt_type_baseboard },
	{ "chassis", opt_type_chassis },
	{ "processor", opt_type_processor },
	{ "memory", opt_type_memory },
	{ "cache", opt_type_cache },
	{ "connector", opt_type_connector },
	{ "slot", opt_type_slot },
};

static void print_opt_type_list(void)
{
	unsigned int i;

	fprintf(stderr, "Valid type keywords are:\n");
	for(i=0; i<ARRAY_SIZE(opt_type_keyword); i++)
	{
		fprintf(stderr, "  %s\n", opt_type_keyword[i].keyword);
	}
}

static u8 *parse_opt_type(u8 *p, const char *arg)
{
	unsigned int i;

	/* Allocate memory on first call only */
	if(p==NULL)
	{
		p=(u8 *)calloc(256, sizeof(u8));
		if(p==NULL)
		{
			perror("calloc");
			return NULL;
		}
	}

	/* First try as a keyword */
	for(i=0; i<ARRAY_SIZE(opt_type_keyword); i++)
	{
		if(!strcasecmp(arg, opt_type_keyword[i].keyword))
		{
			int j=0;
			while(opt_type_keyword[i].type[j]!=255)
				p[opt_type_keyword[i].type[j++]]=1;
			goto found;
		}
	}

	/* Else try as a number */
	while(*arg!='\0')
	{
		unsigned long val;
		char *next;

		val=strtoul(arg, &next, 0);
		if(next==arg)
		{
			fprintf(stderr, "Invalid type keyword: %s\n", arg);
			print_opt_type_list();
			goto exit_free;
		}
		if(val>0xff)
		{
			fprintf(stderr, "Invalid type number: %lu\n", val);
			goto exit_free;
		}

		p[val]=1;
		arg=next;
		while(*arg==',' || *arg==' ')
			arg++;
	}

found:
	return p;

exit_free:
	free(p);
	return NULL;
}


/*
 * Handling of option --string
 */

/* This lookup table could admittedly be reworked for improved performance.
   Due to the low count of items in there at the moment, it did not seem
   worth the additional code complexity though. */
static const struct string_keyword opt_string_keyword[]={
	{ "bios-vendor", 0, 0x04, NULL, NULL },
	{ "bios-version", 0, 0x05, NULL, NULL },
	{ "bios-release-date", 0, 0x08, NULL, NULL },
	{ "system-manufacturer", 1, 0x04, NULL, NULL },
	{ "system-product-name", 1, 0x05, NULL, NULL },
	{ "system-version", 1, 0x06, NULL, NULL },
	{ "system-serial-number", 1, 0x07, NULL, NULL },
	{ "system-uuid", 1, 0x08, NULL, dmi_system_uuid },
	{ "baseboard-manufacturer", 2, 0x04, NULL, NULL },
	{ "baseboard-product-name", 2, 0x05, NULL, NULL },
	{ "baseboard-version", 2, 0x06, NULL, NULL },
	{ "baseboard-serial-number", 2, 0x07, NULL, NULL },
	{ "baseboard-asset-tag", 2, 0x08, NULL, NULL },
	{ "chassis-manufacturer", 3, 0x04, NULL, NULL },
	{ "chassis-type", 3, 0x05, dmi_chassis_type, NULL },
	{ "chassis-version", 3, 0x06, NULL, NULL },
	{ "chassis-serial-number", 3, 0x07, NULL, NULL },
	{ "chassis-asset-tag", 3, 0x08, NULL, NULL },
	{ "processor-family", 4, 0x06, dmi_processor_family, NULL },
	{ "processor-manufacturer", 4, 0x07, NULL, NULL },
	{ "processor-version", 4, 0x10, NULL, NULL },
	{ "processor-frequency", 4, 0x16, NULL, dmi_processor_frequency },
};

static void print_opt_string_list(void)
{
	unsigned int i;

	fprintf(stderr, "Valid string keywords are:\n");
	for(i=0; i<ARRAY_SIZE(opt_string_keyword); i++)
	{
		fprintf(stderr, "  %s\n", opt_string_keyword[i].keyword);
	}
}

static int parse_opt_string(const char *arg)
{
	unsigned int i;

	if(opt.string)
	{
		fprintf(stderr, "Only one string can be specified\n");
		return -1;
	}

	for(i=0; i<ARRAY_SIZE(opt_string_keyword); i++)
	{
		if(!strcasecmp(arg, opt_string_keyword[i].keyword))
		{
			opt.string=&opt_string_keyword[i];
			return 0;
		}
	}

	fprintf(stderr, "Invalid string keyword: %s\n", arg);
	print_opt_string_list();
	return -1;
}


/*
 * Command line options handling
 */

/* Return -1 on error, 0 on success */
int parse_command_line(int argc, char * const argv[])
{
	int option;
	const char *optstring = "d:hqs:t:uV";
	struct option longopts[]={
		{ "dev-mem", required_argument, NULL, 'd' },
		{ "help", no_argument, NULL, 'h' },
		{ "quiet", no_argument, NULL, 'q' },
		{ "string", required_argument, NULL, 's' },
		{ "type", required_argument, NULL, 't' },
		{ "dump", no_argument, NULL, 'u' },
		{ "dump-bin", required_argument, NULL, 'B' },
		{ "from-dump", required_argument, NULL, 'F' },
		{ "version", no_argument, NULL, 'V' },
		{ 0, 0, 0, 0 }
	};

	while((option=getopt_long(argc, argv, optstring, longopts, NULL))!=-1)
		switch(option)
		{
			case 'B':
				opt.flags|=FLAG_DUMP_BIN;
				opt.dumpfile=optarg;
				break;
			case 'F':
				opt.flags|=FLAG_FROM_DUMP;
				opt.dumpfile=optarg;
				break;
			case 'd':
				opt.devmem=optarg;
				break;
			case 'h':
				opt.flags|=FLAG_HELP;
				break;
			case 'q':
				opt.flags|=FLAG_QUIET;
				break;
			case 's':
				if(parse_opt_string(optarg)<0)
					return -1;
				opt.flags|=FLAG_QUIET;
				break;
			case 't':
				opt.type=parse_opt_type(opt.type, optarg);
				if(opt.type==NULL)
					return -1;
				break;
			case 'u':
				opt.flags|=FLAG_DUMP;
				break;
			case 'V':
				opt.flags|=FLAG_VERSION;
				break;
			case '?':
				switch(optopt)
				{
					case 's':
						fprintf(stderr, "String keyword expected\n");
						print_opt_string_list();
						break;
					case 't':
						fprintf(stderr, "Type number or keyword expected\n");
						print_opt_type_list();
						break;
				}
				return -1;
		}

	if(opt.type!=NULL && opt.string!=NULL)
	{
		fprintf(stderr, "Options --string and --type are mutually exclusive\n");
		return -1;
	}

	if((opt.flags & FLAG_DUMP) && opt.string!=NULL)
	{
		fprintf(stderr, "Options --string and --dump are mutually exclusive\n");
		return -1;
	}

	if((opt.flags & FLAG_DUMP) && (opt.flags & FLAG_QUIET))
	{
		fprintf(stderr, "Options --quiet and --dump are mutually exclusive\n");
		return -1;
	}
	if((opt.flags & FLAG_DUMP_BIN) && (opt.type!=NULL || opt.string!=NULL))
	{
		fprintf(stderr, "Options --dump-bin, --string and --type are mutually exclusive\n");
		return -1;
	}
	if((opt.flags & FLAG_FROM_DUMP) && (opt.flags & FLAG_DUMP_BIN))
	{
		fprintf(stderr, "Options --from-dump and --dump-bin are mutually exclusive\n");
		return -1;
	}

	return 0;
}

void print_help(void)
{
	static const char *help=
		"Usage: dmidecode [OPTIONS]\n"
		"Options are:\n"
		" -d, --dev-mem FILE     Read memory from device FILE (default: " DEFAULT_MEM_DEV ")\n"
		" -h, --help             Display this help text and exit\n"
		" -q, --quiet            Less verbose output\n"
		" -s, --string KEYWORD   Only display the value of the given DMI string\n"
		" -t, --type TYPE        Only display the entries of given type\n"
		" -u, --dump             Do not decode the entries\n"
		"     --dump-bin FILE    Dump the DMI data to a binary file\n"
		"     --from-dump FILE   Read the DMI data from a binary file\n"
		" -V, --version          Display the version and exit\n";

	printf("%s", help);
}
