/* 
   Unix SMB/CIFS implementation.

   Winbind cache backend functions

   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Gerald Carter   2003-2007
   Copyright (C) Volker Lendecke 2005
   Copyright (C) Guenther Deschner 2005
   Copyright (C) Michael Adam    2007
   
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

#include "includes.h"
#include "winbindd.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

#define WINBINDD_CACHE_VERSION 1
#define WINBINDD_CACHE_VERSION_KEYSTR "WINBINDD_CACHE_VERSION"

extern struct winbindd_methods reconnect_methods;
extern BOOL opt_nocache;
#ifdef HAVE_ADS
extern struct winbindd_methods ads_methods;
#endif

/*
 * JRA. KEEP THIS LIST UP TO DATE IF YOU ADD CACHE ENTRIES.
 * Here are the list of entry types that are *not* stored
 * as form struct cache_entry in the cache.
 */

static const char *non_centry_keys[] = {
	"SEQNUM/",
	"DR/",
	"DE/",
	"WINBINDD_OFFLINE",
	WINBINDD_CACHE_VERSION_KEYSTR,
	NULL
};

/************************************************************************
 Is this key a non-centry type ?
************************************************************************/

static BOOL is_non_centry_key(TDB_DATA kbuf)
{
	int i;

	if (kbuf.dptr == NULL || kbuf.dsize == 0) {
		return False;
	}
	for (i = 0; non_centry_keys[i] != NULL; i++) {
		size_t namelen = strlen(non_centry_keys[i]);
		if (kbuf.dsize < namelen) {
			continue;
		}
		if (strncmp(non_centry_keys[i], (const char *)kbuf.dptr, namelen) == 0) {
			return True;
		}
	}
	return False;
}

/* Global online/offline state - False when online. winbindd starts up online
   and sets this to true if the first query fails and there's an entry in
   the cache tdb telling us to stay offline. */

static BOOL global_winbindd_offline_state;

struct winbind_cache {
	TDB_CONTEXT *tdb;
};

struct cache_entry {
	NTSTATUS status;
	uint32 sequence_number;
	uint8 *data;
	uint32 len, ofs;
};

void (*smb_panic_fn)(const char *const why) = smb_panic;

#define WINBINDD_MAX_CACHE_SIZE (50*1024*1024)

static struct winbind_cache *wcache;

void winbindd_check_cache_size(time_t t)
{
	static time_t last_check_time;
	struct stat st;

	if (last_check_time == (time_t)0)
		last_check_time = t;

	if (t - last_check_time < 60 && t - last_check_time > 0)
		return;

	if (wcache == NULL || wcache->tdb == NULL) {
		DEBUG(0, ("Unable to check size of tdb cache - cache not open !\n"));
		return;
	}

	if (fstat(tdb_fd(wcache->tdb), &st) == -1) {
		DEBUG(0, ("Unable to check size of tdb cache %s!\n", strerror(errno) ));
		return;
	}

	if (st.st_size > WINBINDD_MAX_CACHE_SIZE) {
		DEBUG(10,("flushing cache due to size (%lu) > (%lu)\n",
			(unsigned long)st.st_size,
			(unsigned long)WINBINDD_MAX_CACHE_SIZE));
		wcache_flush_cache();
	}
}

/* get the winbind_cache structure */
static struct winbind_cache *get_cache(struct winbindd_domain *domain)
{
	struct winbind_cache *ret = wcache;
#ifdef HAVE_ADS
	struct winbindd_domain *our_domain = domain;
#endif

	/* We have to know what type of domain we are dealing with first. */

	if ( !domain->initialized ) {
		init_dc_connection( domain );
	}

	/* 
	   OK.  listen up becasue I'm only going to say this once.
	   We have the following scenarios to consider
	   (a) trusted AD domains on a Samba DC,
	   (b) trusted AD domains and we are joined to a non-kerberos domain
	   (c) trusted AD domains and we are joined to a kerberos (AD) domain

	   For (a) we can always contact the trusted domain using krb5 
	   since we have the domain trust account password

	   For (b) we can only use RPC since we have no way of 
	   getting a krb5 ticket in our own domain

	   For (c) we can always use krb5 since we have a kerberos trust

	   --jerry
	 */

	if (!domain->backend) {
#ifdef HAVE_ADS
		/* find our domain first so we can figure out if we 
		   are joined to a kerberized domain */

		if ( !domain->primary )
			our_domain = find_our_domain();

		if ( (our_domain->active_directory || IS_DC) && domain->active_directory ) {
			DEBUG(5,("get_cache: Setting ADS methods for domain %s\n", domain->name));
			domain->backend = &ads_methods;
		} else {
#endif	/* HAVE_ADS */
			DEBUG(5,("get_cache: Setting MS-RPC methods for domain %s\n", domain->name));
			domain->backend = &reconnect_methods;
#ifdef HAVE_ADS
		}
#endif	/* HAVE_ADS */
	}

	if (ret)
		return ret;
	
	ret = SMB_XMALLOC_P(struct winbind_cache);
	ZERO_STRUCTP(ret);

	wcache = ret;
	wcache_flush_cache();

	return ret;
}

/*
  free a centry structure
*/
static void centry_free(struct cache_entry *centry)
{
	if (!centry)
		return;
	SAFE_FREE(centry->data);
	free(centry);
}

static BOOL centry_check_bytes(struct cache_entry *centry, size_t nbytes)
{
	if (centry->len - centry->ofs < nbytes) {
		DEBUG(0,("centry corruption? needed %u bytes, have %d\n", 
			 (unsigned int)nbytes,
			 centry->len - centry->ofs));
		return False;
	}
	return True;
}

/*
  pull a uint32 from a cache entry 
*/
static uint32 centry_uint32(struct cache_entry *centry)
{
	uint32 ret;

	if (!centry_check_bytes(centry, 4)) {
		smb_panic_fn("centry_uint32");
	}
	ret = IVAL(centry->data, centry->ofs);
	centry->ofs += 4;
	return ret;
}

/*
  pull a uint16 from a cache entry 
*/
static uint16 centry_uint16(struct cache_entry *centry)
{
	uint16 ret;
	if (!centry_check_bytes(centry, 2)) {
		smb_panic_fn("centry_uint16");
	}
	ret = CVAL(centry->data, centry->ofs);
	centry->ofs += 2;
	return ret;
}

/*
  pull a uint8 from a cache entry 
*/
static uint8 centry_uint8(struct cache_entry *centry)
{
	uint8 ret;
	if (!centry_check_bytes(centry, 1)) {
		smb_panic_fn("centry_uint8");
	}
	ret = CVAL(centry->data, centry->ofs);
	centry->ofs += 1;
	return ret;
}

/*
  pull a NTTIME from a cache entry 
*/
static NTTIME centry_nttime(struct cache_entry *centry)
{
	NTTIME ret;
	if (!centry_check_bytes(centry, 8)) {
		smb_panic_fn("centry_nttime");
	}
	ret = IVAL(centry->data, centry->ofs);
	centry->ofs += 4;
	ret += (uint64_t)IVAL(centry->data, centry->ofs) << 32;
	centry->ofs += 4;
	return ret;
}

/*
  pull a time_t from a cache entry. time_t stored portably as a 64-bit time.
*/
static time_t centry_time(struct cache_entry *centry)
{
	return (time_t)centry_nttime(centry);
}

/* pull a string from a cache entry, using the supplied
   talloc context 
*/
static char *centry_string(struct cache_entry *centry, TALLOC_CTX *mem_ctx)
{
	uint32 len;
	char *ret;

	len = centry_uint8(centry);

	if (len == 0xFF) {
		/* a deliberate NULL string */
		return NULL;
	}

	if (!centry_check_bytes(centry, (size_t)len)) {
		smb_panic_fn("centry_string");
	}

	ret = TALLOC_ARRAY(mem_ctx, char, len+1);
	if (!ret) {
		smb_panic_fn("centry_string out of memory\n");
	}
	memcpy(ret,centry->data + centry->ofs, len);
	ret[len] = 0;
	centry->ofs += len;
	return ret;
}

/* pull a hash16 from a cache entry, using the supplied
   talloc context 
*/
static char *centry_hash16(struct cache_entry *centry, TALLOC_CTX *mem_ctx)
{
	uint32 len;
	char *ret;

	len = centry_uint8(centry);

	if (len != 16) {
		DEBUG(0,("centry corruption? hash len (%u) != 16\n", 
			len ));
		return NULL;
	}

	if (!centry_check_bytes(centry, 16)) {
		return NULL;
	}

	ret = TALLOC_ARRAY(mem_ctx, char, 16);
	if (!ret) {
		smb_panic_fn("centry_hash out of memory\n");
	}
	memcpy(ret,centry->data + centry->ofs, 16);
	centry->ofs += 16;
	return ret;
}

/* pull a sid from a cache entry, using the supplied
   talloc context 
*/
static BOOL centry_sid(struct cache_entry *centry, TALLOC_CTX *mem_ctx, DOM_SID *sid)
{
	char *sid_string;
	sid_string = centry_string(centry, mem_ctx);
	if ((sid_string == NULL) || (!string_to_sid(sid, sid_string))) {
		return False;
	}
	return True;
}

/* the server is considered down if it can't give us a sequence number */
static BOOL wcache_server_down(struct winbindd_domain *domain)
{
	BOOL ret;

	if (!wcache->tdb)
		return False;

	ret = (domain->sequence_number == DOM_SEQUENCE_NONE);

	if (ret)
		DEBUG(10,("wcache_server_down: server for Domain %s down\n", 
			domain->name ));
	return ret;
}

