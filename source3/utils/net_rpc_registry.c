/* 
   Samba Unix/Linux SMB client library 
   Distributed SMB/CIFS Server Management Utility 

   Copyright (C) Gerald (Jerry) Carter          2005-2006

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */
 
#include "includes.h"
#include "utils/net.h"
#include "regfio.h"
#include "reg_objects.h"

static BOOL reg_hive_key(const char *fullname, uint32 *reg_type,
			 const char **key_name)
{
	const char *sep;
	ptrdiff_t len;

	sep = strchr_m(fullname, '\\');

	if (sep != NULL) {
		len = sep - fullname;
		*key_name = sep+1;
	}
	else {
		len = strlen(fullname);
		*key_name = "";
	}

	if (strnequal(fullname, "HKLM", len) ||
	    strnequal(fullname, "HKEY_LOCAL_MACHINE", len))
		(*reg_type) = HKEY_LOCAL_MACHINE;
	else if (strnequal(fullname, "HKCR", len) ||
		 strnequal(fullname, "HKEY_CLASSES_ROOT", len))
		(*reg_type) = HKEY_CLASSES_ROOT;
	else if (strnequal(fullname, "HKU", len) ||
		 strnequal(fullname, "HKEY_USERS", len))
		(*reg_type) = HKEY_USERS;
	else if (strnequal(fullname, "HKPD", len) ||
		 strnequal(fullname, "HKEY_PERFORMANCE_DATA", len))
		(*reg_type) = HKEY_PERFORMANCE_DATA;
	else {
		DEBUG(10,("reg_hive_key: unrecognised hive key %s\n",
			  fullname));
		return False;
	}

	return True;
}

static NTSTATUS registry_openkey(TALLOC_CTX *mem_ctx,
				 struct rpc_pipe_client *pipe_hnd,
				 const char *name, uint32 access_mask,
				 struct policy_handle *hive_hnd,
				 struct policy_handle *key_hnd)
{
	uint32 hive;
	NTSTATUS status;
	struct winreg_String key;

	if (!reg_hive_key(name, &hive, &key.name)) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	status = rpccli_winreg_Connect(pipe_hnd, mem_ctx, hive, access_mask,
				       hive_hnd);
	if (!(NT_STATUS_IS_OK(status))) {
		return status;
	}

	status = rpccli_winreg_OpenKey(pipe_hnd, mem_ctx, hive_hnd, key, 0,
				       access_mask, key_hnd);
	if (!(NT_STATUS_IS_OK(status))) {
		rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, hive_hnd);
		return status;
	}

	return NT_STATUS_OK;
}

