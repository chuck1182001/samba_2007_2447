/* header auto-generated by pidl */

#include <stdint.h>

#ifndef _HEADER_initshutdown
#define _HEADER_initshutdown

struct initshutdown_String_sub {
	uint32_t name_size;/* [value(strlen_m_term(name))] */
	const char * name;/* [flag(LIBNDR_FLAG_STR_LEN4|LIBNDR_FLAG_STR_NOTERM)] */
};

struct initshutdown_String {
	uint16_t name_len;/* [value(strlen_m(r->name->name)*2)] */
	uint16_t name_size;/* [value(strlen_m_term(r->name->name)*2)] */
	struct initshutdown_String_sub *name;/* [unique] */
}/* [public] */;


struct initshutdown_Init {
	struct {
		uint16_t *hostname;/* [unique] */
		struct initshutdown_String *message;/* [unique] */
		uint32_t timeout;
		uint8_t force_apps;
		uint8_t reboot;
	} in;

	struct {
		WERROR result;
	} out;

};


struct initshutdown_Abort {
	struct {
		uint16_t *server;/* [unique] */
	} in;

	struct {
		WERROR result;
	} out;

};


struct initshutdown_InitEx {
	struct {
		uint16_t *hostname;/* [unique] */
		struct initshutdown_String *message;/* [unique] */
		uint32_t timeout;
		uint8_t force_apps;
		uint8_t reboot;
		uint32_t reason;
	} in;

	struct {
		WERROR result;
	} out;

};

#endif /* _HEADER_initshutdown */