static NTSTATUS fetch_cache_seqnum( struct winbindd_domain *domain, time_t now )
{
	TDB_DATA data;
	fstring key;
	uint32 time_diff;
	
	if (!wcache->tdb) {
		DEBUG(10,("fetch_cache_seqnum: tdb == NULL\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}
		
	fstr_sprintf( key, "SEQNUM/%s", domain->name );
	
	data = tdb_fetch_bystring( wcache->tdb, key );
	if ( !data.dptr || data.dsize!=8 ) {
		DEBUG(10,("fetch_cache_seqnum: invalid data size key [%s]\n", key ));
		return NT_STATUS_UNSUCCESSFUL;
	}
	
	domain->sequence_number = IVAL(data.dptr, 0);
	domain->last_seq_check  = IVAL(data.dptr, 4);
	
	SAFE_FREE(data.dptr);

	/* have we expired? */
	
	time_diff = now - domain->last_seq_check;
	if ( time_diff > lp_winbind_cache_time() ) {
		DEBUG(10,("fetch_cache_seqnum: timeout [%s][%u @ %u]\n",
			domain->name, domain->sequence_number,
			(uint32)domain->last_seq_check));
		return NT_STATUS_UNSUCCESSFUL;
	}

	DEBUG(10,("fetch_cache_seqnum: success [%s][%u @ %u]\n", 
		domain->name, domain->sequence_number, 
		(uint32)domain->last_seq_check));

	return NT_STATUS_OK;
}

static NTSTATUS store_cache_seqnum( struct winbindd_domain *domain )
{
	TDB_DATA data;
	fstring key_str;
	uint8 buf[8];
	
	if (!wcache->tdb) {
		DEBUG(10,("store_cache_seqnum: tdb == NULL\n"));
		return NT_STATUS_UNSUCCESSFUL;
	}
		
	fstr_sprintf( key_str, "SEQNUM/%s", domain->name );
	
	SIVAL(buf, 0, domain->sequence_number);
	SIVAL(buf, 4, domain->last_seq_check);
	data.dptr = buf;
	data.dsize = 8;
	
	if ( tdb_store_bystring( wcache->tdb, key_str, data, TDB_REPLACE) == -1 ) {
		DEBUG(10,("store_cache_seqnum: tdb_store fail key [%s]\n", key_str ));
		return NT_STATUS_UNSUCCESSFUL;
	}

	DEBUG(10,("store_cache_seqnum: success [%s][%u @ %u]\n", 
		domain->name, domain->sequence_number, 
		(uint32)domain->last_seq_check));
	
	return NT_STATUS_OK;
}

/*
  refresh the domain sequence number. If force is True
  then always refresh it, no matter how recently we fetched it
*/

static void refresh_sequence_number(struct winbindd_domain *domain, BOOL force)
{
	NTSTATUS status;
	unsigned time_diff;
	time_t t = time(NULL);
	unsigned cache_time = lp_winbind_cache_time();

	if ( IS_DOMAIN_OFFLINE(domain) ) {
		return;
	}
	
	get_cache( domain );

#if 0	/* JERRY -- disable as the default cache time is now 5 minutes */
	/* trying to reconnect is expensive, don't do it too often */
	if (domain->sequence_number == DOM_SEQUENCE_NONE) {
		cache_time *= 8;
	}
#endif

	time_diff = t - domain->last_seq_check;

	/* see if we have to refetch the domain sequence number */
	if (!force && (time_diff < cache_time)) {
		DEBUG(10, ("refresh_sequence_number: %s time ok\n", domain->name));
		goto done;
	}
	
	/* try to get the sequence number from the tdb cache first */
	/* this will update the timestamp as well */
	
	status = fetch_cache_seqnum( domain, t );
	if ( NT_STATUS_IS_OK(status) )
		goto done;	

	/* important! make sure that we know if this is a native 
	   mode domain or not.  And that we can contact it. */

	if ( winbindd_can_contact_domain( domain ) ) {		
		status = domain->backend->sequence_number(domain, 
							  &domain->sequence_number);
	} else {
		/* just use the current time */
		status = NT_STATUS_OK;
		domain->sequence_number = time(NULL);
	}


	/* the above call could have set our domain->backend to NULL when
	 * coming from offline to online mode, make sure to reinitialize the
	 * backend - Guenther */
	get_cache( domain );

	if (!NT_STATUS_IS_OK(status)) {
		DEBUG(10,("refresh_sequence_number: failed with %s\n", nt_errstr(status)));
		domain->sequence_number = DOM_SEQUENCE_NONE;
	}
	
	domain->last_status = status;
	domain->last_seq_check = time(NULL);
	
	/* save the new sequence number ni the cache */
	store_cache_seqnum( domain );

done:
	DEBUG(10, ("refresh_sequence_number: %s seq number is now %d\n", 
		   domain->name, domain->sequence_number));

	return;
}

/*
  decide if a cache entry has expired
*/
static BOOL centry_expired(struct winbindd_domain *domain, const char *keystr, struct cache_entry *centry)
{
	/* If we've been told to be offline - stay in that state... */
	if (lp_winbind_offline_logon() && global_winbindd_offline_state) {
		DEBUG(10,("centry_expired: Key %s for domain %s valid as winbindd is globally offline.\n",
			keystr, domain->name ));
		return False;
	}

	/* when the domain is offline return the cached entry.
	 * This deals with transient offline states... */

	if (!domain->online) {
		DEBUG(10,("centry_expired: Key %s for domain %s valid as domain is offline.\n",
			keystr, domain->name ));
		return False;
	}

	/* if the server is OK and our cache entry came from when it was down then
	   the entry is invalid */
	if ((domain->sequence_number != DOM_SEQUENCE_NONE) &&  
	    (centry->sequence_number == DOM_SEQUENCE_NONE)) {
		DEBUG(10,("centry_expired: Key %s for domain %s invalid sequence.\n",
			keystr, domain->name ));
		return True;
	}

	/* if the server is down or the cache entry is not older than the
	   current sequence number then it is OK */
	if (wcache_server_down(domain) || 
	    centry->sequence_number == domain->sequence_number) {
		DEBUG(10,("centry_expired: Key %s for domain %s is good.\n",
			keystr, domain->name ));
		return False;
	}

	DEBUG(10,("centry_expired: Key %s for domain %s expired\n",
		keystr, domain->name ));

	/* it's expired */
	return True;
}

static struct cache_entry *wcache_fetch_raw(char *kstr)
{
	TDB_DATA data;
	struct cache_entry *centry;
	TDB_DATA key;

	key = string_tdb_data(kstr);
	data = tdb_fetch(wcache->tdb, key);
	if (!data.dptr) {
		/* a cache miss */
		return NULL;
	}

	centry = SMB_XMALLOC_P(struct cache_entry);
	centry->data = (unsigned char *)data.dptr;
	centry->len = data.dsize;
	centry->ofs = 0;

	if (centry->len < 8) {
		/* huh? corrupt cache? */
		DEBUG(10,("wcache_fetch_raw: Corrupt cache for key %s (len < 8) ?\n", kstr));
		centry_free(centry);
		return NULL;
	}
	
	centry->status = NT_STATUS(centry_uint32(centry));
	centry->sequence_number = centry_uint32(centry);

	return centry;
}

/*
  fetch an entry from the cache, with a varargs key. auto-fetch the sequence
  number and return status
*/
static struct cache_entry *wcache_fetch(struct winbind_cache *cache, 
					struct winbindd_domain *domain,
					const char *format, ...) PRINTF_ATTRIBUTE(3,4);
static struct cache_entry *wcache_fetch(struct winbind_cache *cache, 
					struct winbindd_domain *domain,
					const char *format, ...)
{
	va_list ap;
	char *kstr;
	struct cache_entry *centry;

	if (opt_nocache) {
		return NULL;
	}

	refresh_sequence_number(domain, False);

	va_start(ap, format);
	smb_xvasprintf(&kstr, format, ap);
	va_end(ap);

	centry = wcache_fetch_raw(kstr);
	if (centry == NULL) {
		free(kstr);
		return NULL;
	}

	if (centry_expired(domain, kstr, centry)) {

		DEBUG(10,("wcache_fetch: entry %s expired for domain %s\n",
			 kstr, domain->name ));

		centry_free(centry);
		free(kstr);
		return NULL;
	}

	DEBUG(10,("wcache_fetch: returning entry %s for domain %s\n",
		 kstr, domain->name ));

	free(kstr);
	return centry;
}

static void wcache_delete(const char *format, ...) PRINTF_ATTRIBUTE(1,2);
static void wcache_delete(const char *format, ...)
{
	va_list ap;
	char *kstr;
	TDB_DATA key;

	va_start(ap, format);
	smb_xvasprintf(&kstr, format, ap);
	va_end(ap);

	key = string_tdb_data(kstr);

	tdb_delete(wcache->tdb, key);
	free(kstr);
}

/*
  make sure we have at least len bytes available in a centry 
*/
static void centry_expand(struct cache_entry *centry, uint32 len)
{
	if (centry->len - centry->ofs >= len)
		return;
	centry->len *= 2;
	centry->data = SMB_REALLOC_ARRAY(centry->data, unsigned char,
					 centry->len);
	if (!centry->data) {
		DEBUG(0,("out of memory: needed %d bytes in centry_expand\n", centry->len));
		smb_panic_fn("out of memory in centry_expand");
	}
}

/*
  push a uint32 into a centry 
*/
static void centry_put_uint32(struct cache_entry *centry, uint32 v)
{
	centry_expand(centry, 4);
	SIVAL(centry->data, centry->ofs, v);
	centry->ofs += 4;
}

/*
  push a uint16 into a centry 
*/
static void centry_put_uint16(struct cache_entry *centry, uint16 v)
{
	centry_expand(centry, 2);
	SIVAL(centry->data, centry->ofs, v);
	centry->ofs += 2;
}

/*
  push a uint8 into a centry 
*/
static void centry_put_uint8(struct cache_entry *centry, uint8 v)
{
	centry_expand(centry, 1);
	SCVAL(centry->data, centry->ofs, v);
	centry->ofs += 1;
}

/* 
   push a string into a centry 
 */
static void centry_put_string(struct cache_entry *centry, const char *s)
{
	int len;

	if (!s) {
		/* null strings are marked as len 0xFFFF */
		centry_put_uint8(centry, 0xFF);
		return;
	}

	len = strlen(s);
	/* can't handle more than 254 char strings. Truncating is probably best */
	if (len > 254) {
		DEBUG(10,("centry_put_string: truncating len (%d) to: 254\n", len));
		len = 254;
	}
	centry_put_uint8(centry, len);
	centry_expand(centry, len);
	memcpy(centry->data + centry->ofs, s, len);
	centry->ofs += len;
}

/* 
   push a 16 byte hash into a centry - treat as 16 byte string.
 */
static void centry_put_hash16(struct cache_entry *centry, const uint8 val[16])
{
	centry_put_uint8(centry, 16);
	centry_expand(centry, 16);
	memcpy(centry->data + centry->ofs, val, 16);
	centry->ofs += 16;
}

static void centry_put_sid(struct cache_entry *centry, const DOM_SID *sid) 
{
	fstring sid_string;
	centry_put_string(centry, sid_to_string(sid_string, sid));
}

/*
  push a NTTIME into a centry 
*/
static void centry_put_nttime(struct cache_entry *centry, NTTIME nt)
{
	centry_expand(centry, 8);
	SIVAL(centry->data, centry->ofs, nt & 0xFFFFFFFF);
	centry->ofs += 4;
	SIVAL(centry->data, centry->ofs, nt >> 32);
	centry->ofs += 4;
}

/*
  push a time_t into a centry - use a 64 bit size.
  NTTIME here is being used as a convenient 64-bit size.
*/
static void centry_put_time(struct cache_entry *centry, time_t t)
{
	NTTIME nt = (NTTIME)t;
	centry_put_nttime(centry, nt);
}

/*
  start a centry for output. When finished, call centry_end()
*/
struct cache_entry *centry_start(struct winbindd_domain *domain, NTSTATUS status)
{
	struct cache_entry *centry;

	if (!wcache->tdb)
		return NULL;

	centry = SMB_XMALLOC_P(struct cache_entry);

	centry->len = 8192; /* reasonable default */
	centry->data = SMB_XMALLOC_ARRAY(uint8, centry->len);
	centry->ofs = 0;
	centry->sequence_number = domain->sequence_number;
	centry_put_uint32(centry, NT_STATUS_V(status));
	centry_put_uint32(centry, centry->sequence_number);
	return centry;
}

/*
  finish a centry and write it to the tdb
*/
static void centry_end(struct cache_entry *centry, const char *format, ...) PRINTF_ATTRIBUTE(2,3);
static void centry_end(struct cache_entry *centry, const char *format, ...)
{
	va_list ap;
	char *kstr;
	TDB_DATA key, data;

	va_start(ap, format);
	smb_xvasprintf(&kstr, format, ap);
	va_end(ap);

	key = string_tdb_data(kstr);
	data.dptr = centry->data;
	data.dsize = centry->ofs;

	tdb_store(wcache->tdb, key, data, TDB_REPLACE);
	free(kstr);
}

static void wcache_save_name_to_sid(struct winbindd_domain *domain, 
				    NTSTATUS status, const char *domain_name,
				    const char *name, const DOM_SID *sid, 
				    enum lsa_SidType type)
{
	struct cache_entry *centry;
	fstring uname;

	centry = centry_start(domain, status);
	if (!centry)
		return;
	centry_put_uint32(centry, type);
	centry_put_sid(centry, sid);
	fstrcpy(uname, name);
	strupper_m(uname);
	centry_end(centry, "NS/%s/%s", domain_name, uname);
	DEBUG(10,("wcache_save_name_to_sid: %s\\%s -> %s (%s)\n", domain_name, uname,
		  sid_string_static(sid), nt_errstr(status)));
	centry_free(centry);
}

static void wcache_save_sid_to_name(struct winbindd_domain *domain, NTSTATUS status, 
				    const DOM_SID *sid, const char *domain_name, const char *name, enum lsa_SidType type)
{
	struct cache_entry *centry;
	fstring sid_string;

	if (is_null_sid(sid)) {
		return;
	}

	centry = centry_start(domain, status);
	if (!centry)
		return;
	if (NT_STATUS_IS_OK(status)) {
		centry_put_uint32(centry, type);
		centry_put_string(centry, domain_name);
		centry_put_string(centry, name);
	}
	centry_end(centry, "SN/%s", sid_to_string(sid_string, sid));
	DEBUG(10,("wcache_save_sid_to_name: %s -> %s (%s)\n", sid_string, 
		  name, nt_errstr(status)));
	centry_free(centry);
}


static void wcache_save_user(struct winbindd_domain *domain, NTSTATUS status, WINBIND_USERINFO *info)
{
	struct cache_entry *centry;
	fstring sid_string;

	if (is_null_sid(&info->user_sid)) {
		return;
	}

	centry = centry_start(domain, status);
	if (!centry)
		return;
	centry_put_string(centry, info->acct_name);
	centry_put_string(centry, info->full_name);
	centry_put_string(centry, info->homedir);
	centry_put_string(centry, info->shell);
	centry_put_uint32(centry, info->primary_gid);
	centry_put_sid(centry, &info->user_sid);
	centry_put_sid(centry, &info->group_sid);
	centry_end(centry, "U/%s", sid_to_string(sid_string, &info->user_sid));
	DEBUG(10,("wcache_save_user: %s (acct_name %s)\n", sid_string, info->acct_name));
	centry_free(centry);
}

static void wcache_save_lockout_policy(struct winbindd_domain *domain, NTSTATUS status, SAM_UNK_INFO_12 *lockout_policy)
{
	struct cache_entry *centry;

	centry = centry_start(domain, status);
	if (!centry)
		return;

	centry_put_nttime(centry, lockout_policy->duration);
	centry_put_nttime(centry, lockout_policy->reset_count);
	centry_put_uint16(centry, lockout_policy->bad_attempt_lockout);

	centry_end(centry, "LOC_POL/%s", domain->name);
	
	DEBUG(10,("wcache_save_lockout_policy: %s\n", domain->name));

	centry_free(centry);
}

static void wcache_save_password_policy(struct winbindd_domain *domain, NTSTATUS status, SAM_UNK_INFO_1 *policy)
{
	struct cache_entry *centry;

	centry = centry_start(domain, status);
	if (!centry)
		return;

	centry_put_uint16(centry, policy->min_length_password);
	centry_put_uint16(centry, policy->password_history);
	centry_put_uint32(centry, policy->password_properties);
	centry_put_nttime(centry, policy->expire);
	centry_put_nttime(centry, policy->min_passwordage);

	centry_end(centry, "PWD_POL/%s", domain->name);
	
	DEBUG(10,("wcache_save_password_policy: %s\n", domain->name));

	centry_free(centry);
}

NTSTATUS wcache_cached_creds_exist(struct winbindd_domain *domain, const DOM_SID *sid)
{
	struct winbind_cache *cache = get_cache(domain);
	TDB_DATA data;
	fstring key_str;
	uint32 rid;

	if (!cache->tdb) {
		return NT_STATUS_INTERNAL_DB_ERROR;
	}

	if (is_null_sid(sid)) {
		return NT_STATUS_INVALID_SID;
	}

	if (!(sid_peek_rid(sid, &rid)) || (rid == 0)) {
		return NT_STATUS_INVALID_SID;
	}

	fstr_sprintf(key_str, "CRED/%s", sid_string_static(sid));

	data = tdb_fetch(cache->tdb, string_tdb_data(key_str));
	if (!data.dptr) {
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;
	}

	SAFE_FREE(data.dptr);
	return NT_STATUS_OK;
}

/* Lookup creds for a SID - copes with old (unsalted) creds as well
   as new salted ones. */

NTSTATUS wcache_get_creds(struct winbindd_domain *domain, 
			  TALLOC_CTX *mem_ctx, 
			  const DOM_SID *sid,
			  const uint8 **cached_nt_pass,
			  const uint8 **cached_salt)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	time_t t;
	uint32 rid;

	if (!cache->tdb) {
		return NT_STATUS_INTERNAL_DB_ERROR;
	}

	if (is_null_sid(sid)) {
		return NT_STATUS_INVALID_SID;
	}

	if (!(sid_peek_rid(sid, &rid)) || (rid == 0)) {
		return NT_STATUS_INVALID_SID;
	}

	/* Try and get a salted cred first. If we can't
	   fall back to an unsalted cred. */

	centry = wcache_fetch(cache, domain, "CRED/%s", sid_string_static(sid));
	if (!centry) {
		DEBUG(10,("wcache_get_creds: entry for [CRED/%s] not found\n", 
				sid_string_static(sid)));
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;
	}

	t = centry_time(centry);

	/* In the salted case this isn't actually the nt_hash itself,
	   but the MD5 of the salt + nt_hash. Let the caller
	   sort this out. It can tell as we only return the cached_salt
	   if we are returning a salted cred. */

	*cached_nt_pass = (const uint8 *)centry_hash16(centry, mem_ctx);
	if (*cached_nt_pass == NULL) {
		const char *sidstr = sid_string_static(sid);

		/* Bad (old) cred cache. Delete and pretend we
		   don't have it. */
		DEBUG(0,("wcache_get_creds: bad entry for [CRED/%s] - deleting\n", 
				sidstr));
		wcache_delete("CRED/%s", sidstr);
		centry_free(centry);
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;
	}

	/* We only have 17 bytes more data in the salted cred case. */
	if (centry->len - centry->ofs == 17) {
		*cached_salt = (const uint8 *)centry_hash16(centry, mem_ctx);
	} else {
		*cached_salt = NULL;
	}

#if DEBUG_PASSWORD
	dump_data(100, *cached_nt_pass, NT_HASH_LEN);
	if (*cached_salt) {
		dump_data(100, *cached_salt, NT_HASH_LEN);
	}
#endif
	status = centry->status;

	DEBUG(10,("wcache_get_creds: [Cached] - cached creds for user %s status: %s\n",
		sid_string_static(sid), nt_errstr(status) ));

	centry_free(centry);
	return status;
}

/* Store creds for a SID - only writes out new salted ones. */

NTSTATUS wcache_save_creds(struct winbindd_domain *domain, 
			   TALLOC_CTX *mem_ctx, 
			   const DOM_SID *sid, 
			   const uint8 nt_pass[NT_HASH_LEN])
{
	struct cache_entry *centry;
	fstring sid_string;
	uint32 rid;
	uint8 cred_salt[NT_HASH_LEN];
	uint8 salted_hash[NT_HASH_LEN];

	if (is_null_sid(sid)) {
		return NT_STATUS_INVALID_SID;
	}

	if (!(sid_peek_rid(sid, &rid)) || (rid == 0)) {
		return NT_STATUS_INVALID_SID;
	}

	centry = centry_start(domain, NT_STATUS_OK);
	if (!centry) {
		return NT_STATUS_INTERNAL_DB_ERROR;
	}

#if DEBUG_PASSWORD
	dump_data(100, nt_pass, NT_HASH_LEN);
#endif

	centry_put_time(centry, time(NULL));

	/* Create a salt and then salt the hash. */
	generate_random_buffer(cred_salt, NT_HASH_LEN);
	E_md5hash(cred_salt, nt_pass, salted_hash);

	centry_put_hash16(centry, salted_hash);
	centry_put_hash16(centry, cred_salt);
	centry_end(centry, "CRED/%s", sid_to_string(sid_string, sid));

	DEBUG(10,("wcache_save_creds: %s\n", sid_string));

	centry_free(centry);

	return NT_STATUS_OK;
}


/* Query display info. This is the basic user list fn */
static NTSTATUS query_user_list(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				WINBIND_USERINFO **info)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	unsigned int i, retry;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "UL/%s", domain->name);
	if (!centry)
		goto do_query;

	*num_entries = centry_uint32(centry);
	
	if (*num_entries == 0)
		goto do_cached;

	(*info) = TALLOC_ARRAY(mem_ctx, WINBIND_USERINFO, *num_entries);
	if (! (*info)) {
		smb_panic_fn("query_user_list out of memory");
	}
	for (i=0; i<(*num_entries); i++) {
		(*info)[i].acct_name = centry_string(centry, mem_ctx);
		(*info)[i].full_name = centry_string(centry, mem_ctx);
		(*info)[i].homedir = centry_string(centry, mem_ctx);
		(*info)[i].shell = centry_string(centry, mem_ctx);
		centry_sid(centry, mem_ctx, &(*info)[i].user_sid);
		centry_sid(centry, mem_ctx, &(*info)[i].group_sid);
	}

do_cached:	
	status = centry->status;

	DEBUG(10,("query_user_list: [Cached] - cached list for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	*num_entries = 0;
	*info = NULL;

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	/* Put the query_user_list() in a retry loop.  There appears to be
	 * some bug either with Windows 2000 or Samba's handling of large
	 * rpc replies.  This manifests itself as sudden disconnection
	 * at a random point in the enumeration of a large (60k) user list.
	 * The retry loop simply tries the operation again. )-:  It's not
	 * pretty but an acceptable workaround until we work out what the
	 * real problem is. */

	retry = 0;
	do {

		DEBUG(10,("query_user_list: [Cached] - doing backend query for list for domain %s\n",
			domain->name ));

		status = domain->backend->query_user_list(domain, mem_ctx, num_entries, info);
		if (!NT_STATUS_IS_OK(status))
			DEBUG(3, ("query_user_list: returned 0x%08x, "
				  "retrying\n", NT_STATUS_V(status)));
			if (NT_STATUS_EQUAL(status, NT_STATUS_UNSUCCESSFUL)) {
				DEBUG(3, ("query_user_list: flushing "
					  "connection cache\n"));
				invalidate_cm_connection(&domain->conn);
			}

	} while (NT_STATUS_V(status) == NT_STATUS_V(NT_STATUS_UNSUCCESSFUL) && 
		 (retry++ < 5));

	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_entries);
	for (i=0; i<(*num_entries); i++) {
		centry_put_string(centry, (*info)[i].acct_name);
		centry_put_string(centry, (*info)[i].full_name);
		centry_put_string(centry, (*info)[i].homedir);
		centry_put_string(centry, (*info)[i].shell);
		centry_put_sid(centry, &(*info)[i].user_sid);
		centry_put_sid(centry, &(*info)[i].group_sid);
		if (domain->backend && domain->backend->consistent) {
			/* when the backend is consistent we can pre-prime some mappings */
			wcache_save_name_to_sid(domain, NT_STATUS_OK, 
						domain->name,
						(*info)[i].acct_name, 
						&(*info)[i].user_sid,
						SID_NAME_USER);
			wcache_save_sid_to_name(domain, NT_STATUS_OK, 
						&(*info)[i].user_sid,
						domain->name,
						(*info)[i].acct_name, 
						SID_NAME_USER);
			wcache_save_user(domain, NT_STATUS_OK, &(*info)[i]);
		}
	}	
	centry_end(centry, "UL/%s", domain->name);
	centry_free(centry);

skip_save:
	return status;
}

/* list all domain groups */
static NTSTATUS enum_dom_groups(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				struct acct_info **info)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	unsigned int i;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "GL/%s/domain", domain->name);
	if (!centry)
		goto do_query;

	*num_entries = centry_uint32(centry);
	
	if (*num_entries == 0)
		goto do_cached;

	(*info) = TALLOC_ARRAY(mem_ctx, struct acct_info, *num_entries);
	if (! (*info)) {
		smb_panic_fn("enum_dom_groups out of memory");
	}
	for (i=0; i<(*num_entries); i++) {
		fstrcpy((*info)[i].acct_name, centry_string(centry, mem_ctx));
		fstrcpy((*info)[i].acct_desc, centry_string(centry, mem_ctx));
		(*info)[i].rid = centry_uint32(centry);
	}

