/* 
 *  Samba Unix/Linux SMB client library 
 *  Distributed SMB/CIFS Server Management Utility 
 *  Local configuration interface
 *  Copyright (C) Michael Adam 2007
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  
 */

/*
 * This currently only an interface to the configuration
 * stored inside the samba registry. In the future there
 * might be support for other configuration backends as well.
 */

/*
 * TODO:
 *
 *  - check uid 0 for write operations
 *  - check for valid parameter names and types (loadparm...) ???
 *  - check for correctness of shares (service_ok) ?
 *  - refactor to use _internal functions for pieces of code
 *
 */

#include "includes.h"
#include "utils/net.h"

extern REGISTRY_OPS smbconf_reg_ops;

/* 
 * usage functions
 */

static int net_conf_list_usage(int argc, const char **argv)
{
	d_printf("USAGE: net conf list\n");
	return -1;
}

static int net_conf_import_usage(int argc, const char**argv)
{
	d_printf("USAGE: net conf import [--test|-T] <filename> [<servicename>]\n"
		 "\t[--test|-T]    testmode - do not act, just print "
		                   "what would be done\n"
		 "\t<servicename>  only import service <servicename>, "
		                   "ignore the rest\n");
	return -1;
}

static int net_conf_listshares_usage(int argc, const char **argv)
{
	d_printf("USAGE: net conf listshares\n");
	return -1;
}

static int net_conf_showshare_usage(int argc, const char **argv)
{
	d_printf("USAGE: net conf showshare <sharename>\n");
	return -1;
}

static int net_conf_addshare_usage(int argc, const char **argv)
{
	d_printf("USAGE: net conf addshare <sharename> <path> "
		 "[writeable={y|N} [guest_ok={y|N} [<comment>]]\n"
		 "\t<sharename>      the new share name.\n"
		 "\t<path>           the path on the filesystem to export.\n"
		 "\twriteable={y|N}  set \"writeable to \"yes\" or "
		 "\"no\" (default) on this share.\n"
		 "\tguest_ok={y|N}   set \"guest ok\" to \"yes\" or "
		 "\"no\" (default)   on this share.\n"
		 "\t<comment>        optional comment for the new share.\n");
	return -1;
}

static int net_conf_delshare_usage(int argc, const char **argv)
{
	d_printf("USAGE: net conf delshare <sharename>\n");
	return -1;
}

static int net_conf_setparm_usage(int argc, const char **argv)
{
	d_printf("USAGE: net conf setparm <section> <param> <type> <value>\n"
		 "\t(Supported types are 'dword' and 'sz' by now.)\n");
	return -1;
}

static int net_conf_getparm_usage(int argc, const char **argv)
{
	d_printf("USAGE: net conf getparm <section> <param>\n");
	return -1;
}

static int net_conf_delparm_usage(int argc, const char **argv)
{
	d_printf("USAGE: net conf delparm <section> <param>\n");
	return -1;
}


/*
 * Helper functions
 */

static char *format_value(TALLOC_CTX *mem_ctx, struct registry_value *value)
{
	char *result = NULL;

	/* what if mem_ctx = NULL? */

	switch (value->type) {
	case REG_DWORD:
		result = talloc_asprintf(mem_ctx, "%d", value->v.dword);
		break;
	case REG_SZ:
	case REG_EXPAND_SZ:
		result = talloc_asprintf(mem_ctx, "%s", value->v.sz.str);
		break;
	case REG_MULTI_SZ: {
                uint32 j;
                for (j = 0; j < value->v.multi_sz.num_strings; j++) {
                        result = talloc_asprintf(mem_ctx, "\"%s\" ", 
						 value->v.multi_sz.strings[j]);
                }
                break;
        }
	 case REG_BINARY:
                result = talloc_asprintf(mem_ctx, "binary (%d bytes)",
					 (int)value->v.binary.length);
                break;
        default:
                result = talloc_asprintf(mem_ctx, "<unprintable>");
                break;
        }
	return result;
}

