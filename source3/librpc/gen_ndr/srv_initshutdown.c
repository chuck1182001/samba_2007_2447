/*
 * Unix SMB/CIFS implementation.
 * server auto-generated by pidl. DO NOT MODIFY!
 */

#include "includes.h"
#include "librpc/gen_ndr/srv_initshutdown.h"

static BOOL api_initshutdown_Init(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct initshutdown_Init r;
	TALLOC_CTX *mem_ctx = talloc_init("api_initshutdown_Init");
	
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
	status = ndr_pull_initshutdown_Init(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(initshutdown_Init, &r);
	
	r.out.result = _initshutdown_Init(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(initshutdown_Init, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_initshutdown_Init(push, NDR_OUT, &r);
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

static BOOL api_initshutdown_Abort(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct initshutdown_Abort r;
	TALLOC_CTX *mem_ctx = talloc_init("api_initshutdown_Abort");
	
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
	status = ndr_pull_initshutdown_Abort(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(initshutdown_Abort, &r);
	
	r.out.result = _initshutdown_Abort(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(initshutdown_Abort, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_initshutdown_Abort(push, NDR_OUT, &r);
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

static BOOL api_initshutdown_InitEx(pipes_struct *p)
{
	struct ndr_pull *pull;
	struct ndr_push *push;
	NTSTATUS status;
	DATA_BLOB blob;
	struct initshutdown_InitEx r;
	TALLOC_CTX *mem_ctx = talloc_init("api_initshutdown_InitEx");
	
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
	status = ndr_pull_initshutdown_InitEx(pull, NDR_IN, &r);
	if (NT_STATUS_IS_ERR(status)) {
		talloc_free(mem_ctx);
		return False;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_IN_DEBUG(initshutdown_InitEx, &r);
	
	r.out.result = _initshutdown_InitEx(p, &r);
	
	if (p->rng_fault_state) {
		talloc_free(mem_ctx);
		/* Return True here, srv_pipe_hnd.c will take care */
		return True;
	}
	
	if (DEBUGLEVEL >= 10)
		NDR_PRINT_OUT_DEBUG(initshutdown_InitEx, &r);
	
	push = ndr_push_init_ctx(mem_ctx);
	if (push == NULL) {
		talloc_free(mem_ctx);
		return False;
	}
	
	status = ndr_push_initshutdown_InitEx(push, NDR_OUT, &r);
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
static struct api_struct api_initshutdown_cmds[] = 
{
	{"INITSHUTDOWN_INIT", DCERPC_INITSHUTDOWN_INIT, api_initshutdown_Init},
	{"INITSHUTDOWN_ABORT", DCERPC_INITSHUTDOWN_ABORT, api_initshutdown_Abort},
	{"INITSHUTDOWN_INITEX", DCERPC_INITSHUTDOWN_INITEX, api_initshutdown_InitEx},
};

void initshutdown_get_pipe_fns(struct api_struct **fns, int *n_fns)
{
	*fns = api_initshutdown_cmds;
	*n_fns = sizeof(api_initshutdown_cmds) / sizeof(struct api_struct);
}

NTSTATUS rpc_initshutdown_init(void)
{
	return rpc_pipe_register_commands(SMB_RPC_INTERFACE_VERSION, "initshutdown", "initshutdown", api_initshutdown_cmds, sizeof(api_initshutdown_cmds) / sizeof(struct api_struct));
}