do_cached:	
	status = centry->status;

	DEBUG(10,("enum_dom_groups: [Cached] - cached list for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	*num_entries = 0;
	*info = NULL;

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	DEBUG(10,("enum_dom_groups: [Cached] - doing backend query for list for domain %s\n",
		domain->name ));

	status = domain->backend->enum_dom_groups(domain, mem_ctx, num_entries, info);

	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_entries);
	for (i=0; i<(*num_entries); i++) {
		centry_put_string(centry, (*info)[i].acct_name);
		centry_put_string(centry, (*info)[i].acct_desc);
		centry_put_uint32(centry, (*info)[i].rid);
	}	
	centry_end(centry, "GL/%s/domain", domain->name);
	centry_free(centry);

skip_save:
	return status;
}

/* list all domain groups */
static NTSTATUS enum_local_groups(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				struct acct_info **info)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	unsigned int i;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "GL/%s/local", domain->name);
	if (!centry)
		goto do_query;

	*num_entries = centry_uint32(centry);
	
	if (*num_entries == 0)
		goto do_cached;

	(*info) = TALLOC_ARRAY(mem_ctx, struct acct_info, *num_entries);
	if (! (*info)) {
		smb_panic_fn("enum_dom_groups out of memory");
	}
	for (i=0; i<(*num_entries); i++) {
		fstrcpy((*info)[i].acct_name, centry_string(centry, mem_ctx));
		fstrcpy((*info)[i].acct_desc, centry_string(centry, mem_ctx));
		(*info)[i].rid = centry_uint32(centry);
	}

do_cached:	

	/* If we are returning cached data and the domain controller
	   is down then we don't know whether the data is up to date
	   or not.  Return NT_STATUS_MORE_PROCESSING_REQUIRED to
	   indicate this. */

	if (wcache_server_down(domain)) {
		DEBUG(10, ("enum_local_groups: returning cached user list and server was down\n"));
		status = NT_STATUS_MORE_PROCESSING_REQUIRED;
	} else
		status = centry->status;

	DEBUG(10,("enum_local_groups: [Cached] - cached list for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	*num_entries = 0;
	*info = NULL;

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	DEBUG(10,("enum_local_groups: [Cached] - doing backend query for list for domain %s\n",
		domain->name ));

	status = domain->backend->enum_local_groups(domain, mem_ctx, num_entries, info);

	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_entries);
	for (i=0; i<(*num_entries); i++) {
		centry_put_string(centry, (*info)[i].acct_name);
		centry_put_string(centry, (*info)[i].acct_desc);
		centry_put_uint32(centry, (*info)[i].rid);
	}
	centry_end(centry, "GL/%s/local", domain->name);
	centry_free(centry);

skip_save:
	return status;
}

