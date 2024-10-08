/*
 * Unix SMB/CIFS implementation.
 * client auto-generated by pidl. DO NOT MODIFY!
 */

#include "includes.h"
#include "librpc/gen_ndr/cli_unixinfo.h"

NTSTATUS rpccli_unixinfo_SidToUid(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, struct dom_sid sid, uint64_t *uid)
{
	struct unixinfo_SidToUid r;
	NTSTATUS status;
	
	/* In parameters */
	r.in.sid = sid;
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(unixinfo_SidToUid, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_UNIXINFO, DCERPC_UNIXINFO_SIDTOUID, &r, (ndr_pull_flags_fn_t)ndr_pull_unixinfo_SidToUid, (ndr_push_flags_fn_t)ndr_push_unixinfo_SidToUid);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(unixinfo_SidToUid, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	*uid = *r.out.uid;
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_unixinfo_UidToSid(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, uint64_t uid, struct dom_sid *sid)
{
	struct unixinfo_UidToSid r;
	NTSTATUS status;
	
	/* In parameters */
	r.in.uid = uid;
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(unixinfo_UidToSid, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_UNIXINFO, DCERPC_UNIXINFO_UIDTOSID, &r, (ndr_pull_flags_fn_t)ndr_pull_unixinfo_UidToSid, (ndr_push_flags_fn_t)ndr_push_unixinfo_UidToSid);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(unixinfo_UidToSid, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	*sid = *r.out.sid;
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_unixinfo_SidToGid(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, struct dom_sid sid, uint64_t *gid)
{
	struct unixinfo_SidToGid r;
	NTSTATUS status;
	
	/* In parameters */
	r.in.sid = sid;
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(unixinfo_SidToGid, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_UNIXINFO, DCERPC_UNIXINFO_SIDTOGID, &r, (ndr_pull_flags_fn_t)ndr_pull_unixinfo_SidToGid, (ndr_push_flags_fn_t)ndr_push_unixinfo_SidToGid);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(unixinfo_SidToGid, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	*gid = *r.out.gid;
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_unixinfo_GidToSid(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, uint64_t gid, struct dom_sid *sid)
{
	struct unixinfo_GidToSid r;
	NTSTATUS status;
	
	/* In parameters */
	r.in.gid = gid;
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(unixinfo_GidToSid, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_UNIXINFO, DCERPC_UNIXINFO_GIDTOSID, &r, (ndr_pull_flags_fn_t)ndr_pull_unixinfo_GidToSid, (ndr_push_flags_fn_t)ndr_push_unixinfo_GidToSid);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(unixinfo_GidToSid, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	*sid = *r.out.sid;
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_unixinfo_GetPWUid(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, uint32_t *count, uint64_t *uids, struct unixinfo_GetPWUidInfo *infos)
{
	struct unixinfo_GetPWUid r;
	NTSTATUS status;
	
	/* In parameters */
	r.in.count = count;
	r.in.uids = uids;
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(unixinfo_GetPWUid, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_UNIXINFO, DCERPC_UNIXINFO_GETPWUID, &r, (ndr_pull_flags_fn_t)ndr_pull_unixinfo_GetPWUid, (ndr_push_flags_fn_t)ndr_push_unixinfo_GetPWUid);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(unixinfo_GetPWUid, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	*count = *r.out.count;
	memcpy(infos, r.out.infos, *r.in.count);
	
	/* Return result */
	return r.out.result;
}