static NTSTATUS registry_enumkeys(TALLOC_CTX *ctx,
				  struct rpc_pipe_client *pipe_hnd,
				  struct policy_handle *key_hnd,
				  uint32 *pnum_keys, char ***pnames,
				  char ***pclasses, NTTIME ***pmodtimes)
{
	TALLOC_CTX *mem_ctx;
	NTSTATUS status;
	uint32 num_subkeys, max_subkeylen, max_classlen;
	uint32 num_values, max_valnamelen, max_valbufsize;
	uint32 i;
	NTTIME last_changed_time;
	uint32 secdescsize;
	struct winreg_String classname;
	char **names, **classes;
	NTTIME **modtimes;

	if (!(mem_ctx = talloc_new(ctx))) {
		return NT_STATUS_NO_MEMORY;
	}

	ZERO_STRUCT(classname);
	status = rpccli_winreg_QueryInfoKey(
		pipe_hnd, mem_ctx, key_hnd, &classname, &num_subkeys,
		&max_subkeylen, &max_classlen, &num_values, &max_valnamelen,
		&max_valbufsize, &secdescsize, &last_changed_time );

	if (!NT_STATUS_IS_OK(status)) {
		goto error;
	}

	if (num_subkeys == 0) {
		*pnum_keys = 0;
		TALLOC_FREE(mem_ctx);
		return NT_STATUS_OK;
	}

	if ((!(names = TALLOC_ZERO_ARRAY(mem_ctx, char *, num_subkeys))) ||
	    (!(classes = TALLOC_ZERO_ARRAY(mem_ctx, char *, num_subkeys))) ||
	    (!(modtimes = TALLOC_ZERO_ARRAY(mem_ctx, NTTIME *,
					    num_subkeys)))) {
		status = NT_STATUS_NO_MEMORY;
		goto error;
	}

	for (i=0; i<num_subkeys; i++) {
		char c, n;
		struct winreg_StringBuf class_buf;
		struct winreg_StringBuf name_buf;
		NTTIME modtime;

		c = '\0';
		class_buf.name = &c;
		class_buf.size = max_classlen+2;

		n = '\0';
		name_buf.name = &n;
		name_buf.size = max_subkeylen+2;

		ZERO_STRUCT(modtime);

		status = rpccli_winreg_EnumKey(pipe_hnd, mem_ctx, key_hnd,
					       i, &name_buf, &class_buf,
					       &modtime);
		
		if (W_ERROR_EQUAL(ntstatus_to_werror(status),
				  WERR_NO_MORE_ITEMS) ) {
			status = NT_STATUS_OK;
			break;
		}
		if (!NT_STATUS_IS_OK(status)) {
			goto error;
		}

		classes[i] = NULL;

		if (class_buf.name &&
		    (!(classes[i] = talloc_strdup(classes, class_buf.name)))) {
			status = NT_STATUS_NO_MEMORY;
			goto error;
		}

		if (!(names[i] = talloc_strdup(names, name_buf.name))) {
			status = NT_STATUS_NO_MEMORY;
			goto error;
		}

		if ((!(modtimes[i] = (NTTIME *)talloc_memdup(
			       modtimes, &modtime, sizeof(modtime))))) {
			status = NT_STATUS_NO_MEMORY;
			goto error;
		}
	}

	*pnum_keys = num_subkeys;

	if (pnames) {
		*pnames = talloc_move(ctx, &names);
	}
	if (pclasses) {
		*pclasses = talloc_move(ctx, &classes);
	}
	if (pmodtimes) {
		*pmodtimes = talloc_move(ctx, &modtimes);
	}

	status = NT_STATUS_OK;

 error:
	TALLOC_FREE(mem_ctx);
	return status;
}

static NTSTATUS registry_enumvalues(TALLOC_CTX *ctx,
				    struct rpc_pipe_client *pipe_hnd,
				    struct policy_handle *key_hnd,
				    uint32 *pnum_values, char ***pvalnames,
				    struct registry_value ***pvalues)
{
	TALLOC_CTX *mem_ctx;
	NTSTATUS status;
	uint32 num_subkeys, max_subkeylen, max_classlen;
	uint32 num_values, max_valnamelen, max_valbufsize;
	uint32 i;
	NTTIME last_changed_time;
	uint32 secdescsize;
	struct winreg_String classname;
	struct registry_value **values;
	char **names;

	if (!(mem_ctx = talloc_new(ctx))) {
		return NT_STATUS_NO_MEMORY;
	}

	ZERO_STRUCT(classname);
	status = rpccli_winreg_QueryInfoKey(
		pipe_hnd, mem_ctx, key_hnd, &classname, &num_subkeys,
		&max_subkeylen, &max_classlen, &num_values, &max_valnamelen,
		&max_valbufsize, &secdescsize, &last_changed_time );

	if (!NT_STATUS_IS_OK(status)) {
		goto error;
	}

	if (num_values == 0) {
		*pnum_values = 0;
		TALLOC_FREE(mem_ctx);
		return NT_STATUS_OK;
	}

	if ((!(names = TALLOC_ARRAY(mem_ctx, char *, num_values))) ||
	    (!(values = TALLOC_ARRAY(mem_ctx, struct registry_value *,
				     num_values)))) {
		status = NT_STATUS_NO_MEMORY;
		goto error;
	}