/* convert a single name to a sid in a domain */
static NTSTATUS name_to_sid(struct winbindd_domain *domain,
			    TALLOC_CTX *mem_ctx,
			    const char *domain_name,
			    const char *name,
			    DOM_SID *sid,
			    enum lsa_SidType *type)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	fstring uname;

	if (!cache->tdb)
		goto do_query;

	fstrcpy(uname, name);
	strupper_m(uname);
	centry = wcache_fetch(cache, domain, "NS/%s/%s", domain_name, uname);
	if (!centry)
		goto do_query;
	*type = (enum lsa_SidType)centry_uint32(centry);
	status = centry->status;
	if (NT_STATUS_IS_OK(status)) {
		centry_sid(centry, mem_ctx, sid);
	}

	DEBUG(10,("name_to_sid: [Cached] - cached name for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	ZERO_STRUCTP(sid);

	/* If the seq number check indicated that there is a problem
	 * with this DC, then return that status... except for
	 * access_denied.  This is special because the dc may be in
	 * "restrict anonymous = 1" mode, in which case it will deny
	 * most unauthenticated operations, but *will* allow the LSA
	 * name-to-sid that we try as a fallback. */

	if (!(NT_STATUS_IS_OK(domain->last_status)
	      || NT_STATUS_EQUAL(domain->last_status, NT_STATUS_ACCESS_DENIED)))
		return domain->last_status;

	DEBUG(10,("name_to_sid: [Cached] - doing backend query for name for domain %s\n",
		domain->name ));

	status = domain->backend->name_to_sid(domain, mem_ctx, domain_name, name, sid, type);

	/* and save it */
	refresh_sequence_number(domain, False);

	if (domain->online && !is_null_sid(sid)) {
		wcache_save_name_to_sid(domain, status, domain_name, name, sid, *type);
	}

	if (NT_STATUS_IS_OK(status)) {
		strupper_m(CONST_DISCARD(char *,domain_name));
		strlower_m(CONST_DISCARD(char *,name));
		wcache_save_sid_to_name(domain, status, sid, domain_name, name, *type);
	}

	return status;
}

/* convert a sid to a user or group name. The sid is guaranteed to be in the domain
   given */
static NTSTATUS sid_to_name(struct winbindd_domain *domain,
			    TALLOC_CTX *mem_ctx,
			    const DOM_SID *sid,
			    char **domain_name,
			    char **name,
			    enum lsa_SidType *type)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	fstring sid_string;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "SN/%s", sid_to_string(sid_string, sid));
	if (!centry)
		goto do_query;
	if (NT_STATUS_IS_OK(centry->status)) {
		*type = (enum lsa_SidType)centry_uint32(centry);
		*domain_name = centry_string(centry, mem_ctx);
		*name = centry_string(centry, mem_ctx);
	}
	status = centry->status;

	DEBUG(10,("sid_to_name: [Cached] - cached name for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	*name = NULL;
	*domain_name = NULL;

	/* If the seq number check indicated that there is a problem
	 * with this DC, then return that status... except for
	 * access_denied.  This is special because the dc may be in
	 * "restrict anonymous = 1" mode, in which case it will deny
	 * most unauthenticated operations, but *will* allow the LSA
	 * sid-to-name that we try as a fallback. */

	if (!(NT_STATUS_IS_OK(domain->last_status)
	      || NT_STATUS_EQUAL(domain->last_status, NT_STATUS_ACCESS_DENIED)))
		return domain->last_status;

	DEBUG(10,("sid_to_name: [Cached] - doing backend query for name for domain %s\n",
		domain->name ));

	status = domain->backend->sid_to_name(domain, mem_ctx, sid, domain_name, name, type);

	/* and save it */
	refresh_sequence_number(domain, False);
	wcache_save_sid_to_name(domain, status, sid, *domain_name, *name, *type);

	/* We can't save the name to sid mapping here, as with sid history a
	 * later name2sid would give the wrong sid. */

	return status;
}

static NTSTATUS rids_to_names(struct winbindd_domain *domain,
			      TALLOC_CTX *mem_ctx,
			      const DOM_SID *domain_sid,
			      uint32 *rids,
			      size_t num_rids,
			      char **domain_name,
			      char ***names,
			      enum lsa_SidType **types)
{
	struct winbind_cache *cache = get_cache(domain);
	size_t i;
	NTSTATUS result = NT_STATUS_UNSUCCESSFUL;
	BOOL have_mapped;
	BOOL have_unmapped;

	*domain_name = NULL;
	*names = NULL;
	*types = NULL;

	if (!cache->tdb) {
		goto do_query;
	}

	if (num_rids == 0) {
		return NT_STATUS_OK;
	}

	*names = TALLOC_ARRAY(mem_ctx, char *, num_rids);
	*types = TALLOC_ARRAY(mem_ctx, enum lsa_SidType, num_rids);

	if ((*names == NULL) || (*types == NULL)) {
		result = NT_STATUS_NO_MEMORY;
		goto error;
	}

	have_mapped = have_unmapped = False;

	for (i=0; i<num_rids; i++) {
		DOM_SID sid;
		struct cache_entry *centry;

		if (!sid_compose(&sid, domain_sid, rids[i])) {
			result = NT_STATUS_INTERNAL_ERROR;
			goto error;
		}

		centry = wcache_fetch(cache, domain, "SN/%s",
				      sid_string_static(&sid));
		if (!centry) {
			goto do_query;
		}

		(*types)[i] = SID_NAME_UNKNOWN;
		(*names)[i] = talloc_strdup(*names, "");

		if (NT_STATUS_IS_OK(centry->status)) {
			char *dom;
			have_mapped = True;
			(*types)[i] = (enum lsa_SidType)centry_uint32(centry);
			dom = centry_string(centry, mem_ctx);
			if (*domain_name == NULL) {
				*domain_name = dom;
			} else {
				talloc_free(dom);
			}
			(*names)[i] = centry_string(centry, *names);
		} else {
			have_unmapped = True;
		}

		centry_free(centry);
	}

	if (!have_mapped) {
		return NT_STATUS_NONE_MAPPED;
	}
	if (!have_unmapped) {
		return NT_STATUS_OK;
	}
	return STATUS_SOME_UNMAPPED;

 do_query:

	TALLOC_FREE(*names);
	TALLOC_FREE(*types);

	result = domain->backend->rids_to_names(domain, mem_ctx, domain_sid,
						rids, num_rids, domain_name,
						names, types);

	if (!NT_STATUS_IS_OK(result) &&
	    !NT_STATUS_EQUAL(result, STATUS_SOME_UNMAPPED)) {
		return result;
	}

	refresh_sequence_number(domain, False);

	for (i=0; i<num_rids; i++) {
		DOM_SID sid;
		NTSTATUS status;

		if (!sid_compose(&sid, domain_sid, rids[i])) {
			result = NT_STATUS_INTERNAL_ERROR;
			goto error;
		}

		status = (*types)[i] == SID_NAME_UNKNOWN ?
			NT_STATUS_NONE_MAPPED : NT_STATUS_OK;

		wcache_save_sid_to_name(domain, status, &sid, *domain_name,
					(*names)[i], (*types)[i]);
	}

	return result;

 error:
	
	TALLOC_FREE(*names);
	TALLOC_FREE(*types);
	return result;
}

/* Lookup user information from a rid */
static NTSTATUS query_user(struct winbindd_domain *domain, 
			   TALLOC_CTX *mem_ctx, 
			   const DOM_SID *user_sid, 
			   WINBIND_USERINFO *info)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "U/%s", sid_string_static(user_sid));
	
	/* If we have an access denied cache entry and a cached info3 in the
           samlogon cache then do a query.  This will force the rpc back end
           to return the info3 data. */

	if (NT_STATUS_V(domain->last_status) == NT_STATUS_V(NT_STATUS_ACCESS_DENIED) &&
	    netsamlogon_cache_have(user_sid)) {
		DEBUG(10, ("query_user: cached access denied and have cached info3\n"));
		domain->last_status = NT_STATUS_OK;
		centry_free(centry);
		goto do_query;
	}
	
	if (!centry)
		goto do_query;

	info->acct_name = centry_string(centry, mem_ctx);
	info->full_name = centry_string(centry, mem_ctx);
	info->homedir = centry_string(centry, mem_ctx);
	info->shell = centry_string(centry, mem_ctx);
	info->primary_gid = centry_uint32(centry);
	centry_sid(centry, mem_ctx, &info->user_sid);
	centry_sid(centry, mem_ctx, &info->group_sid);
	status = centry->status;

	DEBUG(10,("query_user: [Cached] - cached info for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	ZERO_STRUCTP(info);

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;
	
	DEBUG(10,("query_user: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));

	status = domain->backend->query_user(domain, mem_ctx, user_sid, info);

	/* and save it */
	refresh_sequence_number(domain, False);
	wcache_save_user(domain, status, info);

	return status;
}


/* Lookup groups a user is a member of. */
static NTSTATUS lookup_usergroups(struct winbindd_domain *domain,
				  TALLOC_CTX *mem_ctx,
				  const DOM_SID *user_sid, 
				  uint32 *num_groups, DOM_SID **user_gids)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	unsigned int i;
	fstring sid_string;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "UG/%s", sid_to_string(sid_string, user_sid));
	
	/* If we have an access denied cache entry and a cached info3 in the
           samlogon cache then do a query.  This will force the rpc back end
           to return the info3 data. */

	if (NT_STATUS_V(domain->last_status) == NT_STATUS_V(NT_STATUS_ACCESS_DENIED) &&
	    netsamlogon_cache_have(user_sid)) {
		DEBUG(10, ("lookup_usergroups: cached access denied and have cached info3\n"));
		domain->last_status = NT_STATUS_OK;
		centry_free(centry);
		goto do_query;
	}
	
	if (!centry)
		goto do_query;

	*num_groups = centry_uint32(centry);
	
	if (*num_groups == 0)
		goto do_cached;

	(*user_gids) = TALLOC_ARRAY(mem_ctx, DOM_SID, *num_groups);
	if (! (*user_gids)) {
		smb_panic_fn("lookup_usergroups out of memory");
	}
	for (i=0; i<(*num_groups); i++) {
		centry_sid(centry, mem_ctx, &(*user_gids)[i]);
	}

do_cached:	
	status = centry->status;

	DEBUG(10,("lookup_usergroups: [Cached] - cached info for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	(*num_groups) = 0;
	(*user_gids) = NULL;

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	DEBUG(10,("lookup_usergroups: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));

	status = domain->backend->lookup_usergroups(domain, mem_ctx, user_sid, num_groups, user_gids);

	if ( NT_STATUS_EQUAL(status, NT_STATUS_SYNCHRONIZATION_REQUIRED) )
		goto skip_save;
	
	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_groups);
	for (i=0; i<(*num_groups); i++) {
		centry_put_sid(centry, &(*user_gids)[i]);
	}	
	centry_end(centry, "UG/%s", sid_to_string(sid_string, user_sid));
	centry_free(centry);

skip_save:
	return status;
}

static NTSTATUS lookup_useraliases(struct winbindd_domain *domain,
				   TALLOC_CTX *mem_ctx,
				   uint32 num_sids, const DOM_SID *sids,
				   uint32 *num_aliases, uint32 **alias_rids)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	char *sidlist = talloc_strdup(mem_ctx, "");
	int i;

	if (!cache->tdb)
		goto do_query;

	if (num_sids == 0) {
		*num_aliases = 0;
		*alias_rids = NULL;
		return NT_STATUS_OK;
	}

	/* We need to cache indexed by the whole list of SIDs, the aliases
	 * resulting might come from any of the SIDs. */

	for (i=0; i<num_sids; i++) {
		sidlist = talloc_asprintf(mem_ctx, "%s/%s", sidlist,
					  sid_string_static(&sids[i]));
		if (sidlist == NULL)
			return NT_STATUS_NO_MEMORY;
	}

	centry = wcache_fetch(cache, domain, "UA%s", sidlist);

	if (!centry)
		goto do_query;

	*num_aliases = centry_uint32(centry);
	*alias_rids = NULL;

	if (*num_aliases) {
		(*alias_rids) = TALLOC_ARRAY(mem_ctx, uint32, *num_aliases);

		if ((*alias_rids) == NULL) {
			centry_free(centry);
			return NT_STATUS_NO_MEMORY;
		}
	} else {
		(*alias_rids) = NULL;
	}

	for (i=0; i<(*num_aliases); i++)
		(*alias_rids)[i] = centry_uint32(centry);

	status = centry->status;

	DEBUG(10,("lookup_useraliases: [Cached] - cached info for domain: %s "
		  "status %s\n", domain->name, nt_errstr(status)));

	centry_free(centry);
	return status;

 do_query:
	(*num_aliases) = 0;
	(*alias_rids) = NULL;

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	DEBUG(10,("lookup_usergroups: [Cached] - doing backend query for info "
		  "for domain %s\n", domain->name ));

	status = domain->backend->lookup_useraliases(domain, mem_ctx,
						     num_sids, sids,
						     num_aliases, alias_rids);

	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_aliases);
	for (i=0; i<(*num_aliases); i++)
		centry_put_uint32(centry, (*alias_rids)[i]);
	centry_end(centry, "UA%s", sidlist);
	centry_free(centry);

 skip_save:
	return status;
}


static NTSTATUS lookup_groupmem(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				const DOM_SID *group_sid, uint32 *num_names, 
				DOM_SID **sid_mem, char ***names, 
				uint32 **name_types)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	unsigned int i;
	fstring sid_string;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "GM/%s", sid_to_string(sid_string, group_sid));
	if (!centry)
		goto do_query;

	*num_names = centry_uint32(centry);
	
	if (*num_names == 0)
		goto do_cached;

	(*sid_mem) = TALLOC_ARRAY(mem_ctx, DOM_SID, *num_names);
	(*names) = TALLOC_ARRAY(mem_ctx, char *, *num_names);
	(*name_types) = TALLOC_ARRAY(mem_ctx, uint32, *num_names);

	if (! (*sid_mem) || ! (*names) || ! (*name_types)) {
		smb_panic_fn("lookup_groupmem out of memory");
	}

	for (i=0; i<(*num_names); i++) {
		centry_sid(centry, mem_ctx, &(*sid_mem)[i]);
		(*names)[i] = centry_string(centry, mem_ctx);
		(*name_types)[i] = centry_uint32(centry);
	}

do_cached:	
	status = centry->status;

	DEBUG(10,("lookup_groupmem: [Cached] - cached info for domain %s status: %s\n",
		domain->name, nt_errstr(status)));

	centry_free(centry);
	return status;

do_query:
	(*num_names) = 0;
	(*sid_mem) = NULL;
	(*names) = NULL;
	(*name_types) = NULL;
	
	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;

	DEBUG(10,("lookup_groupmem: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));

	status = domain->backend->lookup_groupmem(domain, mem_ctx, group_sid, num_names, 
						  sid_mem, names, name_types);

	/* and save it */
	refresh_sequence_number(domain, False);
	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;
	centry_put_uint32(centry, *num_names);
	for (i=0; i<(*num_names); i++) {
		centry_put_sid(centry, &(*sid_mem)[i]);
		centry_put_string(centry, (*names)[i]);
		centry_put_uint32(centry, (*name_types)[i]);
	}	
	centry_end(centry, "GM/%s", sid_to_string(sid_string, group_sid));
	centry_free(centry);

skip_save:
	return status;
}

/* find the sequence number for a domain */
static NTSTATUS sequence_number(struct winbindd_domain *domain, uint32 *seq)
{
	refresh_sequence_number(domain, False);

	*seq = domain->sequence_number;

	return NT_STATUS_OK;
}

/* enumerate trusted domains 
 * (we need to have the list of trustdoms in the cache when we go offline) -
 * Guenther */
static NTSTATUS trusted_domains(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_domains,
				char ***names,
				char ***alt_names,
				DOM_SID **dom_sids)
{
 	struct winbind_cache *cache = get_cache(domain);
 	struct cache_entry *centry = NULL;
 	NTSTATUS status;
	int i;
 
	if (!cache->tdb)
		goto do_query;
 
	centry = wcache_fetch(cache, domain, "TRUSTDOMS/%s", domain->name);
	
	if (!centry) {
 		goto do_query;
	}
 
	*num_domains = centry_uint32(centry);
	
	if (*num_domains) {
		(*names) 	= TALLOC_ARRAY(mem_ctx, char *, *num_domains);
		(*alt_names) 	= TALLOC_ARRAY(mem_ctx, char *, *num_domains);
		(*dom_sids) 	= TALLOC_ARRAY(mem_ctx, DOM_SID, *num_domains);
 
		if (! (*dom_sids) || ! (*names) || ! (*alt_names)) {
			smb_panic_fn("trusted_domains out of memory");
 		}
	} else {
		(*names) = NULL;
		(*alt_names) = NULL;
		(*dom_sids) = NULL;
	}
 
	for (i=0; i<(*num_domains); i++) {
		(*names)[i] = centry_string(centry, mem_ctx);
		(*alt_names)[i] = centry_string(centry, mem_ctx);
		centry_sid(centry, mem_ctx, &(*dom_sids)[i]);
	}

 	status = centry->status;
 
	DEBUG(10,("trusted_domains: [Cached] - cached info for domain %s (%d trusts) status: %s\n",
		domain->name, *num_domains, nt_errstr(status) ));
 
 	centry_free(centry);
 	return status;
 
do_query:
	(*num_domains) = 0;
	(*dom_sids) = NULL;
	(*names) = NULL;
	(*alt_names) = NULL;
 
	/* Return status value returned by seq number check */

 	if (!NT_STATUS_IS_OK(domain->last_status))
 		return domain->last_status;
	
	DEBUG(10,("trusted_domains: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));
 
	status = domain->backend->trusted_domains(domain, mem_ctx, num_domains,
						names, alt_names, dom_sids);

	/* no trusts gives NT_STATUS_NO_MORE_ENTRIES resetting to NT_STATUS_OK
	 * so that the generic centry handling still applies correctly -
	 * Guenther*/

	if (!NT_STATUS_IS_ERR(status)) {
		status = NT_STATUS_OK;
	}


#if 0    /* Disabled as we want the trust dom list to be managed by
	    the main parent and always to make the query.  --jerry */

	/* and save it */
	refresh_sequence_number(domain, False);
 
 	centry = centry_start(domain, status);
	if (!centry)
		goto skip_save;

	centry_put_uint32(centry, *num_domains);

	for (i=0; i<(*num_domains); i++) {
		centry_put_string(centry, (*names)[i]);
		centry_put_string(centry, (*alt_names)[i]);
		centry_put_sid(centry, &(*dom_sids)[i]);
 	}
	
	centry_end(centry, "TRUSTDOMS/%s", domain->name);
 
 	centry_free(centry);
 
skip_save:
#endif

 	return status;
}	

/* get lockout policy */
static NTSTATUS lockout_policy(struct winbindd_domain *domain,
 			       TALLOC_CTX *mem_ctx,
			       SAM_UNK_INFO_12 *policy){
 	struct winbind_cache *cache = get_cache(domain);
 	struct cache_entry *centry = NULL;
 	NTSTATUS status;
 
	if (!cache->tdb)
		goto do_query;
 
	centry = wcache_fetch(cache, domain, "LOC_POL/%s", domain->name);
	
	if (!centry)
 		goto do_query;
 
	policy->duration = centry_nttime(centry);
	policy->reset_count = centry_nttime(centry);
	policy->bad_attempt_lockout = centry_uint16(centry);
 
 	status = centry->status;
 
	DEBUG(10,("lockout_policy: [Cached] - cached info for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));
 
 	centry_free(centry);
 	return status;
 
do_query:
	ZERO_STRUCTP(policy);
 
	/* Return status value returned by seq number check */

 	if (!NT_STATUS_IS_OK(domain->last_status))
 		return domain->last_status;
	
	DEBUG(10,("lockout_policy: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));
 
	status = domain->backend->lockout_policy(domain, mem_ctx, policy); 
 
	/* and save it */
 	refresh_sequence_number(domain, False);
	wcache_save_lockout_policy(domain, status, policy);
 
 	return status;
}
 
/* get password policy */
static NTSTATUS password_policy(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				SAM_UNK_INFO_1 *policy)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS status;

	if (!cache->tdb)
		goto do_query;
 
	centry = wcache_fetch(cache, domain, "PWD_POL/%s", domain->name);
	
	if (!centry)
		goto do_query;

	policy->min_length_password = centry_uint16(centry);
	policy->password_history = centry_uint16(centry);
	policy->password_properties = centry_uint32(centry);
	policy->expire = centry_nttime(centry);
	policy->min_passwordage = centry_nttime(centry);

	status = centry->status;

	DEBUG(10,("lockout_policy: [Cached] - cached info for domain %s status: %s\n",
		domain->name, nt_errstr(status) ));

	centry_free(centry);
	return status;

do_query:
	ZERO_STRUCTP(policy);

	/* Return status value returned by seq number check */

	if (!NT_STATUS_IS_OK(domain->last_status))
		return domain->last_status;
	
	DEBUG(10,("password_policy: [Cached] - doing backend query for info for domain %s\n",
		domain->name ));

	status = domain->backend->password_policy(domain, mem_ctx, policy); 

	/* and save it */
	refresh_sequence_number(domain, False);
	wcache_save_password_policy(domain, status, policy);

	return status;
}


/* Invalidate cached user and group lists coherently */

static int traverse_fn(TDB_CONTEXT *the_tdb, TDB_DATA kbuf, TDB_DATA dbuf, 
		       void *state)
{
	if (strncmp((const char *)kbuf.dptr, "UL/", 3) == 0 ||
	    strncmp((const char *)kbuf.dptr, "GL/", 3) == 0)
		tdb_delete(the_tdb, kbuf);

	return 0;
}

/* Invalidate the getpwnam and getgroups entries for a winbindd domain */

void wcache_invalidate_samlogon(struct winbindd_domain *domain, 
				NET_USER_INFO_3 *info3)
{
	struct winbind_cache *cache;

	/* dont clear cached U/SID and UG/SID entries when we want to logon
	 * offline - gd */

	if (lp_winbind_offline_logon()) {
		return;
	}

	if (!domain)
		return;

	cache = get_cache(domain);
	netsamlogon_clear_cached_user(cache->tdb, info3);
}

void wcache_invalidate_cache(void)
{
	struct winbindd_domain *domain;

	for (domain = domain_list(); domain; domain = domain->next) {
		struct winbind_cache *cache = get_cache(domain);

		DEBUG(10, ("wcache_invalidate_cache: invalidating cache "
			   "entries for %s\n", domain->name));
		if (cache)
			tdb_traverse(cache->tdb, traverse_fn, NULL);
	}
}

BOOL init_wcache(void)
{
	if (wcache == NULL) {
		wcache = SMB_XMALLOC_P(struct winbind_cache);
		ZERO_STRUCTP(wcache);
	}

	if (wcache->tdb != NULL)
		return True;

	/* when working offline we must not clear the cache on restart */
	wcache->tdb = tdb_open_log(lock_path("winbindd_cache.tdb"),
				WINBINDD_CACHE_TDB_DEFAULT_HASH_SIZE, 
				lp_winbind_offline_logon() ? TDB_DEFAULT : (TDB_DEFAULT | TDB_CLEAR_IF_FIRST), 
				O_RDWR|O_CREAT, 0600);

	if (wcache->tdb == NULL) {
		DEBUG(0,("Failed to open winbindd_cache.tdb!\n"));
		return False;
	}

	return True;
}

/************************************************************************
 This is called by the parent to initialize the cache file.
 We don't need sophisticated locking here as we know we're the
 only opener.
************************************************************************/

BOOL initialize_winbindd_cache(void)
{
	BOOL cache_bad = True;
	uint32 vers;

	if (!init_wcache()) {
		DEBUG(0,("initialize_winbindd_cache: init_wcache failed.\n"));
		return False;
	}

	/* Check version number. */
	if (tdb_fetch_uint32(wcache->tdb, WINBINDD_CACHE_VERSION_KEYSTR, &vers) &&
			vers == WINBINDD_CACHE_VERSION) {
		cache_bad = False;
	}

	if (cache_bad) {
		DEBUG(0,("initialize_winbindd_cache: clearing cache "
			"and re-creating with version number %d\n",
			WINBINDD_CACHE_VERSION ));

		tdb_close(wcache->tdb);
		wcache->tdb = NULL;

		if (unlink(lock_path("winbindd_cache.tdb")) == -1) {
			DEBUG(0,("initialize_winbindd_cache: unlink %s failed %s ",
				lock_path("winbindd_cache.tdb"),
				strerror(errno) ));
			return False;
		}
		if (!init_wcache()) {
			DEBUG(0,("initialize_winbindd_cache: re-initialization "
					"init_wcache failed.\n"));
			return False;
		}

		/* Write the version. */
		if (!tdb_store_uint32(wcache->tdb, WINBINDD_CACHE_VERSION_KEYSTR, WINBINDD_CACHE_VERSION)) {
			DEBUG(0,("initialize_winbindd_cache: version number store failed %s\n",
				tdb_errorstr(wcache->tdb) ));
			return False;
		}
	}

	tdb_close(wcache->tdb);
	wcache->tdb = NULL;
	return True;
}

void cache_store_response(pid_t pid, struct winbindd_response *response)
{
	fstring key_str;

	if (!init_wcache())
		return;

	DEBUG(10, ("Storing response for pid %d, len %d\n",
		   pid, response->length));

	fstr_sprintf(key_str, "DR/%d", pid);
	if (tdb_store(wcache->tdb, string_tdb_data(key_str), 
		      make_tdb_data((uint8 *)response, sizeof(*response)),
		      TDB_REPLACE) == -1)
		return;

	if (response->length == sizeof(*response))
		return;

	/* There's extra data */

	DEBUG(10, ("Storing extra data: len=%d\n",
		   (int)(response->length - sizeof(*response))));

	fstr_sprintf(key_str, "DE/%d", pid);
	if (tdb_store(wcache->tdb, string_tdb_data(key_str),
		      make_tdb_data((uint8 *)response->extra_data.data,
				    response->length - sizeof(*response)),
		      TDB_REPLACE) == 0)
		return;

	/* We could not store the extra data, make sure the tdb does not
	 * contain a main record with wrong dangling extra data */

	fstr_sprintf(key_str, "DR/%d", pid);
	tdb_delete(wcache->tdb, string_tdb_data(key_str));

	return;
}

BOOL cache_retrieve_response(pid_t pid, struct winbindd_response * response)
{
	TDB_DATA data;
	fstring key_str;

	if (!init_wcache())
		return False;

	DEBUG(10, ("Retrieving response for pid %d\n", pid));

	fstr_sprintf(key_str, "DR/%d", pid);
	data = tdb_fetch(wcache->tdb, string_tdb_data(key_str));

	if (data.dptr == NULL)
		return False;

	if (data.dsize != sizeof(*response))
		return False;

	memcpy(response, data.dptr, data.dsize);
	SAFE_FREE(data.dptr);

	if (response->length == sizeof(*response)) {
		response->extra_data.data = NULL;
		return True;
	}

	/* There's extra data */

	DEBUG(10, ("Retrieving extra data length=%d\n",
		   (int)(response->length - sizeof(*response))));

	fstr_sprintf(key_str, "DE/%d", pid);
	data = tdb_fetch(wcache->tdb, string_tdb_data(key_str));

	if (data.dptr == NULL) {
		DEBUG(0, ("Did not find extra data\n"));
		return False;
	}

	if (data.dsize != (response->length - sizeof(*response))) {
		DEBUG(0, ("Invalid extra data length: %d\n", (int)data.dsize));
		SAFE_FREE(data.dptr);
		return False;
	}

	dump_data(11, (uint8 *)data.dptr, data.dsize);

	response->extra_data.data = data.dptr;
	return True;
}

void cache_cleanup_response(pid_t pid)
{
	fstring key_str;

	if (!init_wcache())
		return;

	fstr_sprintf(key_str, "DR/%d", pid);
	tdb_delete(wcache->tdb, string_tdb_data(key_str));

	fstr_sprintf(key_str, "DE/%d", pid);
	tdb_delete(wcache->tdb, string_tdb_data(key_str));

	return;
}


BOOL lookup_cached_sid(TALLOC_CTX *mem_ctx, const DOM_SID *sid,
		       char **domain_name, char **name,
		       enum lsa_SidType *type)
{
	struct winbindd_domain *domain;
	struct winbind_cache *cache;
	struct cache_entry *centry = NULL;
	NTSTATUS status;

	domain = find_lookup_domain_from_sid(sid);
	if (domain == NULL) {
		return False;
	}

	cache = get_cache(domain);

	if (cache->tdb == NULL) {
		return False;
	}

	centry = wcache_fetch(cache, domain, "SN/%s", sid_string_static(sid));
	if (centry == NULL) {
		return False;
	}

	if (NT_STATUS_IS_OK(centry->status)) {
		*type = (enum lsa_SidType)centry_uint32(centry);
		*domain_name = centry_string(centry, mem_ctx);
		*name = centry_string(centry, mem_ctx);
	}

	status = centry->status;
	centry_free(centry);
	return NT_STATUS_IS_OK(status);
}

BOOL lookup_cached_name(TALLOC_CTX *mem_ctx,
			const char *domain_name,
			const char *name,
			DOM_SID *sid,
			enum lsa_SidType *type)
{
	struct winbindd_domain *domain;
	struct winbind_cache *cache;
	struct cache_entry *centry = NULL;
	NTSTATUS status;
	fstring uname;
	BOOL original_online_state;	

	domain = find_lookup_domain_from_name(domain_name);
	if (domain == NULL) {
		return False;
	}

	cache = get_cache(domain);

	if (cache->tdb == NULL) {
		return False;
	}

	fstrcpy(uname, name);
	strupper_m(uname);
	
	/* If we are doing a cached logon, temporarily set the domain
	   offline so the cache won't expire the entry */
	
	original_online_state = domain->online;
	domain->online = False;
	centry = wcache_fetch(cache, domain, "NS/%s/%s", domain_name, uname);
	domain->online = original_online_state;
	
	if (centry == NULL) {
		return False;
	}

	if (NT_STATUS_IS_OK(centry->status)) {
		*type = (enum lsa_SidType)centry_uint32(centry);
		centry_sid(centry, mem_ctx, sid);
	}

	status = centry->status;
	centry_free(centry);
	
	return NT_STATUS_IS_OK(status);
}

void cache_name2sid(struct winbindd_domain *domain, 
		    const char *domain_name, const char *name,
		    enum lsa_SidType type, const DOM_SID *sid)
{
	refresh_sequence_number(domain, False);
	wcache_save_name_to_sid(domain, NT_STATUS_OK, domain_name, name,
				sid, type);
}

/*
 * The original idea that this cache only contains centries has
 * been blurred - now other stuff gets put in here. Ensure we
 * ignore these things on cleanup.
 */

static int traverse_fn_cleanup(TDB_CONTEXT *the_tdb, TDB_DATA kbuf, 
			       TDB_DATA dbuf, void *state)
{
	struct cache_entry *centry;

	if (is_non_centry_key(kbuf)) {
		return 0;
	}

	centry = wcache_fetch_raw((char *)kbuf.dptr);
	if (!centry) {
		return 0;
	}

	if (!NT_STATUS_IS_OK(centry->status)) {
		DEBUG(10,("deleting centry %s\n", (const char *)kbuf.dptr));
		tdb_delete(the_tdb, kbuf);
	}

	centry_free(centry);
	return 0;
}

/* flush the cache */
void wcache_flush_cache(void)
{
	if (!wcache)
		return;
	if (wcache->tdb) {
		tdb_close(wcache->tdb);
		wcache->tdb = NULL;
	}
	if (opt_nocache)
		return;

	/* when working offline we must not clear the cache on restart */
	wcache->tdb = tdb_open_log(lock_path("winbindd_cache.tdb"),
				WINBINDD_CACHE_TDB_DEFAULT_HASH_SIZE, 
				lp_winbind_offline_logon() ? TDB_DEFAULT : (TDB_DEFAULT | TDB_CLEAR_IF_FIRST), 
				O_RDWR|O_CREAT, 0600);

	if (!wcache->tdb) {
		DEBUG(0,("Failed to open winbindd_cache.tdb!\n"));
		return;
	}

	tdb_traverse(wcache->tdb, traverse_fn_cleanup, NULL);

	DEBUG(10,("wcache_flush_cache success\n"));
}

/* Count cached creds */

static int traverse_fn_cached_creds(TDB_CONTEXT *the_tdb, TDB_DATA kbuf, TDB_DATA dbuf, 
			 	    void *state)
{
	int *cred_count = (int*)state;
 
	if (strncmp((const char *)kbuf.dptr, "CRED/", 5) == 0) {
		(*cred_count)++;
	}
	return 0;
}

NTSTATUS wcache_count_cached_creds(struct winbindd_domain *domain, int *count)
{
	struct winbind_cache *cache = get_cache(domain);

	*count = 0;

	if (!cache->tdb) {
		return NT_STATUS_INTERNAL_DB_ERROR;
	}
 
	tdb_traverse(cache->tdb, traverse_fn_cached_creds, (void *)count);

	return NT_STATUS_OK;
}

struct cred_list {
	struct cred_list *prev, *next;
	TDB_DATA key;
	fstring name;
	time_t created;
};
static struct cred_list *wcache_cred_list;

static int traverse_fn_get_credlist(TDB_CONTEXT *the_tdb, TDB_DATA kbuf, TDB_DATA dbuf, 
				    void *state)
{
	struct cred_list *cred;

	if (strncmp((const char *)kbuf.dptr, "CRED/", 5) == 0) {

		cred = SMB_MALLOC_P(struct cred_list);
		if (cred == NULL) {
			DEBUG(0,("traverse_fn_remove_first_creds: failed to malloc new entry for list\n"));
			return -1;
		}

		ZERO_STRUCTP(cred);
		
		/* save a copy of the key */
		
		fstrcpy(cred->name, (const char *)kbuf.dptr);		
		DLIST_ADD(wcache_cred_list, cred);
	}
	
	return 0;
}

NTSTATUS wcache_remove_oldest_cached_creds(struct winbindd_domain *domain, const DOM_SID *sid) 
{
	struct winbind_cache *cache = get_cache(domain);
	NTSTATUS status;
	int ret;
	struct cred_list *cred, *oldest = NULL;

	if (!cache->tdb) {
		return NT_STATUS_INTERNAL_DB_ERROR;
	}

	/* we possibly already have an entry */
 	if (sid && NT_STATUS_IS_OK(wcache_cached_creds_exist(domain, sid))) {
	
		fstring key_str;

		DEBUG(11,("we already have an entry, deleting that\n"));

		fstr_sprintf(key_str, "CRED/%s", sid_string_static(sid));

		tdb_delete(cache->tdb, string_tdb_data(key_str));

		return NT_STATUS_OK;
	}

	ret = tdb_traverse(cache->tdb, traverse_fn_get_credlist, NULL);
	if (ret == 0) {
		return NT_STATUS_OK;
	} else if ((ret == -1) || (wcache_cred_list == NULL)) {
		return NT_STATUS_OBJECT_NAME_NOT_FOUND;
	}

	ZERO_STRUCTP(oldest);

	for (cred = wcache_cred_list; cred; cred = cred->next) {

		TDB_DATA data;
		time_t t;

		data = tdb_fetch(cache->tdb, string_tdb_data(cred->name));
		if (!data.dptr) {
			DEBUG(10,("wcache_remove_oldest_cached_creds: entry for [%s] not found\n", 
				cred->name));
			status = NT_STATUS_OBJECT_NAME_NOT_FOUND;
			goto done;
		}
	
		t = IVAL(data.dptr, 0);
		SAFE_FREE(data.dptr);

		if (!oldest) {
			oldest = SMB_MALLOC_P(struct cred_list);
			if (oldest == NULL) {
				status = NT_STATUS_NO_MEMORY;
				goto done;
			}

			fstrcpy(oldest->name, cred->name);
			oldest->created = t;
			continue;
		}

		if (t < oldest->created) {
			fstrcpy(oldest->name, cred->name);
			oldest->created = t;
		}
	}

	if (tdb_delete(cache->tdb, string_tdb_data(oldest->name)) == 0) {
		status = NT_STATUS_OK;
	} else {
		status = NT_STATUS_UNSUCCESSFUL;
	}
done:
	SAFE_FREE(wcache_cred_list);
	SAFE_FREE(oldest);
	
	return status;
}

/* Change the global online/offline state. */
BOOL set_global_winbindd_state_offline(void)
{
	TDB_DATA data;

	DEBUG(10,("set_global_winbindd_state_offline: offline requested.\n"));

	/* Only go offline if someone has created
	   the key "WINBINDD_OFFLINE" in the cache tdb. */

	if (wcache == NULL || wcache->tdb == NULL) {
		DEBUG(10,("set_global_winbindd_state_offline: wcache not open yet.\n"));
		return False;
	}

	if (!lp_winbind_offline_logon()) {
		DEBUG(10,("set_global_winbindd_state_offline: rejecting.\n"));
		return False;
	}

	if (global_winbindd_offline_state) {
		/* Already offline. */
		return True;
	}

	data = tdb_fetch_bystring( wcache->tdb, "WINBINDD_OFFLINE" );

	if (!data.dptr || data.dsize != 4) {
		DEBUG(10,("set_global_winbindd_state_offline: offline state not set.\n"));
		SAFE_FREE(data.dptr);
		return False;
	} else {
		DEBUG(10,("set_global_winbindd_state_offline: offline state set.\n"));
		global_winbindd_offline_state = True;
		SAFE_FREE(data.dptr);
		return True;
	}
}

void set_global_winbindd_state_online(void)
{
	DEBUG(10,("set_global_winbindd_state_online: online requested.\n"));

	if (!lp_winbind_offline_logon()) {
		DEBUG(10,("set_global_winbindd_state_online: rejecting.\n"));
		return;
	}

	if (!global_winbindd_offline_state) {
		/* Already online. */
		return;
	}
	global_winbindd_offline_state = False;

	if (!wcache->tdb) {
		return;
	}

	/* Ensure there is no key "WINBINDD_OFFLINE" in the cache tdb. */
	tdb_delete_bystring(wcache->tdb, "WINBINDD_OFFLINE");
}

BOOL get_global_winbindd_state_offline(void)
{
	return global_winbindd_offline_state;
}

/***********************************************************************
 Validate functions for all possible cache tdb keys.
***********************************************************************/

struct validation_status {
	BOOL tdb_error;
	BOOL bad_freelist;
	BOOL bad_entry;
	BOOL unknown_key;
	BOOL success;
};

static struct cache_entry *create_centry_validate(const char *kstr, TDB_DATA data, 
						  struct validation_status *state)
{
	struct cache_entry *centry;

	centry = SMB_XMALLOC_P(struct cache_entry);
	centry->data = (unsigned char *)memdup(data.dptr, data.dsize);
	if (!centry->data) {
		SAFE_FREE(centry);
		return NULL;
	}
	centry->len = data.dsize;
	centry->ofs = 0;

	if (centry->len < 8) {
		/* huh? corrupt cache? */
		DEBUG(0,("create_centry_validate: Corrupt cache for key %s (len < 8) ?\n", kstr));
		centry_free(centry);
		state->bad_entry = True;
		state->success = False;
		return NULL;
	}

	centry->status = NT_STATUS(centry_uint32(centry));
	centry->sequence_number = centry_uint32(centry);
	return centry;
}

static int validate_seqnum(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
			   struct validation_status *state)
{
	if (dbuf.dsize != 8) {
		DEBUG(0,("validate_seqnum: Corrupt cache for key %s (len %u != 8) ?\n",
				keystr, (unsigned int)dbuf.dsize ));
		state->bad_entry = True;
		return 1;
	}
	return 0;
}

static int validate_ns(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
		       struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);
	if (!centry) {
		return 1;
	}

	(void)centry_uint32(centry);
	if (NT_STATUS_IS_OK(centry->status)) {
		DOM_SID sid;
		(void)centry_sid(centry, mem_ctx, &sid);
	}

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_ns: %s ok\n", keystr));
	return 0;
}

static int validate_sn(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
		       struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);
	if (!centry) {
		return 1;
	}

	if (NT_STATUS_IS_OK(centry->status)) {
		(void)centry_uint32(centry);
		(void)centry_string(centry, mem_ctx);
		(void)centry_string(centry, mem_ctx);
	}

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_sn: %s ok\n", keystr));
	return 0;
}

static int validate_u(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
		      struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);
	DOM_SID sid;

	if (!centry) {
		return 1;
	}

	(void)centry_string(centry, mem_ctx);
	(void)centry_string(centry, mem_ctx);
	(void)centry_string(centry, mem_ctx);
	(void)centry_string(centry, mem_ctx);
	(void)centry_uint32(centry);
	(void)centry_sid(centry, mem_ctx, &sid);
	(void)centry_sid(centry, mem_ctx, &sid);

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_u: %s ok\n", keystr));
	return 0;
}

static int validate_loc_pol(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
			    struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);

	if (!centry) {
		return 1;
	}

	(void)centry_nttime(centry);
	(void)centry_nttime(centry);
	(void)centry_uint16(centry);

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_loc_pol: %s ok\n", keystr));
	return 0;
}

static int validate_pwd_pol(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
			    struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);

	if (!centry) {
		return 1;
	}

	(void)centry_uint16(centry);
	(void)centry_uint16(centry);
	(void)centry_uint32(centry);
	(void)centry_nttime(centry);
	(void)centry_nttime(centry);

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_pwd_pol: %s ok\n", keystr));
	return 0;
}

static int validate_cred(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
			 struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);

	if (!centry) {
		return 1;
	}

	(void)centry_time(centry);
	(void)centry_hash16(centry, mem_ctx);

	/* We only have 17 bytes more data in the salted cred case. */
	if (centry->len - centry->ofs == 17) {
		(void)centry_hash16(centry, mem_ctx);
	}

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_cred: %s ok\n", keystr));
	return 0;
}

static int validate_ul(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
		       struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);
	int32 num_entries, i;

	if (!centry) {
		return 1;
	}

	num_entries = (int32)centry_uint32(centry);

	for (i=0; i< num_entries; i++) {
		DOM_SID sid;
		(void)centry_string(centry, mem_ctx);
		(void)centry_string(centry, mem_ctx);
		(void)centry_string(centry, mem_ctx);
		(void)centry_string(centry, mem_ctx);
		(void)centry_sid(centry, mem_ctx, &sid);
		(void)centry_sid(centry, mem_ctx, &sid);
	}

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_ul: %s ok\n", keystr));
	return 0;
}

