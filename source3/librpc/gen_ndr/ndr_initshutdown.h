/* header auto-generated by pidl */

#include "librpc/gen_ndr/initshutdown.h"

#ifndef _HEADER_NDR_initshutdown
#define _HEADER_NDR_initshutdown

#include "librpc/ndr/libndr.h"
#define DCERPC_INITSHUTDOWN_UUID "894de0c0-0d55-11d3-a322-00c04fa321a1"
#define DCERPC_INITSHUTDOWN_VERSION 1.0
#define DCERPC_INITSHUTDOWN_NAME "initshutdown"
#define DCERPC_INITSHUTDOWN_HELPSTRING "Init shutdown service"
extern const struct dcerpc_interface_table dcerpc_table_initshutdown;
NTSTATUS dcerpc_server_initshutdown_init(void);
#define DCERPC_INITSHUTDOWN_INIT (0x00)

#define DCERPC_INITSHUTDOWN_ABORT (0x01)

#define DCERPC_INITSHUTDOWN_INITEX (0x02)

#define DCERPC_INITSHUTDOWN_CALL_COUNT (3)
void ndr_print_initshutdown_String_sub(struct ndr_print *ndr, const char *name, const struct initshutdown_String_sub *r);
NTSTATUS ndr_push_initshutdown_String(struct ndr_push *ndr, int ndr_flags, const struct initshutdown_String *r);
NTSTATUS ndr_pull_initshutdown_String(struct ndr_pull *ndr, int ndr_flags, struct initshutdown_String *r);
void ndr_print_initshutdown_String(struct ndr_print *ndr, const char *name, const struct initshutdown_String *r);
void ndr_print_initshutdown_Init(struct ndr_print *ndr, const char *name, int flags, const struct initshutdown_Init *r);
void ndr_print_initshutdown_Abort(struct ndr_print *ndr, const char *name, int flags, const struct initshutdown_Abort *r);
void ndr_print_initshutdown_InitEx(struct ndr_print *ndr, const char *name, int flags, const struct initshutdown_InitEx *r);
#endif /* _HEADER_NDR_initshutdown */