	for (i=0; i<num_values; i++) {
		enum winreg_Type type = REG_NONE;
		uint8 *data = NULL;
		uint32 data_size;
		uint32 value_length;

		char n;
		struct winreg_ValNameBuf name_buf;
		WERROR err;

		n = '\0';
		name_buf.name = &n;
		name_buf.size = max_valnamelen + 2;

		data_size = max_valbufsize;
		data = (uint8 *)TALLOC(mem_ctx, data_size);
		value_length = 0;

		status = rpccli_winreg_EnumValue(pipe_hnd, mem_ctx, key_hnd,
						 i, &name_buf, &type,
						 data, &data_size,
						 &value_length );

		if ( W_ERROR_EQUAL(ntstatus_to_werror(status),
				   WERR_NO_MORE_ITEMS) ) {
			status = NT_STATUS_OK;
			break;
		}

		if (!(NT_STATUS_IS_OK(status))) {
			goto error;
		}

		if (name_buf.name == NULL) {
			status = NT_STATUS_INVALID_PARAMETER;
			goto error;
		}

		if (!(names[i] = talloc_strdup(names, name_buf.name))) {
			status = NT_STATUS_NO_MEMORY;
			goto error;
		}

		err = registry_pull_value(values, &values[i], type, data,
					  data_size, value_length);
		if (!W_ERROR_IS_OK(err)) {
			status = werror_to_ntstatus(err);
			goto error;
		}
	}

	*pnum_values = num_values;

	if (pvalnames) {
		*pvalnames = talloc_move(ctx, &names);
	}
	if (pvalues) {
		*pvalues = talloc_move(ctx, &values);
	}

	status = NT_STATUS_OK;

 error:
	TALLOC_FREE(mem_ctx);
	return status;
}

static NTSTATUS registry_setvalue(TALLOC_CTX *mem_ctx,
				  struct rpc_pipe_client *pipe_hnd,
				  struct policy_handle *key_hnd,
				  const char *name,
				  const struct registry_value *value)
{
	struct winreg_String name_string;
	DATA_BLOB blob;
	NTSTATUS result;
	WERROR err;

	err = registry_push_value(mem_ctx, value, &blob);
	if (!W_ERROR_IS_OK(err)) {
		return werror_to_ntstatus(err);
	}

	name_string.name = name;
	result = rpccli_winreg_SetValue(pipe_hnd, blob.data, key_hnd,
					name_string, value->type,
					blob.data, blob.length);
	TALLOC_FREE(blob.data);
	return result;
}

static NTSTATUS rpc_registry_setvalue_internal(const DOM_SID *domain_sid,
					       const char *domain_name, 
					       struct cli_state *cli,
					       struct rpc_pipe_client *pipe_hnd,
					       TALLOC_CTX *mem_ctx, 
					       int argc,
					       const char **argv )
{
	struct policy_handle hive_hnd, key_hnd;
	NTSTATUS status;
	struct registry_value value;

	status = registry_openkey(mem_ctx, pipe_hnd, argv[0], REG_KEY_WRITE,
				  &hive_hnd, &key_hnd);
	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "registry_openkey failed: %s\n",
			  nt_errstr(status));
		return status;
	}

	if (!strequal(argv[2], "multi_sz") && (argc != 4)) {
		d_fprintf(stderr, "Too many args for type %s\n", argv[2]);
		return NT_STATUS_NOT_IMPLEMENTED;
	}

	if (strequal(argv[2], "dword")) {
		value.type = REG_DWORD;
		value.v.dword = strtoul(argv[3], NULL, 10);
	}
	else if (strequal(argv[2], "sz")) {
		value.type = REG_SZ;
		value.v.sz.len = strlen(argv[3])+1;
		value.v.sz.str = CONST_DISCARD(char *, argv[3]);
	}
	else {
		d_fprintf(stderr, "type \"%s\" not implemented\n", argv[2]);
		status = NT_STATUS_NOT_IMPLEMENTED;
		goto error;
	}

	status = registry_setvalue(mem_ctx, pipe_hnd, &key_hnd,
				   argv[1], &value);

	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "registry_setvalue failed: %s\n",
			  nt_errstr(status));
	}

 error:
	rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &key_hnd);
	rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &hive_hnd);

	return NT_STATUS_OK;
}

static int rpc_registry_setvalue( int argc, const char **argv )
{
	if (argc < 4) {
		d_fprintf(stderr, "usage: net rpc registry setvalue <key> "
			  "<valuename> <type> [<val>]+\n");
		return -1;
	}

	return run_rpc_command( NULL, PI_WINREG, 0, 
		rpc_registry_setvalue_internal, argc, argv );
}

