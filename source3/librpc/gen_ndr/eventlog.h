/* header auto-generated by pidl */

#include <stdint.h>

#include "librpc/gen_ndr/lsa.h"
#include "librpc/gen_ndr/security.h"
#ifndef _HEADER_eventlog
#define _HEADER_eventlog

/* bitmap eventlogReadFlags */
#define EVENTLOG_SEQUENTIAL_READ ( 0x0001 )
#define EVENTLOG_SEEK_READ ( 0x0002 )
#define EVENTLOG_FORWARDS_READ ( 0x0004 )
#define EVENTLOG_BACKWARDS_READ ( 0x0008 )

;

/* bitmap eventlogEventTypes */
#define EVENTLOG_SUCCESS ( 0x0000 )
#define EVENTLOG_ERROR_TYPE ( 0x0001 )
#define EVENTLOG_WARNING_TYPE ( 0x0002 )
#define EVENTLOG_INFORMATION_TYPE ( 0x0004 )
#define EVENTLOG_AUDIT_SUCCESS ( 0x0008 )
#define EVENTLOG_AUDIT_FAILURE ( 0x0010 )

;

struct eventlog_OpenUnknown0 {
	uint16_t unknown0;
	uint16_t unknown1;
};

struct eventlog_Record {
	uint32_t size;
	uint32_t reserved;
	uint32_t record_number;
	uint32_t time_generated;
	uint32_t time_written;
	uint32_t event_id;
	uint16_t event_type;
	uint16_t num_of_strings;
	uint16_t event_category;
	uint16_t reserved_flags;
	uint32_t closing_record_number;
	uint32_t stringoffset;
	uint32_t sid_length;
	uint32_t sid_offset;
	uint32_t data_length;
	uint32_t data_offset;
	const char * source_name;/* [flag(LIBNDR_FLAG_STR_NULLTERM)] */
	const char * computer_name;/* [flag(LIBNDR_FLAG_STR_NULLTERM)] */
	const char * *strings;/* [flag(LIBNDR_FLAG_STR_NULLTERM)] */
	const char * raw_data;/* [flag(LIBNDR_FLAG_STR_ASCII|LIBNDR_FLAG_STR_NULLTERM)] */
}/* [public] */;


struct eventlog_ClearEventLogW {
	struct {
		struct policy_handle *handle;/* [ref] */
		struct lsa_String *unknown;/* [unique] */
	} in;

	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_BackupEventLogW {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_CloseEventLog {
	struct {
		struct policy_handle *handle;/* [ref] */
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		NTSTATUS result;
	} out;

};


struct eventlog_DeregisterEventSource {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_GetNumRecords {
	struct {
		struct policy_handle *handle;/* [ref] */
	} in;

	struct {
		uint32_t *number;/* [ref] */
		NTSTATUS result;
	} out;

};


struct eventlog_GetOldestRecord {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_ChangeNotify {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_OpenEventLogW {
	struct {
		struct eventlog_OpenUnknown0 *unknown0;/* [unique] */
		struct lsa_String logname;
		struct lsa_String servername;
		uint32_t unknown2;
		uint32_t unknown3;
	} in;

	struct {
		struct policy_handle *handle;/* [ref] */
		NTSTATUS result;
	} out;

};


struct eventlog_RegisterEventSourceW {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_OpenBackupEventLogW {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_ReadEventLogW {
	struct {
		struct policy_handle *handle;/* [ref] */
		uint32_t flags;
		uint32_t offset;
		uint32_t number_of_bytes;
	} in;

	struct {
		uint8_t *data;/* [ref,size_is(number_of_bytes)] */
		uint32_t *sent_size;/* [ref] */
		uint32_t *real_size;/* [ref] */
		NTSTATUS result;
	} out;

};


struct eventlog_ReportEventW {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_ClearEventLogA {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_BackupEventLogA {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_OpenEventLogA {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_RegisterEventSourceA {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_OpenBackupEventLogA {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_ReadEventLogA {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_ReportEventA {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_RegisterClusterSvc {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_DeregisterClusterSvc {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_WriteClusterEvents {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_GetLogIntormation {
	struct {
		NTSTATUS result;
	} out;

};


struct eventlog_FlushEventLog {
	struct {
		struct policy_handle *handle;/* [ref] */
	} in;

	struct {
		NTSTATUS result;
	} out;

};

#endif /* _HEADER_eventlog */
