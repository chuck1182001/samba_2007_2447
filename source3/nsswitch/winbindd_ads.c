/*
   Unix SMB/CIFS implementation.

   Winbind ADS backend functions

   Copyright (C) Andrew Tridgell 2001
   Copyright (C) Andrew Bartlett <abartlet@samba.org> 2003
   Copyright (C) Gerald (Jerry) Carter 2004
   
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

#ifdef HAVE_ADS

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_WINBIND

extern struct winbindd_methods reconnect_methods;

/*
  return our ads connections structure for a domain. We keep the connection
  open to make things faster
*/
static ADS_STRUCT *ads_cached_connection(struct winbindd_domain *domain)
{
	ADS_STRUCT *ads;
	ADS_STATUS status;
	fstring dc_name;
	struct in_addr dc_ip;	

	DEBUG(10,("ads_cached_connection\n"));

	if (domain->private_data) {

		time_t expire;
		time_t now = time(NULL);

		/* check for a valid structure */
		ads = (ADS_STRUCT *)domain->private_data;

		expire = MIN(ads->auth.tgt_expire, ads->auth.tgs_expire);

		DEBUG(7, ("Current tickets expire in %d seconds (at %d, time is now %d)\n",
			  (uint32)expire-(uint32)now, (uint32) expire, (uint32) now));

		if ( ads->config.realm && (expire > now)) {
			return ads;
		} else {
			/* we own this ADS_STRUCT so make sure it goes away */
			DEBUG(7,("Deleting expired krb5 credential cache\n"));
			ads->is_mine = True;
			ads_destroy( &ads );
			ads_kdestroy("MEMORY:winbind_ccache");
			domain->private_data = NULL;
		}	
	}

	/* we don't want this to affect the users ccache */
	setenv("KRB5CCNAME", "MEMORY:winbind_ccache", 1);

	ads = ads_init(domain->alt_name, domain->name, NULL);
	if (!ads) {
		DEBUG(1,("ads_init for domain %s failed\n", domain->name));
		return NULL;
	}

	/* the machine acct password might have change - fetch it every time */

	SAFE_FREE(ads->auth.password);
	SAFE_FREE(ads->auth.realm);

	if ( IS_DC ) {
		DOM_SID sid;
		time_t last_set_time;

		if ( !pdb_get_trusteddom_pw( domain->name, &ads->auth.password, &sid, &last_set_time ) ) {
			ads_destroy( &ads );
			return NULL;
		}
		ads->auth.realm = SMB_STRDUP( ads->server.realm );
		strupper_m( ads->auth.realm );
	}
	else {
		struct winbindd_domain *our_domain = domain;

		ads->auth.password = secrets_fetch_machine_password(lp_workgroup(), NULL, NULL);

		/* always give preference to the alt_name in our 
		   primary domain if possible */

		if ( !domain->primary )
			our_domain = find_our_domain();

		if ( our_domain->alt_name[0] != '\0' ) {
			ads->auth.realm = SMB_STRDUP( our_domain->alt_name );
			strupper_m( ads->auth.realm );
		}
		else
			ads->auth.realm = SMB_STRDUP( lp_realm() );
	}

	ads->auth.renewable = WINBINDD_PAM_AUTH_KRB5_RENEW_TIME;

	/* Setup the server affinity cache.  We don't reaally care
	   about the name.  Just setup affinity and the KRB5_CONFIG 
	   file. */

	get_dc_name( ads->server.workgroup, ads->server.realm, dc_name, &dc_ip );
	
	status = ads_connect(ads);
	if (!ADS_ERR_OK(status) || !ads->config.realm) {
		DEBUG(1,("ads_connect for domain %s failed: %s\n", 
			 domain->name, ads_errstr(status)));
		ads_destroy(&ads);

		/* if we get ECONNREFUSED then it might be a NT4
                   server, fall back to MSRPC */
		if (status.error_type == ENUM_ADS_ERROR_SYSTEM &&
		    status.err.rc == ECONNREFUSED) {
			/* 'reconnect_methods' is the MS-RPC backend. */
			DEBUG(1,("Trying MSRPC methods\n"));
			domain->backend = &reconnect_methods;
		}
		return NULL;
	}

	/* set the flag that says we don't own the memory even 
	   though we do so that ads_destroy() won't destroy the 
	   structure we pass back by reference */

	ads->is_mine = False;

	domain->private_data = (void *)ads;
	return ads;
}


