/*
 * Unix SMB/CIFS implementation.
 * client auto-generated by pidl. DO NOT MODIFY!
 */

#include "includes.h"
#include "librpc/gen_ndr/cli_eventlog.h"

NTSTATUS rpccli_eventlog_ClearEventLogW(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, struct policy_handle *handle, struct lsa_String *unknown)
{
	struct eventlog_ClearEventLogW r;
	NTSTATUS status;
	
	/* In parameters */
	r.in.handle = handle;
	r.in.unknown = unknown;
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ClearEventLogW, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_CLEAREVENTLOGW, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_ClearEventLogW, (ndr_push_flags_fn_t)ndr_push_eventlog_ClearEventLogW);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ClearEventLogW, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_BackupEventLogW(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_BackupEventLogW r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_BackupEventLogW, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_BACKUPEVENTLOGW, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_BackupEventLogW, (ndr_push_flags_fn_t)ndr_push_eventlog_BackupEventLogW);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_BackupEventLogW, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_CloseEventLog(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, struct policy_handle *handle)
{
	struct eventlog_CloseEventLog r;
	NTSTATUS status;
	
	/* In parameters */
	r.in.handle = handle;
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_CloseEventLog, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_CLOSEEVENTLOG, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_CloseEventLog, (ndr_push_flags_fn_t)ndr_push_eventlog_CloseEventLog);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_CloseEventLog, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	*handle = *r.out.handle;
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_DeregisterEventSource(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_DeregisterEventSource r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_DeregisterEventSource, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_DEREGISTEREVENTSOURCE, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_DeregisterEventSource, (ndr_push_flags_fn_t)ndr_push_eventlog_DeregisterEventSource);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_DeregisterEventSource, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_GetNumRecords(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, struct policy_handle *handle, uint32_t *number)
{
	struct eventlog_GetNumRecords r;
	NTSTATUS status;
	
	/* In parameters */
	r.in.handle = handle;
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_GetNumRecords, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_GETNUMRECORDS, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_GetNumRecords, (ndr_push_flags_fn_t)ndr_push_eventlog_GetNumRecords);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_GetNumRecords, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	*number = *r.out.number;
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_GetOldestRecord(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_GetOldestRecord r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_GetOldestRecord, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_GETOLDESTRECORD, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_GetOldestRecord, (ndr_push_flags_fn_t)ndr_push_eventlog_GetOldestRecord);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_GetOldestRecord, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_ChangeNotify(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_ChangeNotify r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ChangeNotify, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_CHANGENOTIFY, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_ChangeNotify, (ndr_push_flags_fn_t)ndr_push_eventlog_ChangeNotify);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ChangeNotify, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_OpenEventLogW(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, struct eventlog_OpenUnknown0 *unknown0, struct lsa_String logname, struct lsa_String servername, uint32_t unknown2, uint32_t unknown3, struct policy_handle *handle)
{
	struct eventlog_OpenEventLogW r;
	NTSTATUS status;
	
	/* In parameters */
	r.in.unknown0 = unknown0;
	r.in.logname = logname;
	r.in.servername = servername;
	r.in.unknown2 = unknown2;
	r.in.unknown3 = unknown3;
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_OpenEventLogW, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_OPENEVENTLOGW, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_OpenEventLogW, (ndr_push_flags_fn_t)ndr_push_eventlog_OpenEventLogW);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_OpenEventLogW, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	*handle = *r.out.handle;
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_RegisterEventSourceW(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_RegisterEventSourceW r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_RegisterEventSourceW, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_REGISTEREVENTSOURCEW, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_RegisterEventSourceW, (ndr_push_flags_fn_t)ndr_push_eventlog_RegisterEventSourceW);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_RegisterEventSourceW, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_OpenBackupEventLogW(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_OpenBackupEventLogW r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_OpenBackupEventLogW, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_OPENBACKUPEVENTLOGW, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_OpenBackupEventLogW, (ndr_push_flags_fn_t)ndr_push_eventlog_OpenBackupEventLogW);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_OpenBackupEventLogW, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_ReadEventLogW(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, struct policy_handle *handle, uint32_t flags, uint32_t offset, uint32_t number_of_bytes, uint8_t *data, uint32_t *sent_size, uint32_t *real_size)
{
	struct eventlog_ReadEventLogW r;
	NTSTATUS status;
	
	/* In parameters */
	r.in.handle = handle;
	r.in.flags = flags;
	r.in.offset = offset;
	r.in.number_of_bytes = number_of_bytes;
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ReadEventLogW, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_READEVENTLOGW, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_ReadEventLogW, (ndr_push_flags_fn_t)ndr_push_eventlog_ReadEventLogW);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ReadEventLogW, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	*data = *r.out.data;
	*sent_size = *r.out.sent_size;
	*real_size = *r.out.real_size;
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_ReportEventW(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_ReportEventW r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ReportEventW, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_REPORTEVENTW, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_ReportEventW, (ndr_push_flags_fn_t)ndr_push_eventlog_ReportEventW);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ReportEventW, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_ClearEventLogA(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_ClearEventLogA r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ClearEventLogA, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_CLEAREVENTLOGA, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_ClearEventLogA, (ndr_push_flags_fn_t)ndr_push_eventlog_ClearEventLogA);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ClearEventLogA, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_BackupEventLogA(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_BackupEventLogA r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_BackupEventLogA, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_BACKUPEVENTLOGA, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_BackupEventLogA, (ndr_push_flags_fn_t)ndr_push_eventlog_BackupEventLogA);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_BackupEventLogA, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_OpenEventLogA(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_OpenEventLogA r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_OpenEventLogA, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_OPENEVENTLOGA, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_OpenEventLogA, (ndr_push_flags_fn_t)ndr_push_eventlog_OpenEventLogA);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_OpenEventLogA, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_RegisterEventSourceA(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_RegisterEventSourceA r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_RegisterEventSourceA, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_REGISTEREVENTSOURCEA, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_RegisterEventSourceA, (ndr_push_flags_fn_t)ndr_push_eventlog_RegisterEventSourceA);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_RegisterEventSourceA, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_OpenBackupEventLogA(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_OpenBackupEventLogA r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_OpenBackupEventLogA, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_OPENBACKUPEVENTLOGA, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_OpenBackupEventLogA, (ndr_push_flags_fn_t)ndr_push_eventlog_OpenBackupEventLogA);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_OpenBackupEventLogA, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_ReadEventLogA(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_ReadEventLogA r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ReadEventLogA, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_READEVENTLOGA, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_ReadEventLogA, (ndr_push_flags_fn_t)ndr_push_eventlog_ReadEventLogA);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ReadEventLogA, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_ReportEventA(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_ReportEventA r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ReportEventA, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_REPORTEVENTA, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_ReportEventA, (ndr_push_flags_fn_t)ndr_push_eventlog_ReportEventA);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ReportEventA, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_RegisterClusterSvc(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_RegisterClusterSvc r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_RegisterClusterSvc, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_REGISTERCLUSTERSVC, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_RegisterClusterSvc, (ndr_push_flags_fn_t)ndr_push_eventlog_RegisterClusterSvc);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_RegisterClusterSvc, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_DeregisterClusterSvc(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_DeregisterClusterSvc r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_DeregisterClusterSvc, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_DEREGISTERCLUSTERSVC, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_DeregisterClusterSvc, (ndr_push_flags_fn_t)ndr_push_eventlog_DeregisterClusterSvc);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_DeregisterClusterSvc, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_WriteClusterEvents(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_WriteClusterEvents r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_WriteClusterEvents, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_WRITECLUSTEREVENTS, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_WriteClusterEvents, (ndr_push_flags_fn_t)ndr_push_eventlog_WriteClusterEvents);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_WriteClusterEvents, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_GetLogIntormation(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx)
{
	struct eventlog_GetLogIntormation r;
	NTSTATUS status;
	
	/* In parameters */
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_GetLogIntormation, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_GETLOGINTORMATION, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_GetLogIntormation, (ndr_push_flags_fn_t)ndr_push_eventlog_GetLogIntormation);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_GetLogIntormation, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

NTSTATUS rpccli_eventlog_FlushEventLog(struct rpc_pipe_client *cli, TALLOC_CTX *mem_ctx, struct policy_handle *handle)
{
	struct eventlog_FlushEventLog r;
	NTSTATUS status;
	
	/* In parameters */
	r.in.handle = handle;
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_FlushEventLog, &r);
	
	status = cli_do_rpc_ndr(cli, mem_ctx, PI_EVENTLOG, DCERPC_EVENTLOG_FLUSHEVENTLOG, &r, (ndr_pull_flags_fn_t)ndr_pull_eventlog_FlushEventLog, (ndr_push_flags_fn_t)ndr_push_eventlog_FlushEventLog);
	
	if (!NT_STATUS_IS_OK(status)) {
		return status;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_FlushEventLog, &r);
	
	if (NT_STATUS_IS_ERR(status)) {
		return status;
	}
	
	/* Return variables */
	
	/* Return result */
	return r.out.result;
}