/*
 * add a value to a key. 
 */
static WERROR reg_setvalue_internal(struct registry_key *key, 
				    const char *valname,
				    const char *valtype,
				    const char *valstr)
{
	struct registry_value val;
	WERROR werr = WERR_OK;

	ZERO_STRUCT(val);

	if (strequal(valtype, "dword")) {
		val.type = REG_DWORD;
		val.v.dword = strtoul(valstr, NULL, 10);
	}
	else if (strequal(valtype, "sz")) {
		val.type = REG_SZ;
		val.v.sz.str = CONST_DISCARD(char *, valstr);
		val.v.sz.len = strlen(valstr) + 1;
	}
	else {
		d_fprintf(stderr, "Sorry, only value types DWORD and SZ implementd currently for setting values.\n");
		goto done;
	}

	werr = reg_setvalue(key, valname, &val);
	if (!W_ERROR_IS_OK(werr)) {
		d_fprintf(stderr,
			  "Error adding value '%s' to "
			  "key '%s': %s\n",
			  valname, key->key->name, dos_errstr(werr));
	}

done:
	return werr;
}

/* 
 * Open a subkey of KEY_SMBCONF (i.e a service)
 * - variant without error output (q = quiet)-
 */
static WERROR smbconf_open_path_q(TALLOC_CTX *ctx, const char *subkeyname,
			 	  uint32 desired_access, 
				  struct registry_key **key)
{
	WERROR werr = WERR_OK;
	char *path = NULL;

	if (subkeyname == NULL) {
		path = talloc_strdup(ctx, KEY_SMBCONF);
	}
	else {
		path = talloc_asprintf(ctx, "%s\\%s", KEY_SMBCONF, subkeyname);
	}

	werr = reg_open_path(ctx, path, desired_access,
			     get_root_nt_token(), key);

	TALLOC_FREE(path);
	return werr;
}

/* 
 * Open a subkey of KEY_SMBCONF (i.e a service)
 * - variant with error output -
 */
static WERROR smbconf_open_path(TALLOC_CTX *ctx, const char *subkeyname,
				uint32 desired_access, 
				struct registry_key **key)
{
	WERROR werr = WERR_OK;

	werr = smbconf_open_path_q(ctx, subkeyname, desired_access, key);
	if (!W_ERROR_IS_OK(werr)) {
		d_fprintf(stderr, "Error opening registry path '%s\\%s': %s\n",
			  KEY_SMBCONF, 
			  (subkeyname == NULL) ? "" : subkeyname, 
			  dos_errstr(werr));
	}

	return werr;
}

/*
 * open the base key KEY_SMBCONF
 */
static WERROR smbconf_open_basepath(TALLOC_CTX *ctx, uint32 desired_access,
			     	    struct registry_key **key)
{
	return smbconf_open_path(ctx, NULL, desired_access, key);
}

/*
 * delete a subkey of KEY_SMBCONF
 */
static WERROR reg_delkey_internal(TALLOC_CTX *ctx, const char *keyname)
{
	WERROR werr = WERR_OK;
	struct registry_key *key = NULL;

	werr = smbconf_open_basepath(ctx, REG_KEY_WRITE, &key);
	if (!W_ERROR_IS_OK(werr)) {
		goto done;
	}

	werr = reg_deletekey(key, keyname);
	if (!W_ERROR_IS_OK(werr)) {
		d_fprintf(stderr, "Error deleting registry key %s\\%s: %s\n",
			  KEY_SMBCONF, keyname, dos_errstr(werr));
	}

done:
	TALLOC_FREE(key);
	return werr;
}

/*
 * create a subkey of KEY_SMBCONF
 */