static NTSTATUS rpc_registry_deletevalue_internal(const DOM_SID *domain_sid,
						  const char *domain_name, 
						  struct cli_state *cli,
						  struct rpc_pipe_client *pipe_hnd,
						  TALLOC_CTX *mem_ctx, 
						  int argc,
						  const char **argv )
{
	struct policy_handle hive_hnd, key_hnd;
	NTSTATUS status;
	struct winreg_String valuename;

	status = registry_openkey(mem_ctx, pipe_hnd, argv[0], REG_KEY_WRITE,
				  &hive_hnd, &key_hnd);
	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "registry_openkey failed: %s\n",
			  nt_errstr(status));
		return status;
	}

	valuename.name = argv[1];

	status = rpccli_winreg_DeleteValue(pipe_hnd, mem_ctx, &key_hnd,
					   valuename);

	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "registry_deletevalue failed: %s\n",
			  nt_errstr(status));
	}

	rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &key_hnd);
	rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &hive_hnd);

	return NT_STATUS_OK;
}

static int rpc_registry_deletevalue( int argc, const char **argv )
{
	if (argc != 2) {
		d_fprintf(stderr, "usage: net rpc registry deletevalue <key> "
			  "<valuename>\n");
		return -1;
	}

	return run_rpc_command( NULL, PI_WINREG, 0, 
		rpc_registry_deletevalue_internal, argc, argv );
}

static NTSTATUS rpc_registry_createkey_internal(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv )
{
	uint32 hive;
	struct policy_handle hive_hnd, key_hnd;
	struct winreg_String key, keyclass;
	enum winreg_CreateAction action;
	NTSTATUS status;

	if (!reg_hive_key(argv[0], &hive, &key.name)) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	status = rpccli_winreg_Connect(pipe_hnd, mem_ctx, hive,
				       REG_KEY_READ|REG_KEY_WRITE,
				       &hive_hnd);
	if (!(NT_STATUS_IS_OK(status))) {
		return status;
	}

	action = REG_ACTION_NONE;
	keyclass.name = "";

	status = rpccli_winreg_CreateKey(pipe_hnd, mem_ctx, &hive_hnd, key,
					 keyclass, 0, REG_KEY_READ, NULL,
					 &key_hnd, &action);
	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "createkey returned %s\n",
			  nt_errstr(status));
		rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &hive_hnd);
		return status;
	}

	switch (action) {
		case REG_ACTION_NONE:
			d_printf("createkey did nothing -- huh?\n");
			break;
		case REG_CREATED_NEW_KEY:
			d_printf("createkey created %s\n", argv[0]);
			break;
		case REG_OPENED_EXISTING_KEY:
			d_printf("createkey opened existing %s\n", argv[0]);
			break;
	}

	rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &key_hnd);
	rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &hive_hnd);

	return status;
}

static int rpc_registry_createkey( int argc, const char **argv )
{
	if (argc != 1) {
		d_fprintf(stderr, "usage: net rpc registry createkey <key>\n");
		return -1;
	}

	return run_rpc_command( NULL, PI_WINREG, 0, 
		rpc_registry_createkey_internal, argc, argv );
}

static NTSTATUS rpc_registry_deletekey_internal(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv )
{
	uint32 hive;
	struct policy_handle hive_hnd;
	struct winreg_String key;
	NTSTATUS status;

	if (!reg_hive_key(argv[0], &hive, &key.name)) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	status = rpccli_winreg_Connect(pipe_hnd, mem_ctx, hive, REG_KEY_WRITE,
				       &hive_hnd);
	if (!(NT_STATUS_IS_OK(status))) {
		return status;
	}

	status = rpccli_winreg_DeleteKey(pipe_hnd, mem_ctx, &hive_hnd, key);
	rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &hive_hnd);

	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "deletekey returned %s\n",
			  nt_errstr(status));
	}

	return status;
}