/* Query display info for a realm. This is the basic user list fn */
static NTSTATUS query_user_list(struct winbindd_domain *domain,
			       TALLOC_CTX *mem_ctx,
			       uint32 *num_entries, 
			       WINBIND_USERINFO **info)
{
	ADS_STRUCT *ads = NULL;
	const char *attrs[] = { "*", NULL };
	int i, count;
	ADS_STATUS rc;
	LDAPMessage *res = NULL;
	LDAPMessage *msg = NULL;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;

	*num_entries = 0;

	DEBUG(3,("ads: query_user_list\n"));

	if ( !winbindd_can_contact_domain( domain ) ) {
		DEBUG(10,("query_user_list: No incoming trust for domain %s\n",
			  domain->name));		
		return NT_STATUS_OK;
	}

	ads = ads_cached_connection(domain);
	
	if (!ads) {
		domain->last_status = NT_STATUS_SERVER_DISABLED;
		goto done;
	}

	rc = ads_search_retry(ads, &res, "(objectCategory=user)", attrs);
	if (!ADS_ERR_OK(rc) || !res) {
		DEBUG(1,("query_user_list ads_search: %s\n", ads_errstr(rc)));
		goto done;
	}

	count = ads_count_replies(ads, res);
	if (count == 0) {
		DEBUG(1,("query_user_list: No users found\n"));
		goto done;
	}

	(*info) = TALLOC_ZERO_ARRAY(mem_ctx, WINBIND_USERINFO, count);
	if (!*info) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	i = 0;

	for (msg = ads_first_entry(ads, res); msg; msg = ads_next_entry(ads, msg)) {
		char *name, *gecos = NULL;
		char *homedir = NULL;
		char *shell = NULL;
		uint32 group;
		uint32 atype;
		DOM_SID user_sid;
		gid_t primary_gid = (gid_t)-1;

		if (!ads_pull_uint32(ads, msg, "sAMAccountType", &atype) ||
		    ads_atype_map(atype) != SID_NAME_USER) {
			DEBUG(1,("Not a user account? atype=0x%x\n", atype));
			continue;
		}

		name = ads_pull_username(ads, mem_ctx, msg);

		if ( ads_pull_sid( ads, msg, "objectSid", &user_sid ) ) {
			status = nss_get_info_cached( domain, &user_sid, mem_ctx, 
					       ads, msg, &homedir, &shell, &gecos,
					       &primary_gid );
		}

		if (gecos == NULL) {
			gecos = ads_pull_string(ads, mem_ctx, msg, "name");
		}
	
		if (!ads_pull_sid(ads, msg, "objectSid",
				  &(*info)[i].user_sid)) {
			DEBUG(1,("No sid for %s !?\n", name));
			continue;
		}
		if (!ads_pull_uint32(ads, msg, "primaryGroupID", &group)) {
			DEBUG(1,("No primary group for %s !?\n", name));
			continue;
		}

		(*info)[i].acct_name = name;
		(*info)[i].full_name = gecos;
		(*info)[i].homedir = homedir;
		(*info)[i].shell = shell;
		(*info)[i].primary_gid = primary_gid;
		sid_compose(&(*info)[i].group_sid, &domain->sid, group);
		i++;
	}

	(*num_entries) = i;
	status = NT_STATUS_OK;

	DEBUG(3,("ads query_user_list gave %d entries\n", (*num_entries)));

done:
	if (res) 
		ads_msgfree(ads, res);

	return status;
}

