/* 
   ldb database library

   Copyright (C) Andrew Tridgell  2004

     ** NOTE! The following LGPL license applies to the ldb
     ** library. This does NOT imply that all of Samba is released
     ** under the LGPL
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
 *  Name: ldb
 *
 *  Component: ldbadd
 *
 *  Description: utility to add records - modelled on ldapadd
 *
 *  Author: Andrew Tridgell
 */

#include "includes.h"
#include "ldb/include/includes.h"
#include "ldb/tools/cmdline.h"

static int failures;

static void usage(void)
{
	printf("Usage: ldbadd <options> <ldif...>\n");
	printf("Options:\n");
	printf("  -H ldb_url       choose the database (or $LDB_URL)\n");
	printf("  -o options       pass options like modules to activate\n");
	printf("              e.g: -o modules:timestamps\n");
	printf("\n");
	printf("Adds records to a ldb, reading ldif the specified list of files\n\n");
	exit(1);
}


/*
  add records from an opened file
*/
static int process_file(struct ldb_context *ldb, FILE *f, int *count)
{
	struct ldb_ldif *ldif;
	int ret = LDB_SUCCESS;

	while ((ldif = ldb_ldif_read_file(ldb, f))) {
		if (ldif->changetype != LDB_CHANGETYPE_ADD &&
		    ldif->changetype != LDB_CHANGETYPE_NONE) {
			fprintf(stderr, "Only CHANGETYPE_ADD records allowed\n");
			break;
		}

		ldif->msg = ldb_msg_canonicalize(ldb, ldif->msg);

		ret = ldb_add(ldb, ldif->msg);
		if (ret != LDB_SUCCESS) {
			fprintf(stderr, "ERR: \"%s\" on DN %s\n", 
				ldb_errstring(ldb), ldb_dn_linearize(ldb, ldif->msg->dn));
			failures++;
		} else {
			(*count)++;
		}
		ldb_ldif_read_free(ldb, ldif);
	}

	return ret;
}



int main(int argc, const char **argv)
{
	struct ldb_context *ldb;
	int i, ret=0, count=0;
	struct ldb_cmdline *options;

	ldb_global_init();

	ldb = ldb_init(NULL);

	options = ldb_cmdline_process(ldb, argc, argv, usage);

	if (options->argc == 0) {
		ret = process_file(ldb, stdin, &count);
	} else {
		for (i=0;i<options->argc;i++) {
			const char *fname = options->argv[i];
			FILE *f;
			f = fopen(fname, "r");
			if (!f) {
				perror(fname);
				exit(1);
			}
			ret = process_file(ldb, f, &count);
			fclose(f);
		}
	}

	talloc_free(ldb);

	printf("Added %d records with %d failures\n", count, failures);
	
	return ret;
}