static int validate_gl(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
		       struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);
	int32 num_entries, i;

	if (!centry) {
		return 1;
	}

	num_entries = centry_uint32(centry);
	
	for (i=0; i< num_entries; i++) {
		(void)centry_string(centry, mem_ctx);
		(void)centry_string(centry, mem_ctx);
		(void)centry_uint32(centry);
	}

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_gl: %s ok\n", keystr));
	return 0;
}

static int validate_ug(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
		       struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);
	int32 num_groups, i;

	if (!centry) {
		return 1;
	}

	num_groups = centry_uint32(centry);

	for (i=0; i< num_groups; i++) {
		DOM_SID sid;
		centry_sid(centry, mem_ctx, &sid);
	}

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_ug: %s ok\n", keystr));
	return 0;
}

static int validate_ua(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
		       struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);
	int32 num_aliases, i;

	if (!centry) {
		return 1;
	}

	num_aliases = centry_uint32(centry);

	for (i=0; i < num_aliases; i++) {
		(void)centry_uint32(centry);
	}

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_ua: %s ok\n", keystr));
	return 0;
}

static int validate_gm(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
		       struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);
	int32 num_names, i;

	if (!centry) {
		return 1;
	}

	num_names = centry_uint32(centry);

	for (i=0; i< num_names; i++) {
		DOM_SID sid;
		centry_sid(centry, mem_ctx, &sid);
		(void)centry_string(centry, mem_ctx);
		(void)centry_uint32(centry);
	}

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_gm: %s ok\n", keystr));
	return 0;
}

