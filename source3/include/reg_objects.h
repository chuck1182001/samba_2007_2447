/* 
   Samba's Internal Registry objects
   
   SMB parameters and setup
   Copyright (C) Gerald Carter                   2002-2006.
   
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
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _REG_OBJECTS_H /* _REG_OBJECTS_H */
#define _REG_OBJECTS_H

/* structure to contain registry values */

typedef struct {
	fstring		valuename;
	uint16		type;
	/* this should be encapsulated in an RPC_DATA_BLOB */
	uint32		size;	/* in bytes */
	uint8           *data_p;
} REGISTRY_VALUE;

/*
 * A REG_SZ string is not necessarily NULL terminated. When retrieving it from
 * the net, we guarantee this however. A server might want to push it without
 * the terminator though.
 */

struct registry_string {
	size_t len;
	char *str;
};

struct registry_value {
	enum winreg_Type type;
	union {
		uint32 dword;
		uint64 qword;
		struct registry_string sz;
		struct {
			uint32 num_strings;
			char **strings;
		} multi_sz;
		DATA_BLOB binary;
	} v;
};

/* container for registry values */

typedef struct {
	uint32          num_values;
	REGISTRY_VALUE	**values;
} REGVAL_CTR;

/* container for registry subkey names */

typedef struct {
	uint32          num_subkeys;
	char            **subkeys;
} REGSUBKEY_CTR;

/*
 *
 * Macros that used to reside in rpc_reg.h
 *
 */
 
#define HKEY_CLASSES_ROOT	0x80000000
#define HKEY_CURRENT_USER	0x80000001
#define HKEY_LOCAL_MACHINE 	0x80000002
#define HKEY_USERS         	0x80000003
#define HKEY_PERFORMANCE_DATA	0x80000004

#define KEY_HKLM		"HKLM"
#define KEY_HKU			"HKU"
#define KEY_HKCC		"HKCC"
#define KEY_HKCR		"HKCR"
#define KEY_HKPD		"HKPD"
#define KEY_HKPT		"HKPT"
#define KEY_HKPN		"HKPN"
#define KEY_HKCU		"HKCU"
#define KEY_HKDD		"HKDD"
#define KEY_SERVICES		"HKLM\\SYSTEM\\CurrentControlSet\\Services"
#define KEY_PRINTING 		"HKLM\\SYSTEM\\CurrentControlSet\\Control\\Print"
#define KEY_PRINTING_2K		"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Print\\Printers"
#define KEY_PRINTING_PORTS	"HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Ports"
#define KEY_EVENTLOG 		"HKLM\\SYSTEM\\CurrentControlSet\\Services\\Eventlog"
#define KEY_SHARES		"HKLM\\SYSTEM\\CurrentControlSet\\Services\\LanmanServer\\Shares"
#define KEY_SMBCONF		"HKLM\\SOFTWARE\\Samba\\smbconf"
#define KEY_TREE_ROOT		""

/*
 * Registry key types
 *	Most keys are going to be GENERIC -- may need a better name?
 *	HKPD and HKPT are used by reg_perfcount.c
 *		they are special keys that contain performance data
 */
#define REG_KEY_GENERIC		0
#define REG_KEY_HKPD		1
#define REG_KEY_HKPT		2

/* 
 * container for function pointers to enumeration routines
 * for virtual registry view 
 */ 
 
typedef struct {
	/* functions for enumerating subkeys and values */	
	int 	(*fetch_subkeys)( const char *key, REGSUBKEY_CTR *subkeys);
	int 	(*fetch_values) ( const char *key, REGVAL_CTR *val );
	BOOL 	(*store_subkeys)( const char *key, REGSUBKEY_CTR *subkeys );
	BOOL 	(*store_values)( const char *key, REGVAL_CTR *val );
	BOOL	(*reg_access_check)( const char *keyname, uint32 requested,
				     uint32 *granted,
				     const NT_USER_TOKEN *token );
	WERROR (*get_secdesc)(TALLOC_CTX *mem_ctx, const char *key,
			      struct security_descriptor **psecdesc);
	WERROR (*set_secdesc)(const char *key,
			      struct security_descriptor *sec_desc);
} REGISTRY_OPS;

typedef struct {
	const char	*keyname;	/* full path to name of key */
	REGISTRY_OPS	*ops;		/* registry function hooks */
} REGISTRY_HOOK;


/* structure to store the registry handles */

typedef struct _RegistryKey {
	uint32		type;
	char		*name; 		/* full name of registry key */
	uint32 		access_granted;
	REGISTRY_HOOK	*hook;	
} REGISTRY_KEY;

struct registry_key {
	REGISTRY_KEY *key;
	REGSUBKEY_CTR *subkeys;
	REGVAL_CTR *values;
	struct nt_user_token *token;
};

#endif /* _REG_OBJECTS_H */