static WERROR reg_createkey_internal(TALLOC_CTX *ctx,
				     const char * subkeyname,
				     struct registry_key **newkey)
{
	WERROR werr = WERR_OK;
	struct registry_key *create_parent = NULL;
	TALLOC_CTX *create_ctx;
	enum winreg_CreateAction action = REG_ACTION_NONE;

	/* create a new talloc ctx for creation. it will hold
	 * the intermediate parent key (SMBCONF) for creation
	 * and will be destroyed when leaving this function... */
	if (!(create_ctx = talloc_new(ctx))) {
		werr = WERR_NOMEM;
		goto done;
	}

	werr = smbconf_open_basepath(create_ctx, REG_KEY_WRITE, &create_parent);
	if (!W_ERROR_IS_OK(werr)) {
		goto done;
	}

	werr = reg_createkey(ctx, create_parent, subkeyname, 
			     REG_KEY_WRITE, newkey, &action);
	if (W_ERROR_IS_OK(werr) && (action != REG_CREATED_NEW_KEY)) {
		d_fprintf(stderr, "Key '%s' already exists.\n", subkeyname);
		werr = WERR_ALREADY_EXISTS;
	}
	if (!W_ERROR_IS_OK(werr)) {
		d_fprintf(stderr, "Error creating key %s: %s\n",
			 subkeyname, dos_errstr(werr));
	}

done:
	TALLOC_FREE(create_ctx);
	return werr;
}

/*
 * check if a subkey of KEY_SMBCONF of a given name exists
 */
static BOOL smbconf_key_exists(TALLOC_CTX *ctx, const char *subkeyname)
{
	BOOL ret = False;
	WERROR werr = WERR_OK;
	TALLOC_CTX *mem_ctx;
	struct registry_key *key;

	if (!(mem_ctx = talloc_new(ctx))) {
		d_fprintf(stderr, "ERROR: Out of memory...!\n");
		goto done;
	}

	werr = smbconf_open_path_q(mem_ctx, subkeyname, REG_KEY_READ, &key);
	if (W_ERROR_IS_OK(werr)) {
		ret = True;
	}

done:
	TALLOC_FREE(mem_ctx);
	return ret;
}

static BOOL smbconf_value_exists(TALLOC_CTX *ctx, struct registry_key *key,
				 const char *param)
{
	BOOL ret = False;
	WERROR werr = WERR_OK;
	struct registry_value *value = NULL;

	werr = reg_queryvalue(ctx, key, param, &value);	
	if (W_ERROR_IS_OK(werr)) {
		ret = True;
	}

	TALLOC_FREE(value);
	return ret;
}

static WERROR list_values(TALLOC_CTX *ctx, struct registry_key *key)
{
	WERROR werr = WERR_OK;
	uint32 idx = 0;
	struct registry_value *valvalue = NULL;
	char *valname = NULL;

	for (idx = 0;
	     W_ERROR_IS_OK(werr = reg_enumvalue(ctx, key, idx, &valname,
			                        &valvalue));
	     idx++)
	{
		d_printf("\t%s = %s\n", valname, format_value(ctx, valvalue));
	}
	if (!W_ERROR_EQUAL(WERR_NO_MORE_ITEMS, werr)) {
                d_fprintf(stderr, "Error enumerating values: %s\n",
                          dos_errstr(werr));
		goto done;
        }
	werr = WERR_OK;

done:
	return werr; 
}

static int import_process_service(TALLOC_CTX *ctx, 
				  struct share_params *share)
{
	int ret = -1;
	struct parm_struct *parm;
	int pnum = 0;
	const char *servicename;
	struct registry_key *key;
	WERROR werr;
	const char *valtype = NULL;
	char *valstr = NULL;

	servicename = (share->service == GLOBAL_SECTION_SNUM)?
		GLOBAL_NAME : lp_servicename(share->service);