static int rpc_registry_deletekey( int argc, const char **argv )
{
	if (argc != 1) {
		d_fprintf(stderr, "usage: net rpc registry deletekey <key>\n");
		return -1;
	}

	return run_rpc_command( NULL, PI_WINREG, 0, 
		rpc_registry_deletekey_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static NTSTATUS rpc_registry_enumerate_internal(const DOM_SID *domain_sid,
						const char *domain_name, 
						struct cli_state *cli,
						struct rpc_pipe_client *pipe_hnd,
						TALLOC_CTX *mem_ctx, 
						int argc,
						const char **argv )
{
	POLICY_HND pol_hive, pol_key; 
	NTSTATUS status;
	uint32 num_subkeys = 0;
	uint32 num_values = 0;
	char **names = NULL, **classes = NULL;
	NTTIME **modtimes = NULL;
	uint32 i;
	struct registry_value **values = NULL;
	
	if (argc != 1 ) {
		d_printf("Usage:    net rpc enumerate <path> [recurse]\n");
		d_printf("Example:  net rpc enumerate 'HKLM\\Software\\Samba'\n");
		return NT_STATUS_OK;
	}

	status = registry_openkey(mem_ctx, pipe_hnd, argv[0], REG_KEY_READ,
				  &pol_hive, &pol_key);
	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "registry_openkey failed: %s\n",
			  nt_errstr(status));
		return status;
	}

	status = registry_enumkeys(mem_ctx, pipe_hnd, &pol_key, &num_subkeys,
				   &names, &classes, &modtimes);
	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "enumerating keys failed: %s\n",
			  nt_errstr(status));
		return status;
	}

	for (i=0; i<num_subkeys; i++) {
		d_printf("Keyname   = %s\n", names[i]);
		d_printf("Modtime   = %s\n", modtimes[i]
			 ? http_timestring(nt_time_to_unix(*modtimes[i]))
			 : "None");
		d_printf("\n" );
	}

	status = registry_enumvalues(mem_ctx, pipe_hnd, &pol_key, &num_values,
				     &names, &values);
	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "enumerating values failed: %s\n",
			  nt_errstr(status));
		return status;
	}

	for (i=0; i<num_values; i++) {
		struct registry_value *v = values[i];
		d_printf("Valuename  = %s\n", names[i]);
		d_printf("Type       = %s\n",
			 reg_type_lookup(v->type));
		switch(v->type) {
		case REG_DWORD:
			d_printf("Value      = %d\n", v->v.dword);
			break;
		case REG_SZ:
		case REG_EXPAND_SZ:
			d_printf("Value      = \"%s\"\n", v->v.sz.str);
			break;
		case REG_MULTI_SZ: {
			uint32 j;
			for (j = 0; j < v->v.multi_sz.num_strings; j++) {
				d_printf("Value[%3.3d] = \"%s\"\n", j,
					 v->v.multi_sz.strings[j]);
			}
			break;
		}
		case REG_BINARY:
			d_printf("Value      = %d bytes\n",
				 (int)v->v.binary.length);
			break;
		default:
			d_printf("Value      = <unprintable>\n");
			break;
		}
			
		d_printf("\n");
	}

	rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &pol_key );
	rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &pol_hive );

	return status;
}

/********************************************************************
********************************************************************/

static int rpc_registry_enumerate( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_WINREG, 0, 
		rpc_registry_enumerate_internal, argc, argv );
}

/********************************************************************
********************************************************************/

static NTSTATUS rpc_registry_save_internal(const DOM_SID *domain_sid,
					const char *domain_name, 
					struct cli_state *cli,
					struct rpc_pipe_client *pipe_hnd,
					TALLOC_CTX *mem_ctx, 
					int argc,
					const char **argv )
{
	WERROR result = WERR_GENERAL_FAILURE;
	POLICY_HND pol_hive, pol_key; 
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	struct winreg_String filename;
	
	if (argc != 2 ) {
		d_printf("Usage:    net rpc backup <path> <file> \n");
		return NT_STATUS_OK;
	}
	
	status = registry_openkey(mem_ctx, pipe_hnd, argv[0], REG_KEY_ALL,
				  &pol_hive, &pol_key);
	if (!NT_STATUS_IS_OK(status)) {
		d_fprintf(stderr, "registry_openkey failed: %s\n",
			  nt_errstr(status));
		return status;
	}

	filename.name = argv[1];
	status = rpccli_winreg_SaveKey( pipe_hnd, mem_ctx, &pol_key, &filename, NULL  );
	if ( !W_ERROR_IS_OK(result) ) {
		d_fprintf(stderr, "Unable to save [%s] to %s:%s\n", argv[0], cli->desthost, argv[1]);
	}
	
	/* cleanup */
	
	rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &pol_key );
	rpccli_winreg_CloseKey(pipe_hnd, mem_ctx, &pol_hive );

	return status;
}