/* list all domain groups */
static NTSTATUS enum_dom_groups(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				struct acct_info **info)
{
	ADS_STRUCT *ads = NULL;
	const char *attrs[] = {"userPrincipalName", "sAMAccountName",
			       "name", "objectSid", NULL};
	int i, count;
	ADS_STATUS rc;
	LDAPMessage *res = NULL;
	LDAPMessage *msg = NULL;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	const char *filter;
	BOOL enum_dom_local_groups = False;

	*num_entries = 0;

	DEBUG(3,("ads: enum_dom_groups\n"));

	if ( !winbindd_can_contact_domain( domain ) ) {
		DEBUG(10,("enum_dom_groups: No incoming trust for domain %s\n",
			  domain->name));		
		return NT_STATUS_OK;
	}

	/* only grab domain local groups for our domain */
	if ( domain->active_directory && strequal(lp_realm(), domain->alt_name)  ) {
		enum_dom_local_groups = True;
	}

	/* Workaround ADS LDAP bug present in MS W2K3 SP0 and W2K SP4 w/o
	 * rollup-fixes:
	 *
	 * According to Section 5.1(4) of RFC 2251 if a value of a type is it's
	 * default value, it MUST be absent. In case of extensible matching the
	 * "dnattr" boolean defaults to FALSE and so it must be only be present
	 * when set to TRUE. 
	 *
	 * When it is set to FALSE and the OpenLDAP lib (correctly) encodes a
	 * filter using bitwise matching rule then the buggy AD fails to decode
	 * the extensible match. As a workaround set it to TRUE and thereby add
	 * the dnAttributes "dn" field to cope with those older AD versions.
	 * It should not harm and won't put any additional load on the AD since
	 * none of the dn components have a bitmask-attribute.
	 *
	 * Thanks to Ralf Haferkamp for input and testing - Guenther */

	filter = talloc_asprintf(mem_ctx, "(&(objectCategory=group)(&(groupType:dn:%s:=%d)(!(groupType:dn:%s:=%d))))", 
				 ADS_LDAP_MATCHING_RULE_BIT_AND, GROUP_TYPE_SECURITY_ENABLED,
				 ADS_LDAP_MATCHING_RULE_BIT_AND, 
				 enum_dom_local_groups ? GROUP_TYPE_BUILTIN_LOCAL_GROUP : GROUP_TYPE_RESOURCE_GROUP);

	if (filter == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	ads = ads_cached_connection(domain);

	if (!ads) {
		domain->last_status = NT_STATUS_SERVER_DISABLED;
		goto done;
	}

	rc = ads_search_retry(ads, &res, filter, attrs);
	if (!ADS_ERR_OK(rc) || !res) {
		DEBUG(1,("enum_dom_groups ads_search: %s\n", ads_errstr(rc)));
		goto done;
	}

	count = ads_count_replies(ads, res);
	if (count == 0) {
		DEBUG(1,("enum_dom_groups: No groups found\n"));
		goto done;
	}

	(*info) = TALLOC_ZERO_ARRAY(mem_ctx, struct acct_info, count);
	if (!*info) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	i = 0;
	
	for (msg = ads_first_entry(ads, res); msg; msg = ads_next_entry(ads, msg)) {
		char *name, *gecos;
		DOM_SID sid;
		uint32 rid;

		name = ads_pull_username(ads, mem_ctx, msg);
		gecos = ads_pull_string(ads, mem_ctx, msg, "name");
		if (!ads_pull_sid(ads, msg, "objectSid", &sid)) {
			DEBUG(1,("No sid for %s !?\n", name));
			continue;
		}

		if (!sid_peek_check_rid(&domain->sid, &sid, &rid)) {
			DEBUG(1,("No rid for %s !?\n", name));
			continue;
		}

		fstrcpy((*info)[i].acct_name, name);
		fstrcpy((*info)[i].acct_desc, gecos);
		(*info)[i].rid = rid;
		i++;
	}

	(*num_entries) = i;

	status = NT_STATUS_OK;

	DEBUG(3,("ads enum_dom_groups gave %d entries\n", (*num_entries)));

done:
	if (res) 
		ads_msgfree(ads, res);

	return status;
}

/* list all domain local groups */
static NTSTATUS enum_local_groups(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_entries, 
				struct acct_info **info)
{
	/*
	 * This is a stub function only as we returned the domain 
	 * local groups in enum_dom_groups() if the domain->native field
	 * was true.  This is a simple performance optimization when
	 * using LDAP.
	 *
	 * if we ever need to enumerate domain local groups separately, 
	 * then this the optimization in enum_dom_groups() will need 
	 * to be split out
	 */
	*num_entries = 0;
	
	return NT_STATUS_OK;
}

/* convert a DN to a name, SID and name type 
   this might become a major speed bottleneck if groups have
   lots of users, in which case we could cache the results
*/
static BOOL dn_lookup(ADS_STRUCT *ads, TALLOC_CTX *mem_ctx,
		      const char *dn,
		      char **name, uint32 *name_type, DOM_SID *sid)
{
	LDAPMessage *res = NULL;
	const char *attrs[] = {"userPrincipalName", "sAMAccountName",
			       "objectSid", "sAMAccountType", NULL};
	ADS_STATUS rc;
	uint32 atype;
	DEBUG(3,("ads: dn_lookup\n"));

	rc = ads_search_retry_dn(ads, &res, dn, attrs);

	if (!ADS_ERR_OK(rc) || !res) {
		goto failed;
	}

	(*name) = ads_pull_username(ads, mem_ctx, res);

	if (!ads_pull_uint32(ads, res, "sAMAccountType", &atype)) {
		goto failed;
	}
	(*name_type) = ads_atype_map(atype);

	if (!ads_pull_sid(ads, res, "objectSid", sid)) {
		goto failed;
	}

	if (res) 
		ads_msgfree(ads, res);

	return True;

failed:
	if (res) 
		ads_msgfree(ads, res);

	return False;
}

/* Lookup user information from a rid */
static NTSTATUS query_user(struct winbindd_domain *domain, 
			   TALLOC_CTX *mem_ctx, 
			   const DOM_SID *sid, 
			   WINBIND_USERINFO *info)
{
	ADS_STRUCT *ads = NULL;
	const char *attrs[] = { "*", NULL };
	ADS_STATUS rc;
	int count;
	LDAPMessage *msg = NULL;
	char *ldap_exp;
	char *sidstr;
	uint32 group_rid;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	NET_USER_INFO_3 *user;

	DEBUG(3,("ads: query_user\n"));

	info->homedir = NULL;
	info->shell = NULL;
	info->primary_gid = (gid_t)-1;

	/* try netsamlogon cache first */
			
	if ( (user = netsamlogon_cache_get( mem_ctx, sid )) != NULL ) 
	{
				
		DEBUG(5,("query_user: Cache lookup succeeded for %s\n", 
			sid_string_static(sid)));

		sid_compose(&info->user_sid, &domain->sid, user->user_rid);
		sid_compose(&info->group_sid, &domain->sid, user->group_rid);
				
		info->acct_name = unistr2_tdup(mem_ctx, &user->uni_user_name);
		info->full_name = unistr2_tdup(mem_ctx, &user->uni_full_name);
		
		nss_get_info_cached( domain, sid, mem_ctx, NULL, NULL, 
			      &info->homedir, &info->shell, &info->full_name, 
			      &info->primary_gid );	

		TALLOC_FREE(user);
				
		return NT_STATUS_OK;
	}

	if ( !winbindd_can_contact_domain(domain)) {
		DEBUG(8,("query_user: No incoming trust from domain %s\n",
			 domain->name));

		/* We still need to generate some basic information
		   about the user even if we cannot contact the 
		   domain.  Most of this stuff we can deduce. */

		sid_copy( &info->user_sid, sid );

		/* Assume "Domain Users" for the primary group */

		sid_compose(&info->group_sid, &domain->sid, DOMAIN_GROUP_RID_USERS );

		/* Try to fill in what the nss_info backend can do */

		nss_get_info_cached( domain, sid, mem_ctx, NULL, NULL, 
			      &info->homedir, &info->shell, &info->full_name, 
			      &info->primary_gid );

		status = NT_STATUS_OK;
		goto done;
	}

	/* no cache...do the query */

	if ( (ads = ads_cached_connection(domain)) == NULL ) {
		domain->last_status = NT_STATUS_SERVER_DISABLED;
		goto done;
	}

	sidstr = sid_binstring(sid);
	asprintf(&ldap_exp, "(objectSid=%s)", sidstr);
	rc = ads_search_retry(ads, &msg, ldap_exp, attrs);
	free(ldap_exp);
	free(sidstr);
	if (!ADS_ERR_OK(rc) || !msg) {
		DEBUG(1,("query_user(sid=%s) ads_search: %s\n",
			 sid_string_static(sid), ads_errstr(rc)));
		goto done;
	}

	count = ads_count_replies(ads, msg);
	if (count != 1) {
		DEBUG(1,("query_user(sid=%s): Not found\n",
			 sid_string_static(sid)));
		goto done;
	}

	info->acct_name = ads_pull_username(ads, mem_ctx, msg);

	nss_get_info_cached( domain, sid, mem_ctx, ads, msg, 
		      &info->homedir, &info->shell, &info->full_name, 
		      &info->primary_gid );	

	if (info->full_name == NULL) {
		info->full_name = ads_pull_string(ads, mem_ctx, msg, "name");
	}

	if (!ads_pull_uint32(ads, msg, "primaryGroupID", &group_rid)) {
		DEBUG(1,("No primary group for %s !?\n",
			 sid_string_static(sid)));
		goto done;
	}

	sid_copy(&info->user_sid, sid);
	sid_compose(&info->group_sid, &domain->sid, group_rid);

	status = NT_STATUS_OK;

	DEBUG(3,("ads query_user gave %s\n", info->acct_name));
done:
	if (msg) 
		ads_msgfree(ads, msg);

	return status;
}

/* Lookup groups a user is a member of - alternate method, for when
   tokenGroups are not available. */
static NTSTATUS lookup_usergroups_member(struct winbindd_domain *domain,
					 TALLOC_CTX *mem_ctx,
					 const char *user_dn, 
					 DOM_SID *primary_group,
					 size_t *p_num_groups, DOM_SID **user_sids)
{
	ADS_STATUS rc;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	int count;
	LDAPMessage *res = NULL;
	LDAPMessage *msg = NULL;
	char *ldap_exp;
	ADS_STRUCT *ads;
	const char *group_attrs[] = {"objectSid", NULL};
	char *escaped_dn;
	size_t num_groups = 0;

	DEBUG(3,("ads: lookup_usergroups_member\n"));

	if ( !winbindd_can_contact_domain( domain ) ) {
		DEBUG(10,("lookup_usergroups_members: No incoming trust for domain %s\n",
			  domain->name));		
		return NT_STATUS_OK;
	}

	ads = ads_cached_connection(domain);

	if (!ads) {
		domain->last_status = NT_STATUS_SERVER_DISABLED;
		goto done;
	}

	if (!(escaped_dn = escape_ldap_string_alloc(user_dn))) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	if (!(ldap_exp = talloc_asprintf(mem_ctx, "(&(member=%s)(objectCategory=group))", escaped_dn))) {
		DEBUG(1,("lookup_usergroups(dn=%s) asprintf failed!\n", user_dn));
		SAFE_FREE(escaped_dn);
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	SAFE_FREE(escaped_dn);

	rc = ads_search_retry(ads, &res, ldap_exp, group_attrs);
	
	if (!ADS_ERR_OK(rc) || !res) {
		DEBUG(1,("lookup_usergroups ads_search member=%s: %s\n", user_dn, ads_errstr(rc)));
		return ads_ntstatus(rc);
	}
	
	count = ads_count_replies(ads, res);
	
	*user_sids = NULL;
	num_groups = 0;

	/* always add the primary group to the sid array */
	if (!add_sid_to_array(mem_ctx, primary_group, user_sids, &num_groups)) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	if (count > 0) {
		for (msg = ads_first_entry(ads, res); msg;
		     msg = ads_next_entry(ads, msg)) {
			DOM_SID group_sid;
		
			if (!ads_pull_sid(ads, msg, "objectSid", &group_sid)) {
				DEBUG(1,("No sid for this group ?!?\n"));
				continue;
			}
	
			/* ignore Builtin groups from ADS - Guenther */
			if (sid_check_is_in_builtin(&group_sid)) {
				continue;
			}
			       
			if (!add_sid_to_array(mem_ctx, &group_sid, user_sids,
					 &num_groups)) {
				status = NT_STATUS_NO_MEMORY;
				goto done;
			}
		}

	}

	*p_num_groups = num_groups;
	status = (user_sids != NULL) ? NT_STATUS_OK : NT_STATUS_NO_MEMORY;

	DEBUG(3,("ads lookup_usergroups (member) succeeded for dn=%s\n", user_dn));
done:
	if (res) 
		ads_msgfree(ads, res);

	return status;
}

/* Lookup groups a user is a member of - alternate method, for when
   tokenGroups are not available. */
static NTSTATUS lookup_usergroups_memberof(struct winbindd_domain *domain,
					   TALLOC_CTX *mem_ctx,
					   const char *user_dn, 
					   DOM_SID *primary_group,
					   size_t *p_num_groups, DOM_SID **user_sids)
{
	ADS_STATUS rc;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	ADS_STRUCT *ads;
	const char *attrs[] = {"memberOf", NULL};
	size_t num_groups = 0;
	DOM_SID *group_sids = NULL;
	int i;
	char **strings;
	size_t num_strings = 0;


	DEBUG(3,("ads: lookup_usergroups_memberof\n"));

	if ( !winbindd_can_contact_domain( domain ) ) {
		DEBUG(10,("lookup_usergroups_memberof: No incoming trust for domain %s\n",
			  domain->name));		
		return NT_STATUS_OK;
	}

	ads = ads_cached_connection(domain);

	if (!ads) {
		domain->last_status = NT_STATUS_SERVER_DISABLED;
		goto done;
	}

	rc = ads_search_retry_extended_dn_ranged(ads, mem_ctx, user_dn, attrs, 
						 ADS_EXTENDED_DN_HEX_STRING, 
						 &strings, &num_strings);

	if (!ADS_ERR_OK(rc)) {
		DEBUG(1,("lookup_usergroups_memberof ads_search member=%s: %s\n", 
			user_dn, ads_errstr(rc)));
		return ads_ntstatus(rc);
	}
	
	*user_sids = NULL;
	num_groups = 0;

	/* always add the primary group to the sid array */
	if (!add_sid_to_array(mem_ctx, primary_group, user_sids, &num_groups)) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	group_sids = TALLOC_ZERO_ARRAY(mem_ctx, DOM_SID, num_strings + 1);
	if (!group_sids) {
		TALLOC_FREE(strings);
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	for (i=0; i<num_strings; i++) {

		if (!ads_get_sid_from_extended_dn(mem_ctx, strings[i], 
						  ADS_EXTENDED_DN_HEX_STRING, 
						  &(group_sids)[i])) {
			TALLOC_FREE(group_sids);
			TALLOC_FREE(strings);
			status = NT_STATUS_NO_MEMORY;
			goto done;
		}
	}

	if (i == 0) {
		DEBUG(1,("No memberOf for this user?!?\n"));
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	for (i=0; i<num_strings; i++) {

		/* ignore Builtin groups from ADS - Guenther */
		if (sid_check_is_in_builtin(&group_sids[i])) {
			continue;
		}
		       
		if (!add_sid_to_array(mem_ctx, &group_sids[i], user_sids,
				 &num_groups)) {
			status = NT_STATUS_NO_MEMORY;
			goto done;
		}
	
	}

	*p_num_groups = num_groups;
	status = (*user_sids != NULL) ? NT_STATUS_OK : NT_STATUS_NO_MEMORY;

	DEBUG(3,("ads lookup_usergroups (memberof) succeeded for dn=%s\n", user_dn));
done:
	TALLOC_FREE(group_sids);

	return status;
}


/* Lookup groups a user is a member of. */
static NTSTATUS lookup_usergroups(struct winbindd_domain *domain,
				  TALLOC_CTX *mem_ctx,
				  const DOM_SID *sid, 
				  uint32 *p_num_groups, DOM_SID **user_sids)
{
	ADS_STRUCT *ads = NULL;
	const char *attrs[] = {"tokenGroups", "primaryGroupID", NULL};
	ADS_STATUS rc;
	int count;
	LDAPMessage *msg = NULL;
	char *user_dn = NULL;
	DOM_SID *sids;
	int i;
	DOM_SID primary_group;
	uint32 primary_group_rid;
	fstring sid_string;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	size_t num_groups = 0;

	DEBUG(3,("ads: lookup_usergroups\n"));
	*p_num_groups = 0;

	status = lookup_usergroups_cached(domain, mem_ctx, sid, 
					  p_num_groups, user_sids);
	if (NT_STATUS_IS_OK(status)) {
		return NT_STATUS_OK;
	}

	if ( !winbindd_can_contact_domain( domain ) ) {
		DEBUG(10,("lookup_usergroups: No incoming trust for domain %s\n",
			  domain->name));

		/* Tell the cache manager not to remember this one */

		return NT_STATUS_SYNCHRONIZATION_REQUIRED;
	}

	ads = ads_cached_connection(domain);
	
	if (!ads) {
		domain->last_status = NT_STATUS_SERVER_DISABLED;
		status = NT_STATUS_SERVER_DISABLED;
		goto done;
	}

	rc = ads_search_retry_sid(ads, &msg, sid, attrs);

	if (!ADS_ERR_OK(rc)) {
		status = ads_ntstatus(rc);
		DEBUG(1,("lookup_usergroups(sid=%s) ads_search tokenGroups: %s\n", 
			 sid_to_string(sid_string, sid), ads_errstr(rc)));
		goto done;
	}
	
	count = ads_count_replies(ads, msg);
	if (count != 1) {
		status = NT_STATUS_UNSUCCESSFUL;
		DEBUG(1,("lookup_usergroups(sid=%s) ads_search tokenGroups: "
			 "invalid number of results (count=%d)\n", 
			 sid_to_string(sid_string, sid), count));
		goto done;
	}

	if (!msg) {
		DEBUG(1,("lookup_usergroups(sid=%s) ads_search tokenGroups: NULL msg\n", 
			 sid_to_string(sid_string, sid)));
		status = NT_STATUS_UNSUCCESSFUL;
		goto done;
	}

	user_dn = ads_get_dn(ads, msg);
	if (user_dn == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	if (!ads_pull_uint32(ads, msg, "primaryGroupID", &primary_group_rid)) {
		DEBUG(1,("%s: No primary group for sid=%s !?\n", 
			 domain->name, sid_to_string(sid_string, sid)));
		goto done;
	}

	sid_copy(&primary_group, &domain->sid);
	sid_append_rid(&primary_group, primary_group_rid);

	count = ads_pull_sids(ads, mem_ctx, msg, "tokenGroups", &sids);

	/* there must always be at least one group in the token, 
	   unless we are talking to a buggy Win2k server */

	/* actually this only happens when the machine account has no read
	 * permissions on the tokenGroup attribute - gd */

	if (count == 0) {

		/* no tokenGroups */
		
		/* lookup what groups this user is a member of by DN search on
		 * "memberOf" */

		status = lookup_usergroups_memberof(domain, mem_ctx, user_dn,
						    &primary_group,
						    &num_groups, user_sids);
		*p_num_groups = (uint32)num_groups;
		if (NT_STATUS_IS_OK(status)) {
			goto done;
		}

		/* lookup what groups this user is a member of by DN search on
		 * "member" */

		status = lookup_usergroups_member(domain, mem_ctx, user_dn, 
						  &primary_group,
						  &num_groups, user_sids);
		*p_num_groups = (uint32)num_groups;
		goto done;
	}

	*user_sids = NULL;
	num_groups = 0;

	if (!add_sid_to_array(mem_ctx, &primary_group, user_sids, &num_groups)) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}
	
	for (i=0;i<count;i++) {

		/* ignore Builtin groups from ADS - Guenther */
		if (sid_check_is_in_builtin(&sids[i])) {
			continue;
		}
			       
		if (!add_sid_to_array_unique(mem_ctx, &sids[i],
					user_sids, &num_groups)) {
			status = NT_STATUS_NO_MEMORY;
			goto done;
		}
	}

	*p_num_groups = (uint32)num_groups;
	status = (*user_sids != NULL) ? NT_STATUS_OK : NT_STATUS_NO_MEMORY;

	DEBUG(3,("ads lookup_usergroups (tokenGroups) succeeded for sid=%s\n",
		 sid_to_string(sid_string, sid)));
done:
	ads_memfree(ads, user_dn);
	ads_msgfree(ads, msg);
	return status;
}

/*
  find the members of a group, given a group rid and domain
 */
static NTSTATUS lookup_groupmem(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				const DOM_SID *group_sid, uint32 *num_names, 
				DOM_SID **sid_mem, char ***names, 
				uint32 **name_types)
{
	ADS_STATUS rc;
	LDAPMessage *res=NULL;
	ADS_STRUCT *ads = NULL;
	char *ldap_exp;
	NTSTATUS status = NT_STATUS_UNSUCCESSFUL;
	char *sidstr;
	char **members;
	int i;
	size_t num_members;
	fstring sid_string;
	ads_control args;

	DEBUG(10,("ads: lookup_groupmem %s sid=%s\n", domain->name, 
		  sid_string_static(group_sid)));

	*num_names = 0;

	if ( !winbindd_can_contact_domain( domain ) ) {
		DEBUG(10,("lookup_groupmem: No incoming trust for domain %s\n",
			  domain->name));		
		return NT_STATUS_OK;
	}

	ads = ads_cached_connection(domain);
	
	if (!ads) {
		domain->last_status = NT_STATUS_SERVER_DISABLED;
		goto done;
	}

	if ((sidstr = sid_binstring(group_sid)) == NULL) {
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}

	/* search for all members of the group */
	if (!(ldap_exp = talloc_asprintf(mem_ctx, "(objectSid=%s)",sidstr))) {
		SAFE_FREE(sidstr);
		DEBUG(1, ("ads: lookup_groupmem: tallloc_asprintf for ldap_exp failed!\n"));
		status = NT_STATUS_NO_MEMORY;
		goto done;
	}
	SAFE_FREE(sidstr);

	members = NULL;
	num_members = 0;

	args.control = ADS_EXTENDED_DN_OID;
	args.val = ADS_EXTENDED_DN_HEX_STRING;
	args.critical = True;

	rc = ads_ranged_search(ads, mem_ctx, LDAP_SCOPE_SUBTREE, ads->config.bind_path, 
			       ldap_exp, &args, "member", &members, &num_members);

	if (!ADS_ERR_OK(rc)) {
		DEBUG(0,("ads_ranged_search failed with: %s\n", ads_errstr(rc)));
		status = NT_STATUS_UNSUCCESSFUL;
		goto done;
	} 
	
	/* now we need to turn a list of members into rids, names and name types 
	   the problem is that the members are in the form of distinguised names
	*/

	if (num_members) {
		(*sid_mem) = TALLOC_ZERO_ARRAY(mem_ctx, DOM_SID, num_members);
		(*name_types) = TALLOC_ZERO_ARRAY(mem_ctx, uint32, num_members);
		(*names) = TALLOC_ZERO_ARRAY(mem_ctx, char *, num_members);

		if ((members == NULL) || (*sid_mem == NULL) ||
		     (*name_types == NULL) || (*names == NULL)) {
			DEBUG(1, ("talloc failed\n"));
			status = NT_STATUS_NO_MEMORY;
			goto done;
		}
	} else {
		(*sid_mem) = NULL;
		(*name_types) = NULL;
		(*names) = NULL;
	}
 
	for (i=0;i<num_members;i++) {
		uint32 name_type;
		char *name, *domain_name, *dn;
		DOM_SID sid;

		if ((!ads_get_sid_from_extended_dn(mem_ctx, members[i], ADS_EXTENDED_DN_HEX_STRING, &sid)) ||
		    (!ads_get_dn_from_extended_dn(mem_ctx, members[i], &dn)))
		{
			status = NT_STATUS_INVALID_PARAMETER;
			goto done;
		}

		if (lookup_cached_sid(mem_ctx, &sid, &domain_name, &name, &name_type)) {

			DEBUG(10,("ads: lookup_groupmem: got sid %s from cache\n", 
				sid_string_static(&sid)));

			(*names)[*num_names] = CONST_DISCARD(char *,name);
			(*name_types)[*num_names] = name_type;
			sid_copy(&(*sid_mem)[*num_names], &sid);

			(*num_names)++;

			continue;
		}

		if (dn_lookup(ads, mem_ctx, dn, &name, &name_type, &sid)) {

			DEBUG(10,("ads: lookup_groupmem: got sid %s from dn_lookup\n", 
				sid_string_static(&sid)));
			
			(*names)[*num_names] = name;
			(*name_types)[*num_names] = name_type;
			sid_copy(&(*sid_mem)[*num_names], &sid);
			
			(*num_names)++;

		}
	}	

	status = NT_STATUS_OK;
	DEBUG(3,("ads lookup_groupmem for sid=%s succeeded\n", sid_to_string(sid_string, group_sid)));
done:

	if (res) 
		ads_msgfree(ads, res);

	return status;
}

/* find the sequence number for a domain */
static NTSTATUS sequence_number(struct winbindd_domain *domain, uint32 *seq)
{
	ADS_STRUCT *ads = NULL;
	ADS_STATUS rc;

	DEBUG(3,("ads: fetch sequence_number for %s\n", domain->name));

	if ( !winbindd_can_contact_domain( domain ) ) {
		DEBUG(10,("sequence: No incoming trust for domain %s\n",
			  domain->name));
		*seq = time(NULL);		
		return NT_STATUS_OK;
	}

	*seq = DOM_SEQUENCE_NONE;

	ads = ads_cached_connection(domain);
	
	if (!ads) {
		domain->last_status = NT_STATUS_SERVER_DISABLED;
		return NT_STATUS_UNSUCCESSFUL;
	}

	rc = ads_USN(ads, seq);
	
	if (!ADS_ERR_OK(rc)) {
	
		/* its a dead connection, destroy it */

		if (domain->private_data) {
			ads = (ADS_STRUCT *)domain->private_data;
			ads->is_mine = True;
			ads_destroy(&ads);
			ads_kdestroy("MEMORY:winbind_ccache");
			domain->private_data = NULL;
		}
	}
	return ads_ntstatus(rc);
}

/* get a list of trusted domains */
static NTSTATUS trusted_domains(struct winbindd_domain *domain,
				TALLOC_CTX *mem_ctx,
				uint32 *num_domains,
				char ***names,
				char ***alt_names,
				DOM_SID **dom_sids)
{
	NTSTATUS 		result = NT_STATUS_UNSUCCESSFUL;
	struct ds_domain_trust	*domains = NULL;
	int			count = 0;
	int			i;
	uint32			flags;	
	struct rpc_pipe_client *cli;
	uint32                 fr_flags = (DS_DOMAIN_IN_FOREST | DS_DOMAIN_TREE_ROOT);	
	int ret_count;
	
	DEBUG(3,("ads: trusted_domains\n"));

	*num_domains = 0;
	*alt_names   = NULL;
	*names       = NULL;
	*dom_sids    = NULL;

	/* If this is our primary domain or a root in our forest,
	   query for all trusts.  If not, then just look for domain
	   trusts in the target forest */

	if ( domain->primary ||
		((domain->domain_flags&fr_flags) == fr_flags) ) 
	{
		flags = DS_DOMAIN_DIRECT_OUTBOUND | 
			DS_DOMAIN_DIRECT_INBOUND | 
			DS_DOMAIN_IN_FOREST;
	} else {
		flags = DS_DOMAIN_IN_FOREST;
	}	

	result = cm_connect_netlogon(domain, &cli);

	if (!NT_STATUS_IS_OK(result)) {
		DEBUG(5, ("trusted_domains: Could not open a connection to %s "
			  "for PIPE_NETLOGON (%s)\n", 
			  domain->name, nt_errstr(result)));
		return NT_STATUS_UNSUCCESSFUL;
	}
	
	if ( NT_STATUS_IS_OK(result) ) {
		result = rpccli_ds_enum_domain_trusts(cli, mem_ctx,
						      cli->cli->desthost, 
						      flags, &domains,
						      (unsigned int *)&count);
	}
	
	if ( NT_STATUS_IS_OK(result) && count) {

		/* Allocate memory for trusted domain names and sids */

		if ( !(*names = TALLOC_ARRAY(mem_ctx, char *, count)) ) {
			DEBUG(0, ("trusted_domains: out of memory\n"));
			return NT_STATUS_NO_MEMORY;
		}

		if ( !(*alt_names = TALLOC_ARRAY(mem_ctx, char *, count)) ) {
			DEBUG(0, ("trusted_domains: out of memory\n"));
			return NT_STATUS_NO_MEMORY;
		}

		if ( !(*dom_sids = TALLOC_ARRAY(mem_ctx, DOM_SID, count)) ) {
			DEBUG(0, ("trusted_domains: out of memory\n"));
			return NT_STATUS_NO_MEMORY;
		}

		/* Copy across names and sids */


		ret_count = 0;		
		for (i = 0; i < count; i++) {
			struct winbindd_domain d;
			
			/* drop external trusts if this is not our primary 
			   domain.  This means that the returned number of 
			   domains may be less that the ones actually trusted
			   by the DC. */

			if ( (domains[i].trust_attributes == DS_DOMAIN_TRUST_ATTRIB_QUARANTINED_DOMAIN) && 
			     !domain->primary ) 
			{
				DEBUG(10,("trusted_domains: Skipping external trusted domain "
					  "%s because it is outside of our primary domain\n",
					  domains[i].netbios_domain));				
				continue;				
			}
			
			(*names)[ret_count] = domains[i].netbios_domain;
			(*alt_names)[ret_count] = domains[i].dns_domain;
			sid_copy(&(*dom_sids)[ret_count], &domains[i].sid);

			/* add to the trusted domain cache */

			fstrcpy( d.name,  domains[i].netbios_domain );
			fstrcpy( d.alt_name, domains[i].dns_domain );			
			sid_copy( &d.sid, &domains[i].sid );

			/* This gets a little tricky.  If we are
			   following a transitive forest trust, then
			   innerit the flags, type, and attrins from
			   the domain we queried to make sure we don't
			   record the view of the trust from the wrong
			   side.  Always view it from the side of our
			   primary domain.   --jerry */
			if ( domain->primary ||
			     ((domain->domain_flags&fr_flags) == fr_flags) ) 
			{
				DEBUG(10,("trusted_domains(ads):  Storing trust "
					  "flags for domain %s\n", d.alt_name));

				/* Look this up in cache to make sure
				   we have the current trust flags and
				   attributes */

				d.domain_flags = domains[i].flags;
				d.domain_type = domains[i].trust_type;
				d.domain_trust_attribs = domains[i].trust_attributes;
			} else {
				DEBUG(10,("trusted_domains(ads):  Inheriting trust "
					  "flags for domain %s\n", d.alt_name));				
				d.domain_flags = domain->domain_flags;				
				d.domain_type  = domain->domain_type;
				d.domain_trust_attribs = domain->domain_trust_attribs;
			}

			wcache_tdc_add_domain( &d );

			ret_count++;

		}

		*num_domains = ret_count;	
	}

	return result;
}

/* the ADS backend methods are exposed via this structure */
struct winbindd_methods ads_methods = {
	True,
	query_user_list,
	enum_dom_groups,
	enum_local_groups,
	msrpc_name_to_sid,
	msrpc_sid_to_name,
	msrpc_rids_to_names,
	query_user,
	lookup_usergroups,
	msrpc_lookup_useraliases,
	lookup_groupmem,
	sequence_number,
	msrpc_lockout_policy,
	msrpc_password_policy,
	trusted_domains,
};

#endif