	if (opt_testmode) {
		d_printf("[%s]\n", servicename);
	}
	else {
		if (smbconf_key_exists(ctx, servicename)) {
			werr = reg_delkey_internal(ctx, servicename);
			if (!W_ERROR_IS_OK(werr)) {
				goto done;
			}
		}
		werr = reg_createkey_internal(ctx, servicename, &key);
		if (!W_ERROR_IS_OK(werr)) {
			goto done;
		}
	}

	while ((parm = lp_next_parameter(share->service, &pnum, 0)))
	{
		void *ptr = parm->ptr;
		int i = 0;

		if ((share->service < 0 && parm->p_class == P_LOCAL) 
		    && !(parm->flags & FLAG_GLOBAL))
			continue;

		if (parm->p_class == P_LOCAL && share->service >= 0) {
			ptr = lp_local_ptr(share->service, ptr);
		}

		valtype = "sz";

		switch (parm->type) {
		case P_CHAR:
			valstr = talloc_asprintf(ctx, "%c", *(char *)ptr);
			break;
		case P_STRING:
		case P_USTRING:
			valstr = talloc_asprintf(ctx, "%s", *(char **)ptr);
			break;
		case P_GSTRING:
		case P_UGSTRING:
			valstr = talloc_asprintf(ctx, "%s", (char *)ptr);
			break;
		case P_BOOL:
			valstr = talloc_asprintf(ctx, "%s", 
						BOOLSTR(*(BOOL *)ptr));
			break;
		case P_BOOLREV:
			valstr = talloc_asprintf(ctx, "%s", 
						BOOLSTR(!*(BOOL *)ptr));
			break;
		case P_ENUM:
                	for (i = 0; parm->enum_list[i].name; i++) {
                	        if (*(int *)ptr == 
				    parm->enum_list[i].value) 
				{
					valstr = talloc_asprintf(ctx, "%s",
                	                         parm->enum_list[i].name);
                	                break;
                	        }
                	}
			break;
		case P_OCTAL:
			talloc_asprintf(ctx, "%s", octal_string(*(int *)ptr));
			break;
		case P_LIST:
			valstr = talloc_strdup(ctx, "");
			if ((char ***)ptr && *(char ***)ptr) {
				char **list = *(char ***)ptr;
				for (; *list; list++) {
					/* surround strings with whitespace 
					 * in double quotes */
					if (strchr_m(*list, ' '))
					{
						valstr = talloc_asprintf_append(
							valstr, "\"%s\"%s", 
							*list, 
							 ((*(list+1))?", ":""));
					}
					else {
						valstr = talloc_asprintf_append(
							valstr, "%s%s", *list, 
							 ((*(list+1))?", ":""));
					}
				}
			}
			break;
		case P_INTEGER:
			valtype = "dword";
			talloc_asprintf(ctx, "%d", *(int *)ptr);
			break;
		case P_SEP:
			break;
		default:
			valstr = talloc_asprintf(ctx, "<type unimplemented>\n");
			break;
		}

		if (parm->type != P_SEP) {
			if (opt_testmode) {
				d_printf("\t%s = %s\n", parm->label, valstr);
			}
			else {
				werr = reg_setvalue_internal(key, parm->label, 
							     valtype, valstr);
				if (!W_ERROR_IS_OK(werr)) {
					goto done;
				}
			}
		}
	}

	if (opt_testmode) {
		d_printf("\n");
	}

	ret = 0;
done:
	return ret;
}


/*
 * the conf functions 
 */