/********************************************************************
********************************************************************/

static int rpc_registry_save( int argc, const char **argv )
{
	return run_rpc_command( NULL, PI_WINREG, 0, 
		rpc_registry_save_internal, argc, argv );
}


/********************************************************************
********************************************************************/

static void dump_values( REGF_NK_REC *nk )
{
	int i, j;
	pstring data_str;
	uint32 data_size, data;

	if ( !nk->values )
		return;

	for ( i=0; i<nk->num_values; i++ ) {
		d_printf( "\"%s\" = ", nk->values[i].valuename ? nk->values[i].valuename : "(default)" );
		d_printf( "(%s) ", reg_type_lookup( nk->values[i].type ) );

		data_size = nk->values[i].data_size & ~VK_DATA_IN_OFFSET;
		switch ( nk->values[i].type ) {
			case REG_SZ:
				rpcstr_pull( data_str, nk->values[i].data, sizeof(data_str), -1, STR_TERMINATE );
				d_printf( "%s", data_str );
				break;
			case REG_MULTI_SZ:
			case REG_EXPAND_SZ:
				for ( j=0; j<data_size; j++ ) {
					d_printf( "%c", nk->values[i].data[j] );
				}
				break;
			case REG_DWORD:
				data = IVAL( nk->values[i].data, 0 );
				d_printf("0x%x", data );
				break;
			case REG_BINARY:
				for ( j=0; j<data_size; j++ ) {
					d_printf( "%x", nk->values[i].data[j] );
				}
				break;
			default:
				d_printf("unknown");
				break;
		}

		d_printf( "\n" );
	}

}

/********************************************************************
********************************************************************/

static BOOL dump_registry_tree( REGF_FILE *file, REGF_NK_REC *nk, const char *parent )
{
	REGF_NK_REC *key;
	pstring regpath;

	/* depth first dump of the registry tree */

	while ( (key = regfio_fetch_subkey( file, nk )) ) {
		pstr_sprintf( regpath, "%s\\%s", parent, key->keyname );
		d_printf("[%s]\n", regpath );
		dump_values( key );
		d_printf("\n");
		dump_registry_tree( file, key, regpath );
	}

	return True;
}

/********************************************************************
********************************************************************/

static BOOL write_registry_tree( REGF_FILE *infile, REGF_NK_REC *nk, 
                                 REGF_NK_REC *parent, REGF_FILE *outfile,
			         const char *parentpath )
{
	REGF_NK_REC *key, *subkey;
	REGVAL_CTR *values;
	REGSUBKEY_CTR *subkeys;
	int i;
	pstring path;

	if ( !( subkeys = TALLOC_ZERO_P( infile->mem_ctx, REGSUBKEY_CTR )) ) {
		DEBUG(0,("write_registry_tree: talloc() failed!\n"));
		return False;
	}

	if ( !(values = TALLOC_ZERO_P( subkeys, REGVAL_CTR )) ) {
		DEBUG(0,("write_registry_tree: talloc() failed!\n"));
		return False;
	}

	/* copy values into the REGVAL_CTR */
	
	for ( i=0; i<nk->num_values; i++ ) {
		regval_ctr_addvalue( values, nk->values[i].valuename, nk->values[i].type,
			(const char *)nk->values[i].data, (nk->values[i].data_size & ~VK_DATA_IN_OFFSET) );
	}

	/* copy subkeys into the REGSUBKEY_CTR */
	
	while ( (subkey = regfio_fetch_subkey( infile, nk )) ) {
		regsubkey_ctr_addkey( subkeys, subkey->keyname );
	}
	
	key = regfio_write_key( outfile, nk->keyname, values, subkeys, nk->sec_desc->sec_desc, parent );

	/* write each one of the subkeys out */

	pstr_sprintf( path, "%s%s%s", parentpath, parent ? "\\" : "", nk->keyname );
	nk->subkey_index = 0;
	while ( (subkey = regfio_fetch_subkey( infile, nk )) ) {
		write_registry_tree( infile, subkey, key, outfile, path );
	}

	TALLOC_FREE( subkeys );

	d_printf("[%s]\n", path );
	
	return True;
}