static int validate_dr(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
		       struct validation_status *state)
{
	/* Can't say anything about this other than must be nonzero. */
	if (dbuf.dsize == 0) {
		DEBUG(0,("validate_dr: Corrupt cache for key %s (len == 0) ?\n",
				keystr));
		state->bad_entry = True;
		state->success = False;
		return 1;
	}

	DEBUG(10,("validate_dr: %s ok\n", keystr));
	return 0;
}

static int validate_de(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
		       struct validation_status *state)
{
	/* Can't say anything about this other than must be nonzero. */
	if (dbuf.dsize == 0) {
		DEBUG(0,("validate_de: Corrupt cache for key %s (len == 0) ?\n",
				keystr));
		state->bad_entry = True;
		state->success = False;
		return 1;
	}

	DEBUG(10,("validate_de: %s ok\n", keystr));
	return 0;
}

static int validate_trustdoms(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
			      struct validation_status *state)
{
	struct cache_entry *centry = create_centry_validate(keystr, dbuf, state);
	int32 num_domains, i;

	if (!centry) {
		return 1;
	}

	num_domains = centry_uint32(centry);
	
	for (i=0; i< num_domains; i++) {
		DOM_SID sid;
		(void)centry_string(centry, mem_ctx);
		(void)centry_string(centry, mem_ctx);
		(void)centry_sid(centry, mem_ctx, &sid);
	}