int net_conf_list(int argc, const char **argv)
{
	WERROR werr = WERR_OK;
	int ret = -1;
	TALLOC_CTX *ctx;
	struct registry_key *base_key = NULL;
	struct registry_key *sub_key = NULL;
	uint32 idx_key = 0;
	char *subkey_name = NULL;

	ctx = talloc_init("list");

	if (argc != 0) {
		net_conf_list_usage(argc, argv);
		goto done;
	}

	werr = smbconf_open_basepath(ctx, REG_KEY_READ, &base_key);
	if (!W_ERROR_IS_OK(werr)) {
		goto done;
	}

	if (smbconf_key_exists(ctx, GLOBAL_NAME))  {
		werr = reg_openkey(ctx, base_key, GLOBAL_NAME, 
				   REG_KEY_READ, &sub_key);
		if (!W_ERROR_IS_OK(werr)) {
			d_fprintf(stderr, "Error opening subkey '%s' : %s\n",
				  subkey_name, dos_errstr(werr));
			goto done;
		}
		d_printf("[%s]\n", GLOBAL_NAME);
		if (!W_ERROR_IS_OK(list_values(ctx, sub_key))) {
			goto done;
		}
		d_printf("\n");
	}

	for (idx_key = 0;
	     W_ERROR_IS_OK(werr = reg_enumkey(ctx, base_key, idx_key,
			     		      &subkey_name, NULL));
	     idx_key++) 
	{
		if (strequal(subkey_name, GLOBAL_NAME)) {
			continue;
		}
		d_printf("[%s]\n", subkey_name);

		werr = reg_openkey(ctx, base_key, subkey_name, 
				   REG_KEY_READ, &sub_key);
		if (!W_ERROR_IS_OK(werr)) {
			d_fprintf(stderr, 
				  "Error opening subkey '%s': %s\n",
				  subkey_name, dos_errstr(werr));
			goto done;
		}
		if (!W_ERROR_IS_OK(list_values(ctx, sub_key))) {
			goto done;
		}
		d_printf("\n");
	}
	if (!W_ERROR_EQUAL(WERR_NO_MORE_ITEMS, werr)) {
		d_fprintf(stderr, "Error enumerating subkeys: %s\n", 
			  dos_errstr(werr));
		goto done;
	}

	ret = 0;

done:
	TALLOC_FREE(ctx);
	return ret;
}

int net_conf_import(int argc, const char **argv)
{
	int ret = -1;
	const char *filename = NULL;
	const char *servicename = NULL;
	BOOL service_found = False;
	TALLOC_CTX *ctx;
	struct share_iterator *shares;
	struct share_params *share;
	struct share_params global_share = { GLOBAL_SECTION_SNUM };

	ctx = talloc_init("net_conf_import");

	switch (argc) {
		case 0:
		default:
			net_conf_import_usage(argc, argv);
			goto done;
		case 2:
			servicename = argv[1];
		case 1:
			filename = argv[0];
			break;
	}

	DEBUG(3,("net_conf_import: reading configuration from file %s.\n",
		filename));

	/* TODO: check for existence and readability */

	if (!lp_load(filename, 
		     False,     /* global_only */
		     True,      /* save_defaults */
		     False,     /* add_ipc */
		     True))     /* initialize_globals */
	{
		d_fprintf(stderr, "Error parsing configuration file.\n");
		goto done;
	}

	if (opt_testmode) {
		d_printf("\nTEST MODE - would import the following configuration:\n\n");
	}

	if ((servicename == NULL) || strequal(servicename, GLOBAL_NAME)) {
		service_found = True;
		if (import_process_service(ctx, &global_share) != 0) {
			goto done;
		}
	}

	if (service_found && (servicename != NULL)) {
		ret = 0;
		goto done;
	}

	if (!(shares = share_list_all(ctx))) {
		d_fprintf(stderr, "Could not list shares...\n");
		goto done;
	}
	while ((share = next_share(shares)) != NULL) {
		if ((servicename == NULL) 
		    || strequal(servicename, lp_servicename(share->service))) 
		{
			service_found = True;
			if (import_process_service(ctx, share)!= 0) {
				goto done;
			}
		}
	}
	
	if ((servicename != NULL) && !service_found) {
		d_printf("Share %s not found in file %s\n", 
			 servicename, filename);
		goto done;

	}

	ret = 0;
	
done:
	TALLOC_FREE(ctx);
	return ret;
}

