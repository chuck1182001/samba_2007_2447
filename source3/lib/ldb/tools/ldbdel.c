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
 *  Component: ldbdel
 *
 *  Description: utility to delete records - modelled on ldapdelete
 *
 *  Author: Andrew Tridgell
 */

#include "includes.h"
#include "ldb/include/includes.h"
#include "ldb/tools/cmdline.h"

static int ldb_delete_recursive(struct ldb_context *ldb, const struct ldb_dn *dn)
{
	int ret, i, total=0;
	const char *attrs[] = { NULL };
	struct ldb_result *res;
	
	ret = ldb_search(ldb, dn, LDB_SCOPE_SUBTREE, "distinguishedName=*", attrs, &res);
	if (ret != LDB_SUCCESS) return -1;

	for (i = 0; i < res->count; i++) {
		if (ldb_delete(ldb, res->msgs[i]->dn) == 0) {
			total++;
		}
	}

	talloc_free(res);

	if (total == 0) {
		return -1;
	}
	printf("Deleted %d records\n", total);
	return 0;
}

static void usage(void)
{
	printf("Usage: ldbdel <options> <DN...>\n");
	printf("Options:\n");
	printf("  -r               recursively delete the given subtree\n");
	printf("  -H ldb_url       choose the database (or $LDB_URL)\n");
	printf("  -o options       pass options like modules to activate\n");
	printf("              e.g: -o modules:timestamps\n");
	printf("\n");
	printf("Deletes records from a ldb\n\n");
	exit(1);
}

int main(int argc, const char **argv)
{
	struct ldb_context *ldb;
	int ret = 0, i;
	struct ldb_cmdline *options;

	ldb_global_init();

	ldb = ldb_init(NULL);

	options = ldb_cmdline_process(ldb, argc, argv, usage);

	if (options->argc < 1) {
		usage();
		exit(1);
	}

	for (i=0;i<options->argc;i++) {
		const struct ldb_dn *dn;

		dn = ldb_dn_explode(ldb, options->argv[i]);
		if (dn == NULL) {
			printf("Invalid DN format\n");
			exit(1);
		}
		if (options->recursive) {
			ret = ldb_delete_recursive(ldb, dn);
		} else {
			ret = ldb_delete(ldb, dn);
			if (ret == 0) {
				printf("Deleted 1 record\n");
			}
		}
		if (ret != 0) {
			printf("delete of '%s' failed - %s\n",
				ldb_dn_linearize(ldb, dn),
				ldb_errstring(ldb));
		}
	}

	talloc_free(ldb);

	return ret;
}