	centry_free(centry);

	if (!(state->success)) {
		return 1;
	}
	DEBUG(10,("validate_trustdoms: %s ok\n", keystr));
	return 0;
}

static int validate_trustdomcache(TALLOC_CTX *mem_ctx, const char *keystr, 
				  TDB_DATA dbuf,
				  struct validation_status *state)
{
	if (dbuf.dsize == 0) {
		DEBUG(0, ("validate_trustdomcache: Corrupt cache for "
			  "key %s (len ==0) ?\n", keystr));
		state->bad_entry = True;
		state->success = False;
		return 1;
	}

	DEBUG(10,    ("validate_trustdomcache: %s ok\n", keystr));
	DEBUGADD(10, ("  Don't trust me, I am a DUMMY!\n"));
	return 0;
}

static int validate_offline(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
			    struct validation_status *state)
{
	if (dbuf.dsize != 4) {
		DEBUG(0,("validate_offline: Corrupt cache for key %s (len %u != 4) ?\n",
				keystr, (unsigned int)dbuf.dsize ));
		state->bad_entry = True;
		state->success = False;
		return 1;
	}
	DEBUG(10,("validate_offline: %s ok\n", keystr));
	return 0;
}

static int validate_cache_version(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf,
				  struct validation_status *state)
{
	if (dbuf.dsize != 4) {
		DEBUG(0, ("validate_cache_version: Corrupt cache for "
			  "key %s (len %u != 4) ?\n", 
			  keystr, (unsigned int)dbuf.dsize));
		state->bad_entry = True;
		state->success = False;
		return 1;
	}

	DEBUG(10, ("validate_cache_version: %s ok\n", keystr));
	return 0;
}

/***********************************************************************
 A list of all possible cache tdb keys with associated validation
 functions.
***********************************************************************/

struct key_val_struct {
	const char *keyname;
	int (*validate_data_fn)(TALLOC_CTX *mem_ctx, const char *keystr, TDB_DATA dbuf, struct validation_status* state);
} key_val[] = {
	{"SEQNUM/", validate_seqnum},
	{"NS/", validate_ns},
	{"SN/", validate_sn},
	{"U/", validate_u},
	{"LOC_POL/", validate_loc_pol},
	{"PWD_POL/", validate_pwd_pol},
	{"CRED/", validate_cred},
	{"UL/", validate_ul},
	{"GL/", validate_gl},
	{"UG/", validate_ug},
	{"UA", validate_ua},
	{"GM/", validate_gm},
	{"DR/", validate_dr},
	{"DE/", validate_de},
	{"TRUSTDOMS/", validate_trustdoms},
	{"TRUSTDOMCACHE/", validate_trustdomcache},
	{"WINBINDD_OFFLINE", validate_offline},
	{WINBINDD_CACHE_VERSION_KEYSTR, validate_cache_version},
	{NULL, NULL}
};

/***********************************************************************
 Function to look at every entry in the tdb and validate it as far as
 possible.
***********************************************************************/

static int cache_traverse_validate_fn(TDB_CONTEXT *the_tdb, TDB_DATA kbuf, TDB_DATA dbuf, void *state)
{
	int i;
	struct validation_status *v_state = (struct validation_status *)state;

	/* Paranoia check. */
	if (kbuf.dsize > 1024) {
		DEBUG(0,("cache_traverse_validate_fn: key length too large (%u) > 1024\n\n",
				(unsigned int)kbuf.dsize ));
		return 1;
	}

	for (i = 0; key_val[i].keyname; i++) {
		size_t namelen = strlen(key_val[i].keyname);
		if (kbuf.dsize >= namelen && (
				strncmp(key_val[i].keyname, (const char *)kbuf.dptr, namelen)) == 0) {
			TALLOC_CTX *mem_ctx;
			char *keystr;
			int ret;

			keystr = SMB_MALLOC_ARRAY(char, kbuf.dsize+1);
			if (!keystr) {
				return 1;
			}
			memcpy(keystr, kbuf.dptr, kbuf.dsize);
			keystr[kbuf.dsize] = '\0';

			mem_ctx = talloc_init("validate_ctx");
			if (!mem_ctx) {
				SAFE_FREE(keystr);
				return 1;
			}

			ret = key_val[i].validate_data_fn(mem_ctx, keystr, dbuf, 
							  v_state);

			SAFE_FREE(keystr);
			talloc_destroy(mem_ctx);
			return ret;
		}
	}

	DEBUG(0,("cache_traverse_validate_fn: unknown cache entry\nkey :\n"));
	dump_data(0, (uint8 *)kbuf.dptr, kbuf.dsize);
	DEBUG(0,("data :\n"));
	dump_data(0, (uint8 *)dbuf.dptr, dbuf.dsize);
	v_state->unknown_key = True;
	v_state->success = False;
	return 1; /* terminate. */
}

static void validate_panic(const char *const why)
{
        DEBUG(0,("validating cache: would panic %s\n", why ));
	DEBUGADD(0, ("exiting instead (cache validation mode)\n"));
	exit(47);
}

/***********************************************************************
 Try and validate every entry in the winbindd cache. If we fail here,
 delete the cache tdb and return non-zero - the caller (main winbindd
 function) will restart us as we don't know if we crashed or not.
***********************************************************************/

/* 
 * internal validation function, executed by the child.  
 */
static int winbindd_validate_cache_child(const char *cache_path, int pfd)
{
	int ret = -1;
	int tfd = -1;
	int num_entries = 0;
	TDB_CONTEXT *tdb = NULL;
	struct validation_status v_status;
	
	v_status.tdb_error = False;
	v_status.bad_freelist = False;
	v_status.bad_entry = False;
	v_status.unknown_key = False;
	v_status.success = True;

	tdb = tdb_open_log(cache_path,
			WINBINDD_CACHE_TDB_DEFAULT_HASH_SIZE,
			lp_winbind_offline_logon() 
				?  TDB_DEFAULT 
				: (TDB_DEFAULT | TDB_CLEAR_IF_FIRST),
			O_RDWR|O_CREAT, 0600);
	if (!tdb) {
		v_status.tdb_error = True;
		v_status.success = False;
		goto out;
	}

	tfd = tdb_fd(tdb);

	/* Check the cache freelist is good. */
	if (tdb_validate_freelist(tdb, &num_entries) == -1) {
		DEBUG(0,("winbindd_validate_cache_child: bad freelist in cache %s\n",
			cache_path));
		v_status.bad_freelist = True;
		v_status.success = False;
		goto out;
	}

	DEBUG(10,("winbindd_validate_cache_child: cache %s freelist has %d entries\n",
		cache_path, num_entries));

	/* Now traverse the cache to validate it. */
	num_entries = tdb_traverse(tdb, cache_traverse_validate_fn, (void *)&v_status);
	if (num_entries == -1 || !(v_status.success)) {
		DEBUG(0,("winbindd_validate_cache_child: cache %s traverse failed\n",
			cache_path));
		if (!(v_status.success)) {
			if (v_status.bad_entry) {
				DEBUGADD(0, (" -> bad entry found\n"));
			}
			if (v_status.unknown_key) {
				DEBUGADD(0, (" -> unknown key encountered\n"));
			}
		}
		goto out;
	}

	DEBUG(10,("winbindd_validate_cache_child: cache %s is good "
		"with %d entries\n", cache_path, num_entries));
	ret = 0; /* Cache is good. */

out:
	if (tdb) {
		if (ret == 0) {
			tdb_close(tdb);
		} 
		else if (tfd != -1) {
			close(tfd);
		}
	}

	DEBUG(10, ("winbindd_validate_cache_child: writing status to pipe\n"));
	write (pfd, (const char *)&v_status, sizeof(v_status));
	close(pfd);

	return ret;
}

int winbindd_validate_cache(void)
{
	pid_t child_pid = -1;
	int child_status = 0;
	int wait_pid = 0;
	int ret = -1;
	int pipe_fds[2];
	struct validation_status v_status;
	int bytes_read = 0;
	const char *cache_path = lock_path("winbindd_cache.tdb");
	
	DEBUG(10, ("winbindd_validate_cache: replacing panic function\n"));
	smb_panic_fn = validate_panic;

	/* fork and let the child do the validation. 
	 * benefit: no need to twist signal handlers and panic functions.
	 * just let the child panic. we catch the signal. 
	 * communicate the extended status struct over a pipe. */

	if (pipe(pipe_fds) != 0) {
		DEBUG(0, ("winbindd_validate_cache: unable to create pipe, "
			  "error %s", strerror(errno)));
		smb_panic("winbind_validate_cache: unable to create pipe.");
	}

	DEBUG(10, ("winbindd_validate_cache: forking to let child do validation.\n"));
	child_pid = sys_fork();
	if (child_pid == 0) {
		DEBUG(10, ("winbindd_validate_cache (validation child): created\n"));
		close(pipe_fds[0]); /* close reading fd */
		DEBUG(10, ("winbindd_validate_cache (validation child): "
			   "calling winbindd_validate_cache_child\n"));
		exit(winbindd_validate_cache_child(cache_path, pipe_fds[1]));
	}
	else if (child_pid < 0) {
		smb_panic("winbindd_validate_cache: fork for validation failed.");
	}

	/* parent */

	DEBUG(10, ("winbindd_validate_cache: fork succeeded, child PID = %d\n", 
		   child_pid));
	close(pipe_fds[1]); /* close writing fd */

	v_status.success = True;
	v_status.bad_entry = False;
	v_status.unknown_key = False;

	DEBUG(10, ("winbindd_validate_cache: reading from pipe.\n"));
	bytes_read = read(pipe_fds[0], (void *)&v_status, sizeof(v_status));
	close(pipe_fds[0]);

	if (bytes_read != sizeof(v_status)) {
		DEBUG(10, ("winbindd_validate_cache: read %d bytes from pipe "
			   "but expected %d", bytes_read, sizeof(v_status)));
		DEBUGADD(10, (" -> assuming child crashed\n"));
		v_status.success = False;
	}
	else {
		DEBUG(10,    ("winbindd_validate_cache: read status from child\n"));
		DEBUGADD(10, (" *  tdb error: %s\n", v_status.tdb_error ? "yes" : "no"));
		DEBUGADD(10, (" *  bad freelist: %s\n", v_status.bad_freelist ? "yes" : "no"));
		DEBUGADD(10, (" *  bad entry: %s\n", v_status.bad_entry ? "yes" : "no"));
		DEBUGADD(10, (" *  unknown key: %s\n", v_status.unknown_key ? "yes" : "no"));
		DEBUGADD(10, (" => overall success: %s\n", v_status.success ? "yes" : "no"));
	}

	if (!v_status.success) {
		DEBUG(10, ("winbindd_validate_cache: validation not successful.\n"));
		DEBUGADD(10, ("removing tdb %s.\n", cache_path));
		unlink(cache_path);
	}

	DEBUG(10, ("winbindd_validate_cache: waiting for child to finish...\n"));
	while  ((wait_pid = sys_waitpid(child_pid, &child_status, 0)) < 0) {
		if (errno == EINTR) {
			DEBUG(10, ("winbindd_validate_cache: got signal during "
				   "waitpid, retrying\n"));
			errno = 0;
			continue;
		}
		DEBUG(0, ("winbindd_validate_cache: waitpid failed with "
                          "errno %s\n", strerror(errno)));
		smb_panic("winbindd_validate_cache: waitpid failed.");
	}
	if (wait_pid != child_pid) {
		DEBUG(0, ("winbindd_validate_cache: waitpid returned pid %d, "
			  "but %d was expexted\n", wait_pid, child_pid));
		smb_panic("winbindd_validate_cache: waitpid returned "
			     "unexpected PID.");
	}

		
	DEBUG(10, ("winbindd_validate_cache: validating child returned.\n"));
	if (WIFEXITED(child_status)) {
		DEBUG(10, ("winbindd_validate_cache: child exited, code %d.\n",
			   WEXITSTATUS(child_status)));
		ret = WEXITSTATUS(child_status);
	}
	if (WIFSIGNALED(child_status)) {
		DEBUG(10, ("winbindd_validate_cache: child terminated "
			   "by signal %d%s\n",
			   WTERMSIG(child_status),
#if defined(WCOREDUMP)
			   WCOREDUMP(child_status)?" (core dumped)":""
#else
			   ""
#endif
			   ));
		ret = WTERMSIG(child_status);
	}
	if (WIFSTOPPED(child_status)) {
		DEBUG(10, ("winbindd_validate_cache: child was stopped "
			   "by signal %d\n",
			   WSTOPSIG(child_status)));
		ret = WSTOPSIG(child_status);
	}

	DEBUG(10, ("winbindd_validate_cache: restoring panic function\n"));
	smb_panic_fn = smb_panic;
	return ret;
}