int net_conf_listshares(int argc, const char **argv)
{
	WERROR werr = WERR_OK;
	int ret = -1;
	struct registry_key *key;
	uint32 idx = 0;
	char *subkey_name = NULL;
	TALLOC_CTX *ctx;

	ctx = talloc_init("listshares");

	if (argc != 0) {
		net_conf_listshares_usage(argc, argv);
		goto done;
	}

	werr = smbconf_open_basepath(ctx, SEC_RIGHTS_ENUM_SUBKEYS, &key);
	if (!W_ERROR_IS_OK(werr)) {
		goto done;
	}

	for (idx = 0;
	     W_ERROR_IS_OK(werr = reg_enumkey(ctx, key, idx,
			    		      &subkey_name, NULL));
	     idx++) 
	{
		d_printf("%s\n", subkey_name);
	}
	if (! W_ERROR_EQUAL(WERR_NO_MORE_ITEMS, werr)) {
		d_fprintf(stderr, "Error enumerating subkeys: %s\n", 
			  dos_errstr(werr));
		goto done;
	}

	ret = 0;

done:
	TALLOC_FREE(ctx);
	return ret;
}

int net_conf_showshare(int argc, const char **argv)
{
	int ret = -1;
	WERROR werr = WERR_OK;
	struct registry_key *key = NULL;
	TALLOC_CTX *ctx;

	ctx = talloc_init("showshare");

	if (argc != 1) {
		net_conf_showshare_usage(argc, argv);
		goto done;
	}

	werr = smbconf_open_path(ctx, argv[0], REG_KEY_READ, &key);
	if (!W_ERROR_IS_OK(werr)) {
		goto done;
	}

	d_printf("[%s]\n", argv[0]);

	if (!W_ERROR_IS_OK(list_values(ctx, key))) {
		goto done;
	}

	ret = 0;

done:
	TALLOC_FREE(ctx);
	return ret;
}

int net_conf_addshare(int argc, const char **argv)
{
	int ret = -1;
	WERROR werr = WERR_OK;
	struct registry_key *newkey = NULL;
	char *sharename = NULL;
	const char *path = NULL;
	const char *comment = NULL;
	const char *guest_ok = "no";
	const char *writeable = "no";
	SMB_STRUCT_STAT sbuf;

	switch (argc) {
		case 0:
		case 1:
		default: 
			net_conf_addshare_usage(argc, argv);
			goto done;
		case 5:
			comment = argv[4];
		case 4:
			if (!strnequal(argv[3], "guest_ok=", 9)) {
				net_conf_addshare_usage(argc, argv);
				goto done;
			}
			switch (argv[3][9]) {
				case 'y':
				case 'Y':
					guest_ok = "yes";
					break;
				case 'n':
				case 'N':
					guest_ok = "no";
					break;
				default: 
					net_conf_addshare_usage(argc, argv);
					goto done;
			}
		case 3:
			if (!strnequal(argv[2], "writeable=", 10)) {
				net_conf_addshare_usage(argc, argv);
				goto done;
			}
			switch (argv[2][10]) {
				case 'y':
				case 'Y':
					writeable = "yes";
					break;
				case 'n':
				case 'N':
					writeable = "no";
					break;
				default:
					net_conf_addshare_usage(argc, argv);
					goto done;
			}

		case 2:
			path = argv[1];
			sharename = strdup_lower(argv[0]);
			break;
	}

	/* 
	 * validate arguments 
	 */

	/* validate share name */

	if (!validate_net_name(sharename, INVALID_SHARENAME_CHARS, 
			       strlen(sharename))) 
	{
		d_fprintf(stderr, "ERROR: share name %s contains "
                        "invalid characters (any of %s)\n",
                        sharename, INVALID_SHARENAME_CHARS);
		goto done;
	}

	if (getpwnam(sharename)) {
		d_fprintf(stderr, "ERROR: share name %s is already a valid "
			  "system user name.\n", sharename);
		goto done;
	}

	if (strequal(sharename, GLOBAL_NAME)) {
		d_fprintf(stderr, 
			  "ERROR: 'global' is not a valid share name.\n");
		goto done;
	}

	/* validate path */

	if (path[0] != '/') {
		d_fprintf(stderr, 
			  "Error: path '%s' is not an absolute path.\n",
			  path);
		goto done;
	}

	if (sys_stat(path, &sbuf) != 0) {
		d_fprintf(stderr,
			  "ERROR: cannot stat path '%s' to ensure "
			  "this is a directory.\n"
			  "Error was '%s'.\n", 
			  path, strerror(errno));
		goto done;
	}

	if (!S_ISDIR(sbuf.st_mode)) {
		d_fprintf(stderr,
			  "ERROR: path '%s' is not a directory.\n",
			  path);
		goto done;
	}

	/* 
	 * create the share 
	 */

	werr = reg_createkey_internal(NULL, argv[0], &newkey);
	if (!W_ERROR_IS_OK(werr)) {
		goto done;
	}

	/* add config params as values */

	werr = reg_setvalue_internal(newkey, "path", "sz", path);
	if (!W_ERROR_IS_OK(werr))
		goto done;

	if (comment != NULL) {
		werr = reg_setvalue_internal(newkey, "comment", "sz", comment);
		if (!W_ERROR_IS_OK(werr))
			goto done;
	}

	werr = reg_setvalue_internal(newkey, "guest ok", "sz", guest_ok);
	if (!W_ERROR_IS_OK(werr))
		goto done;
	
	werr = reg_setvalue_internal(newkey, "writeable", "sz", writeable);
	if (!W_ERROR_IS_OK(werr))
		goto done;

	ret = 0;

done:
	TALLOC_FREE(newkey);
	SAFE_FREE(sharename);
	return ret;
}

