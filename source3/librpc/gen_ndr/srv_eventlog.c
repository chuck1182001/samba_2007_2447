/*
 * Unix SMB/CIFS implementation.
 * server auto-generated by pidl. DO NOT MODIFY!
 */

#include "includes.h"
#include "librpc/gen_ndr/srv_eventlog.h"

static BOOL api_eventlog_ClearEventLogW(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_ClearEventLogW r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_ClearEventLogW");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_ClearEventLogW(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ClearEventLogW, &r);
	
	r.out.result = _eventlog_ClearEventLogW(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ClearEventLogW, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_ClearEventLogW(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_BackupEventLogW(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_BackupEventLogW r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_BackupEventLogW");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_BackupEventLogW(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_BackupEventLogW, &r);
	
	r.out.result = _eventlog_BackupEventLogW(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_BackupEventLogW, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_BackupEventLogW(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_CloseEventLog(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_CloseEventLog r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_CloseEventLog");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_CloseEventLog(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_CloseEventLog, &r);
	
	ZERO_STRUCT(r.out);
	r.out.handle = r.in.handle;
	r.out.result = _eventlog_CloseEventLog(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_CloseEventLog, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_CloseEventLog(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_DeregisterEventSource(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_DeregisterEventSource r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_DeregisterEventSource");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_DeregisterEventSource(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_DeregisterEventSource, &r);
	
	r.out.result = _eventlog_DeregisterEventSource(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_DeregisterEventSource, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_DeregisterEventSource(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_GetNumRecords(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_GetNumRecords r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_GetNumRecords");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_GetNumRecords(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_GetNumRecords, &r);
	
	ZERO_STRUCT(r.out);
	r.out.number = talloc_zero(mem_ctx, uint32_t);
	if (r.out.number == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	r.out.result = _eventlog_GetNumRecords(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_GetNumRecords, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_GetNumRecords(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_GetOldestRecord(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_GetOldestRecord r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_GetOldestRecord");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_GetOldestRecord(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_GetOldestRecord, &r);
	
	r.out.result = _eventlog_GetOldestRecord(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_GetOldestRecord, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_GetOldestRecord(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_ChangeNotify(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_ChangeNotify r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_ChangeNotify");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_ChangeNotify(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ChangeNotify, &r);
	
	r.out.result = _eventlog_ChangeNotify(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ChangeNotify, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_ChangeNotify(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_OpenEventLogW(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_OpenEventLogW r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_OpenEventLogW");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_OpenEventLogW(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_OpenEventLogW, &r);
	
	ZERO_STRUCT(r.out);
	r.out.handle = talloc_zero(mem_ctx, struct policy_handle);
	if (r.out.handle == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	r.out.result = _eventlog_OpenEventLogW(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_OpenEventLogW, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_OpenEventLogW(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_RegisterEventSourceW(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_RegisterEventSourceW r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_RegisterEventSourceW");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_RegisterEventSourceW(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_RegisterEventSourceW, &r);
	
	r.out.result = _eventlog_RegisterEventSourceW(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_RegisterEventSourceW, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_RegisterEventSourceW(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_OpenBackupEventLogW(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_OpenBackupEventLogW r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_OpenBackupEventLogW");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_OpenBackupEventLogW(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_OpenBackupEventLogW, &r);
	
	r.out.result = _eventlog_OpenBackupEventLogW(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_OpenBackupEventLogW, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_OpenBackupEventLogW(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_ReadEventLogW(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_ReadEventLogW r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_ReadEventLogW");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_ReadEventLogW(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ReadEventLogW, &r);
	
	ZERO_STRUCT(r.out);
	r.out.data = talloc_zero_array(mem_ctx, uint8_t, r.in.number_of_bytes);
	if (r.out.data == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	r.out.sent_size = talloc_zero(mem_ctx, uint32_t);
	if (r.out.sent_size == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	r.out.real_size = talloc_zero(mem_ctx, uint32_t);
	if (r.out.real_size == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	r.out.result = _eventlog_ReadEventLogW(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ReadEventLogW, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_ReadEventLogW(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_ReportEventW(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_ReportEventW r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_ReportEventW");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_ReportEventW(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ReportEventW, &r);
	
	r.out.result = _eventlog_ReportEventW(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ReportEventW, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_ReportEventW(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_ClearEventLogA(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_ClearEventLogA r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_ClearEventLogA");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_ClearEventLogA(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ClearEventLogA, &r);
	
	r.out.result = _eventlog_ClearEventLogA(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ClearEventLogA, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_ClearEventLogA(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_BackupEventLogA(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_BackupEventLogA r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_BackupEventLogA");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_BackupEventLogA(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_BackupEventLogA, &r);
	
	r.out.result = _eventlog_BackupEventLogA(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_BackupEventLogA, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_BackupEventLogA(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_OpenEventLogA(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_OpenEventLogA r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_OpenEventLogA");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_OpenEventLogA(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_OpenEventLogA, &r);
	
	r.out.result = _eventlog_OpenEventLogA(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_OpenEventLogA, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_OpenEventLogA(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_RegisterEventSourceA(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_RegisterEventSourceA r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_RegisterEventSourceA");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_RegisterEventSourceA(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_RegisterEventSourceA, &r);
	
	r.out.result = _eventlog_RegisterEventSourceA(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_RegisterEventSourceA, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_RegisterEventSourceA(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_OpenBackupEventLogA(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_OpenBackupEventLogA r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_OpenBackupEventLogA");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_OpenBackupEventLogA(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_OpenBackupEventLogA, &r);
	
	r.out.result = _eventlog_OpenBackupEventLogA(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_OpenBackupEventLogA, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_OpenBackupEventLogA(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_ReadEventLogA(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_ReadEventLogA r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_ReadEventLogA");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_ReadEventLogA(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ReadEventLogA, &r);
	
	r.out.result = _eventlog_ReadEventLogA(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ReadEventLogA, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_ReadEventLogA(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_ReportEventA(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_ReportEventA r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_ReportEventA");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_ReportEventA(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_ReportEventA, &r);
	
	r.out.result = _eventlog_ReportEventA(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_ReportEventA, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_ReportEventA(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_RegisterClusterSvc(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_RegisterClusterSvc r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_RegisterClusterSvc");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_RegisterClusterSvc(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_RegisterClusterSvc, &r);
	
	r.out.result = _eventlog_RegisterClusterSvc(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_RegisterClusterSvc, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_RegisterClusterSvc(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_DeregisterClusterSvc(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_DeregisterClusterSvc r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_DeregisterClusterSvc");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_DeregisterClusterSvc(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_DeregisterClusterSvc, &r);
	
	r.out.result = _eventlog_DeregisterClusterSvc(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_DeregisterClusterSvc, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_DeregisterClusterSvc(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_WriteClusterEvents(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_WriteClusterEvents r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_WriteClusterEvents");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_WriteClusterEvents(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_WriteClusterEvents, &r);
	
	r.out.result = _eventlog_WriteClusterEvents(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_WriteClusterEvents, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_WriteClusterEvents(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_GetLogIntormation(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_GetLogIntormation r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_GetLogIntormation");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_GetLogIntormation(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_GetLogIntormation, &r);
	
	r.out.result = _eventlog_GetLogIntormation(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_GetLogIntormation, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_GetLogIntormation(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}

static BOOL api_eventlog_FlushEventLog(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct eventlog_FlushEventLog r;
	TALLOC_CTX *mem_ctx = talloc_init("api_eventlog_FlushEventLog");
	
	if (!prs_data_blob(&p->in_data.data, &blob, mem_ctx)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull = ndr_pull_init_blob(&blob, mem_ctx);
	if (pull == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	pull->flags |= LIBNDR_FLAG_REF_ALLOC;
	status = ndr_pull_eventlog_FlushEventLog(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(eventlog_FlushEventLog, &r);
	
	r.out.result = _eventlog_FlushEventLog(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(eventlog_FlushEventLog, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_eventlog_FlushEventLog(push, NDR_OUT, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	blob = ndr_push_blob(push);
	if (!prs_copy_data_in(&p->out_data.rdata, (const char *)blob.data, (uint32)blob.length)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	talloc_free(mem_ctx);
	
	return True;
}


/* Tables */
static struct api_struct api_eventlog_cmds[] = 
{
	{"EVENTLOG_CLEAREVENTLOGW", DCERPC_EVENTLOG_CLEAREVENTLOGW, api_eventlog_ClearEventLogW},
	{"EVENTLOG_BACKUPEVENTLOGW", DCERPC_EVENTLOG_BACKUPEVENTLOGW, api_eventlog_BackupEventLogW},
	{"EVENTLOG_CLOSEEVENTLOG", DCERPC_EVENTLOG_CLOSEEVENTLOG, api_eventlog_CloseEventLog},
	{"EVENTLOG_DEREGISTEREVENTSOURCE", DCERPC_EVENTLOG_DEREGISTEREVENTSOURCE, api_eventlog_DeregisterEventSource},
	{"EVENTLOG_GETNUMRECORDS", DCERPC_EVENTLOG_GETNUMRECORDS, api_eventlog_GetNumRecords},
	{"EVENTLOG_GETOLDESTRECORD", DCERPC_EVENTLOG_GETOLDESTRECORD, api_eventlog_GetOldestRecord},
	{"EVENTLOG_CHANGENOTIFY", DCERPC_EVENTLOG_CHANGENOTIFY, api_eventlog_ChangeNotify},
	{"EVENTLOG_OPENEVENTLOGW", DCERPC_EVENTLOG_OPENEVENTLOGW, api_eventlog_OpenEventLogW},
	{"EVENTLOG_REGISTEREVENTSOURCEW", DCERPC_EVENTLOG_REGISTEREVENTSOURCEW, api_eventlog_RegisterEventSourceW},
	{"EVENTLOG_OPENBACKUPEVENTLOGW", DCERPC_EVENTLOG_OPENBACKUPEVENTLOGW, api_eventlog_OpenBackupEventLogW},
	{"EVENTLOG_READEVENTLOGW", DCERPC_EVENTLOG_READEVENTLOGW, api_eventlog_ReadEventLogW},
	{"EVENTLOG_REPORTEVENTW", DCERPC_EVENTLOG_REPORTEVENTW, api_eventlog_ReportEventW},
	{"EVENTLOG_CLEAREVENTLOGA", DCERPC_EVENTLOG_CLEAREVENTLOGA, api_eventlog_ClearEventLogA},
	{"EVENTLOG_BACKUPEVENTLOGA", DCERPC_EVENTLOG_BACKUPEVENTLOGA, api_eventlog_BackupEventLogA},
	{"EVENTLOG_OPENEVENTLOGA", DCERPC_EVENTLOG_OPENEVENTLOGA, api_eventlog_OpenEventLogA},
	{"EVENTLOG_REGISTEREVENTSOURCEA", DCERPC_EVENTLOG_REGISTEREVENTSOURCEA, api_eventlog_RegisterEventSourceA},
	{"EVENTLOG_OPENBACKUPEVENTLOGA", DCERPC_EVENTLOG_OPENBACKUPEVENTLOGA, api_eventlog_OpenBackupEventLogA},
	{"EVENTLOG_READEVENTLOGA", DCERPC_EVENTLOG_READEVENTLOGA, api_eventlog_ReadEventLogA},
	{"EVENTLOG_REPORTEVENTA", DCERPC_EVENTLOG_REPORTEVENTA, api_eventlog_ReportEventA},
	{"EVENTLOG_REGISTERCLUSTERSVC", DCERPC_EVENTLOG_REGISTERCLUSTERSVC, api_eventlog_RegisterClusterSvc},
	{"EVENTLOG_DEREGISTERCLUSTERSVC", DCERPC_EVENTLOG_DEREGISTERCLUSTERSVC, api_eventlog_DeregisterClusterSvc},
	{"EVENTLOG_WRITECLUSTEREVENTS", DCERPC_EVENTLOG_WRITECLUSTEREVENTS, api_eventlog_WriteClusterEvents},
	{"EVENTLOG_GETLOGINTORMATION", DCERPC_EVENTLOG_GETLOGINTORMATION, api_eventlog_GetLogIntormation},
	{"EVENTLOG_FLUSHEVENTLOG", DCERPC_EVENTLOG_FLUSHEVENTLOG, api_eventlog_FlushEventLog},
};

void eventlog_get_pipe_fns(struct api_struct **fns, int *n_fns)
{
	*fns = api_eventlog_cmds;
	*n_fns = sizeof(api_eventlog_cmds) / sizeof(struct api_struct);
}

NTSTATUS rpc_eventlog_init(void)
{
	return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION, "eventlog", "eventlog", api_eventlog_cmds, sizeof(api_eventlog_cmds) / sizeof(struct api_struct));
}