/********************************************************************
********************************************************************/

static int rpc_registry_dump( int argc, const char **argv )
{
	REGF_FILE   *registry;
	REGF_NK_REC *nk;
	
	if (argc != 1 ) {
		d_printf("Usage:    net rpc dump <file> \n");
		return 0;
	}
	
	d_printf("Opening %s....", argv[0]);
	if ( !(registry = regfio_open( argv[0], O_RDONLY, 0)) ) {
		d_fprintf(stderr, "Failed to open %s for reading\n", argv[0]);
		return 1;
	}
	d_printf("ok\n");
	
	/* get the root of the registry file */
	
	if ((nk = regfio_rootkey( registry )) == NULL) {
		d_fprintf(stderr, "Could not get rootkey\n");
		regfio_close( registry );
		return 1;
	}
	d_printf("[%s]\n", nk->keyname);
	dump_values( nk );
	d_printf("\n");

	dump_registry_tree( registry, nk, nk->keyname );

#if 0
	talloc_report_full( registry->mem_ctx, stderr );
#endif	
	d_printf("Closing registry...");
	regfio_close( registry );
	d_printf("ok\n");

	return 0;
}

/********************************************************************
********************************************************************/

static int rpc_registry_copy( int argc, const char **argv )
{
	REGF_FILE   *infile = NULL, *outfile = NULL;
	REGF_NK_REC *nk;
	int result = 1;
	
	if (argc != 2 ) {
		d_printf("Usage:    net rpc copy <srcfile> <newfile>\n");
		return 0;
	}
	
	d_printf("Opening %s....", argv[0]);
	if ( !(infile = regfio_open( argv[0], O_RDONLY, 0 )) ) {
		d_fprintf(stderr, "Failed to open %s for reading\n", argv[0]);
		return 1;
	}
	d_printf("ok\n");

	d_printf("Opening %s....", argv[1]);
	if ( !(outfile = regfio_open( argv[1], (O_RDWR|O_CREAT|O_TRUNC), (S_IREAD|S_IWRITE) )) ) {
		d_fprintf(stderr, "Failed to open %s for writing\n", argv[1]);
		goto out;
	}
	d_printf("ok\n");
	
	/* get the root of the registry file */
	
	if ((nk = regfio_rootkey( infile )) == NULL) {
		d_fprintf(stderr, "Could not get rootkey\n");
		goto out;
	}
	d_printf("RootKey: [%s]\n", nk->keyname);

	write_registry_tree( infile, nk, NULL, outfile, "" );

	result = 0;

out:

	d_printf("Closing %s...", argv[1]);
	if (outfile) {
		regfio_close( outfile );
	}
	d_printf("ok\n");

	d_printf("Closing %s...", argv[0]);
	if (infile) {
		regfio_close( infile );
	}
	d_printf("ok\n");

	return( result);
}

/********************************************************************
********************************************************************/

int net_rpc_registry(int argc, const char **argv) 
{
	struct functable2 func[] = {
		{ "enumerate", rpc_registry_enumerate,
		  "Enumerate registry keys and values" },
		{ "createkey",  rpc_registry_createkey,
		  "Create a new registry key" },
		{ "deletekey",  rpc_registry_deletekey,
		  "Delete a registry key" },
		{ "setvalue",  rpc_registry_setvalue,
		  "Set a new registry value" },
		{ "deletevalue",  rpc_registry_deletevalue,
		  "Delete a registry value" },
		{ "save", rpc_registry_save,
		  "Save a registry file" },
		{ "dump", rpc_registry_dump,
		  "Dump a registry file" },
		{ "copy", rpc_registry_copy,
		  "Copy a registry file" },
		{NULL, NULL, NULL}
	};
	
	return net_run_function2(argc, argv, "net rpc registry", func);
}