int net_conf_delshare(int argc, const char **argv)
{
	int ret = -1;
	const char *sharename = NULL;

	if (argc != 1) {
		net_conf_delshare_usage(argc, argv);
		goto done;
	}
	sharename = argv[0];
	
	if (W_ERROR_IS_OK(reg_delkey_internal(NULL, sharename))) {
		ret = 0;
	}
done:
	return ret;
}

static int net_conf_setparm(int argc, const char **argv)
{
	int ret = -1;
	WERROR werr = WERR_OK;
	struct registry_key *key = NULL;
	char *service = NULL;
	char *param = NULL;
	char *type = NULL;
	const char *value_str = NULL;
	TALLOC_CTX *ctx;

	ctx = talloc_init("setparm");

	if (argc != 4) {
		net_conf_setparm_usage(argc, argv);
		goto done;
	}
	service = strdup_lower(argv[0]);
	param = strdup_lower(argv[1]);
	type = strdup_lower(argv[2]);
	value_str = argv[3];

	if (!smbconf_key_exists(ctx, service)) {
		werr = reg_createkey_internal(ctx, service, &key);
	}
	else {
		werr = smbconf_open_path(ctx, service, REG_KEY_READ, &key);
	}
	if (!W_ERROR_IS_OK(werr)) {
		goto done;
	}

	werr = reg_setvalue_internal(key, param, type, value_str);
	if (!W_ERROR_IS_OK(werr)) {
		d_fprintf(stderr, "Error setting value '%s': %s\n",
			  param, dos_errstr(werr));
		goto done;
	}


	ret = 0;

done:
	SAFE_FREE(service);
	TALLOC_FREE(ctx);
	return ret;
}