/*********************************************************************
 ********************************************************************/

static BOOL add_wbdomain_to_tdc_array( struct winbindd_domain *new_dom,
				       struct winbindd_tdc_domain **domains, 
				       size_t *num_domains )
{
	struct winbindd_tdc_domain *list = NULL;
	size_t idx;
	int i;
	BOOL set_only = False;	
	
	/* don't allow duplicates */

	idx = *num_domains;
	list = *domains;
	
	for ( i=0; i< (*num_domains); i++ ) {
		if ( strequal( new_dom->name, list[i].domain_name ) ) {
			DEBUG(10,("add_wbdomain_to_tdc_array: Found existing record for %s\n",
				  new_dom->name));
			idx = i;
			set_only = True;
			
			break;
		}
	}

	if ( !set_only ) {
		if ( !*domains ) {
			list = TALLOC_ARRAY( NULL, struct winbindd_tdc_domain, 1 );
			idx = 0;
		} else {
			list = TALLOC_REALLOC_ARRAY( *domains, *domains, 
						     struct winbindd_tdc_domain,  
						     (*num_domains)+1);
			idx = *num_domains;		
		}

		ZERO_STRUCT( list[idx] );
	}

	if ( !list )
		return False;

	list[idx].domain_name = talloc_strdup( list, new_dom->name );
	list[idx].dns_name = talloc_strdup( list, new_dom->alt_name );

	if ( !is_null_sid( &new_dom->sid ) )
		sid_copy( &list[idx].sid, &new_dom->sid );

	if ( new_dom->domain_flags != 0x0 )
		list[idx].trust_flags = new_dom->domain_flags;	

	if ( new_dom->domain_type != 0x0 )
		list[idx].trust_type = new_dom->domain_type;

	if ( new_dom->domain_trust_attribs != 0x0 )
		list[idx].trust_attribs = new_dom->domain_trust_attribs;
	
	if ( !set_only ) {
		*domains = list;
		*num_domains = idx + 1;	
	}

	return True;	
}

/*********************************************************************
 ********************************************************************/

static TDB_DATA make_tdc_key( const char *domain_name )
{
	char *keystr = NULL;
	TDB_DATA key = { NULL, 0 };
	
	if ( !domain_name ) {
		DEBUG(5,("make_tdc_key: Keyname workgroup is NULL!\n"));
		return key;
	}
	       
		
	asprintf( &keystr, "TRUSTDOMCACHE/%s", domain_name );
	key.dptr = (unsigned char*)keystr;
	key.dsize = strlen_m(keystr) + 1;
	
	return key;	
}

/*********************************************************************
 ********************************************************************/

static int pack_tdc_domains( struct winbindd_tdc_domain *domains, 
			     size_t num_domains,
			     unsigned char **buf )
{
        unsigned char *buffer = NULL;
	int len = 0;
	int buflen = 0;
	int i = 0;

	DEBUG(10,("pack_tdc_domains: Packing %d trusted domains\n",
		  (int)num_domains));
	
	buflen = 0;
	
 again: 
	len = 0;
	
	/* Store the number of array items first */
	len += tdb_pack( buffer+len, buflen-len, "d", 
			 num_domains );

	/* now pack each domain trust record */
	for ( i=0; i<num_domains; i++ ) {

		if ( buflen > 0 ) {
			DEBUG(10,("pack_tdc_domains: Packing domain %s (%s)\n",
				  domains[i].domain_name,
				  domains[i].dns_name ? domains[i].dns_name : "UNKNOWN" ));
		}
		
		len += tdb_pack( buffer+len, buflen-len, "fffddd",
				 domains[i].domain_name,
				 domains[i].dns_name,
				 sid_string_static(&domains[i].sid),
				 domains[i].trust_flags,
				 domains[i].trust_attribs,
				 domains[i].trust_type );
	}

	if ( buflen < len ) {
		if ( (buffer = SMB_MALLOC_ARRAY(unsigned char, len)) == NULL ) {
			DEBUG(0,("pack_tdc_domains: failed to alloc buffer!\n"));
			buflen = -1;
			goto done;
		}
		buflen = len;
		goto again;
	}

	*buf = buffer;	
	
 done:	
	return buflen;	
}

/*********************************************************************
 ********************************************************************/

static size_t unpack_tdc_domains( unsigned char *buf, int buflen, 
				  struct winbindd_tdc_domain **domains )
{
	fstring domain_name, dns_name, sid_string;	
	uint32 type, attribs, flags;
	int num_domains;
	int len = 0;
	int i;
	struct winbindd_tdc_domain *list = NULL;

	/* get the number of domains */
	len += tdb_unpack( buf+len, buflen-len, "d", &num_domains);
	if ( len == -1 ) {
		DEBUG(5,("unpack_tdc_domains: Failed to unpack domain array\n"));		
		return 0;
	}

	list = TALLOC_ARRAY( NULL, struct winbindd_tdc_domain, num_domains );
	if ( !list ) {
		DEBUG(0,("unpack_tdc_domains: Failed to talloc() domain list!\n"));
		return 0;		
	}
	
	for ( i=0; i<num_domains; i++ ) {
		len += tdb_unpack( buf+len, buflen-len, "fffddd",
				   domain_name,
				   dns_name,
				   sid_string,
				   &flags,
				   &attribs,
				   &type );

		if ( len == -1 ) {
			DEBUG(5,("unpack_tdc_domains: Failed to unpack domain array\n"));
			TALLOC_FREE( list );			
			return 0;
		}

		DEBUG(11,("unpack_tdc_domains: Unpacking domain %s (%s) "
			  "SID %s, flags = 0x%x, attribs = 0x%x, type = 0x%x\n",
			  domain_name, dns_name, sid_string,
			  flags, attribs, type));
		
		list[i].domain_name = talloc_strdup( list, domain_name );
		list[i].dns_name = talloc_strdup( list, dns_name );
		if ( !string_to_sid( &(list[i].sid), sid_string ) ) {			
			DEBUG(10,("unpack_tdc_domains: no SID for domain %s\n",
				  domain_name));
		}
		list[i].trust_flags = flags;
		list[i].trust_attribs = attribs;
		list[i].trust_type = type;
	}

	*domains = list;
	
	return num_domains;
}

/*********************************************************************
 ********************************************************************/

static BOOL wcache_tdc_store_list( struct winbindd_tdc_domain *domains, size_t num_domains )
{
	TDB_DATA key = make_tdc_key( lp_workgroup() );	 
	TDB_DATA data = { NULL, 0 };
	int ret;
	
	if ( !key.dptr )
		return False;
	
	/* See if we were asked to delete the cache entry */

	if ( !domains ) {
		ret = tdb_delete( wcache->tdb, key );
		goto done;
	}
	
	data.dsize = pack_tdc_domains( domains, num_domains, &data.dptr );
	
	if ( !data.dptr ) {
		ret = -1;
		goto done;
	}
		
	ret = tdb_store( wcache->tdb, key, data, 0 );

 done:
	SAFE_FREE( data.dptr );
	SAFE_FREE( key.dptr );
	
	return ( ret != -1 );	
}

/*********************************************************************
 ********************************************************************/

BOOL wcache_tdc_fetch_list( struct winbindd_tdc_domain **domains, size_t *num_domains )
{
	TDB_DATA key = make_tdc_key( lp_workgroup() );
	TDB_DATA data = { NULL, 0 };

	*domains = NULL;	
	*num_domains = 0;	

	if ( !key.dptr )
		return False;
	
	data = tdb_fetch( wcache->tdb, key );

	SAFE_FREE( key.dptr );
	
	if ( !data.dptr ) 
		return False;
	
	*num_domains = unpack_tdc_domains( data.dptr, data.dsize, domains );

	SAFE_FREE( data.dptr );
	
	if ( !*domains )
		return False;

	return True;	
}

/*********************************************************************
 ********************************************************************/

BOOL wcache_tdc_add_domain( struct winbindd_domain *domain )
{
	struct winbindd_tdc_domain *dom_list = NULL;
	size_t num_domains = 0;
	BOOL ret = False;	

	DEBUG(10,("wcache_tdc_add_domain: Adding domain %s (%s), SID %s, "
		  "flags = 0x%x, attributes = 0x%x, type = 0x%x\n",
		  domain->name, domain->alt_name, 
		  sid_string_static(&domain->sid),
		  domain->domain_flags,
		  domain->domain_trust_attribs,
		  domain->domain_type));	
	
	if ( !init_wcache() ) {
		return False;
	}
	
	/* fetch the list */

	wcache_tdc_fetch_list( &dom_list, &num_domains );
	
	/* add the new domain */

	if ( !add_wbdomain_to_tdc_array( domain, &dom_list, &num_domains ) ) {
		goto done;		
	}	

	/* pack the domain */

	if ( !wcache_tdc_store_list( dom_list, num_domains ) ) {
		goto done;		
	}
	
	/* Success */

	ret = True;	
 done:
	TALLOC_FREE( dom_list );
	
	return ret;	
}

/*********************************************************************
 ********************************************************************/

struct winbindd_tdc_domain * wcache_tdc_fetch_domain( TALLOC_CTX *ctx, const char *name )
{
	struct winbindd_tdc_domain *dom_list = NULL;
	size_t num_domains = 0;
	int i;
	struct winbindd_tdc_domain *d = NULL;	

	DEBUG(10,("wcache_tdc_fetch_domain: Searching for domain %s\n", name));

	if ( !init_wcache() ) {
		return False;
	}
	
	/* fetch the list */

	wcache_tdc_fetch_list( &dom_list, &num_domains );
	
	for ( i=0; i<num_domains; i++ ) {
		if ( strequal(name, dom_list[i].domain_name) ||
		     strequal(name, dom_list[i].dns_name) )
		{
			DEBUG(10,("wcache_tdc_fetch_domain: Found domain %s\n",
				  name));
			
			d = TALLOC_P( ctx, struct winbindd_tdc_domain );
			if ( !d )
				break;			
			
			d->domain_name = talloc_strdup( d, dom_list[i].domain_name );
			d->dns_name = talloc_strdup( d, dom_list[i].dns_name );
			sid_copy( &d->sid, &dom_list[i].sid );
			d->trust_flags   = dom_list[i].trust_flags;
			d->trust_type    = dom_list[i].trust_type;
			d->trust_attribs = dom_list[i].trust_attribs;

			break;
		}
	}

        TALLOC_FREE( dom_list );
	
	return d;	
}


/*********************************************************************
 ********************************************************************/

void wcache_tdc_clear( void )
{
	if ( !init_wcache() )
		return;

	wcache_tdc_store_list( NULL, 0 );
	
	return;	
}


/*********************************************************************
 ********************************************************************/

static void wcache_save_user_pwinfo(struct winbindd_domain *domain, 
				    NTSTATUS status,
				    const DOM_SID *user_sid,
				    const char *homedir,
				    const char *shell,
				    const char *gecos,
				    uint32 gid)
{
	struct cache_entry *centry;

	if ( (centry = centry_start(domain, status)) == NULL )
		return;

	centry_put_string( centry, homedir );
	centry_put_string( centry, shell );
	centry_put_string( centry, gecos );
	centry_put_uint32( centry, gid );
	
	centry_end(centry, "NSS/PWINFO/%s", sid_string_static(user_sid) );

	DEBUG(10,("wcache_save_user_pwinfo: %s\n", sid_string_static(user_sid) ));

	centry_free(centry);
}

NTSTATUS nss_get_info_cached( struct winbindd_domain *domain, 
			      const DOM_SID *user_sid,
			      TALLOC_CTX *ctx,
			      ADS_STRUCT *ads, LDAPMessage *msg,
			      char **homedir, char **shell, char **gecos,
			      gid_t *p_gid)
{
	struct winbind_cache *cache = get_cache(domain);
	struct cache_entry *centry = NULL;
	NTSTATUS nt_status;

	if (!cache->tdb)
		goto do_query;

	centry = wcache_fetch(cache, domain, "NSS/PWINFO/%s", sid_string_static(user_sid));	
	
	if (!centry)
		goto do_query;

	*homedir = centry_string( centry, ctx );
	*shell   = centry_string( centry, ctx );
	*gecos   = centry_string( centry, ctx );
	*p_gid   = centry_uint32( centry );	

	centry_free(centry);

	DEBUG(10,("nss_get_info_cached: [Cached] - user_sid %s\n",
		  sid_string_static(user_sid)));

	return NT_STATUS_OK;

do_query:
	
	nt_status = nss_get_info( domain->name, user_sid, ctx, ads, msg, 
				  homedir, shell, gecos, p_gid );

	if ( NT_STATUS_IS_OK(nt_status) ) {
		wcache_save_user_pwinfo( domain, nt_status, user_sid,
					 *homedir, *shell, *gecos, *p_gid );
	}	

	if ( NT_STATUS_EQUAL( nt_status, NT_STATUS_DOMAIN_CONTROLLER_NOT_FOUND ) ) {
		DEBUG(5,("nss_get_info_cached: Setting domain %s offline\n",
			 domain->name ));
		set_domain_offline( domain );
	}

	return nt_status;	
}


/* the cache backend methods are exposed via this structure */
struct winbindd_methods cache_methods = {
	True,
	query_user_list,
	enum_dom_groups,
	enum_local_groups,
	name_to_sid,
	sid_to_name,
	rids_to_names,
	query_user,
	lookup_usergroups,
	lookup_useraliases,
	lookup_groupmem,
	sequence_number,
	lockout_policy,
	password_policy,
	trusted_domains
};