static int net_conf_getparm(int argc, const char **argv)
{
	int ret = -1;
	WERROR werr = WERR_OK;
	struct registry_key *key = NULL;
	char *service = NULL;
	char *param = NULL;
	struct registry_value *value = NULL;
	TALLOC_CTX *ctx;

	ctx = talloc_init("getparm");

	if (argc != 2) {
		net_conf_getparm_usage(argc, argv);
		goto done;
	}
	service = strdup_lower(argv[0]);
	param = strdup_lower(argv[1]);

	if (!smbconf_key_exists(ctx, service)) {
		d_fprintf(stderr, 
			  "ERROR: given service '%s' does not exist.\n",
			  service);
		goto done;
	}

	werr = smbconf_open_path(ctx, service, REG_KEY_READ, &key);
	if (!W_ERROR_IS_OK(werr)) {
		goto done;
	}

	werr = reg_queryvalue(ctx, key, param, &value);
	if (!W_ERROR_IS_OK(werr)) {
		d_fprintf(stderr, "Error querying value '%s': %s.\n",
			  param, dos_errstr(werr));
		goto done;
	}
	
	d_printf("%s\n", format_value(ctx, value));
	
	ret = 0;
done:
	SAFE_FREE(service);
	SAFE_FREE(param);
	TALLOC_FREE(ctx);
	return ret;
}

static int net_conf_delparm(int argc, const char **argv)
{
	int ret = -1;
	WERROR werr = WERR_OK;
	struct registry_key *key = NULL;
	char *service = NULL;
	char *param = NULL;
	TALLOC_CTX *ctx;

	ctx = talloc_init("delparm");

	if (argc != 2) {
		net_conf_delparm_usage(argc, argv);
		goto done;
	}
	service = strdup_lower(argv[0]);
	param = strdup_lower(argv[1]);

	if (!smbconf_key_exists(ctx, service)) {
		d_fprintf(stderr, 
			  "Error: given service '%s' does not exist.\n",
			  service);
		goto done;
	}

	werr = smbconf_open_path(ctx, service, REG_KEY_READ, &key);
	if (!W_ERROR_IS_OK(werr)) {
		goto done;
	}

	if (!smbconf_value_exists(ctx, key, param)) {
		d_fprintf(stderr, 
			  "Error: given parameter '%s' is not set.\n",
			  param);
		goto done;
	}
	werr = reg_deletevalue(key, param);
	if (!W_ERROR_IS_OK(werr)) {
		d_fprintf(stderr, "Error deleting value '%s': %s.\n",
			  param, dos_errstr(werr));
		goto done;
	}

	ret = 0;

done:
	return ret;
}

/*
 * Entry-point for all the CONF functions.
 */

int net_conf(int argc, const char **argv)
{
	int ret = -1;
	int saved_errno = 0;
	struct functable2 func[] = {
		{"list", net_conf_list, 
		 "Dump the complete configuration in smb.conf like format."},
		{"import", net_conf_import,
		 "Import configuration from file in smb.conf format."},
		{"listshares", net_conf_listshares, 
		 "List the registry shares."},
		{"showshare", net_conf_showshare, 
		 "Show the definition of a registry share."},
		{"addshare", net_conf_addshare, 
		 "Create a new registry share."},
		{"delshare", net_conf_delshare, 
		 "Delete a registry share."},
		{"setparm", net_conf_setparm, 
		 "Store a parameter."},
		{"getparm", net_conf_getparm, 
		 "Retrieve the value of a parameter."},
		{"delparm", net_conf_delparm, 
		 "Delete a parameter."},
		{NULL, NULL, NULL}
	};

	REGISTRY_HOOK smbconf_reg_hook = {KEY_SMBCONF, &smbconf_reg_ops};
	
	if (!regdb_init()) {
		saved_errno = errno;
		d_fprintf(stderr, "Can't open the registry");
		if (saved_errno) {
			d_fprintf(stderr, ": %s\n", strerror(saved_errno));
		}
		else {
			d_fprintf(stderr, "!\n");
		}
		goto done;
	}
	reghook_cache_init();
	reghook_cache_add(&smbconf_reg_hook);

	ret = net_run_function2(argc, argv, "net conf", func);

	regdb_close();

done:
	return ret;
}

/* END */
