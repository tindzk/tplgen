#import <Date.h>
#import <String.h>
#import <Macros.h>
#import <DateTime.h>
#import <Date/RFC822.h>
#import <HTML/Entities.h>

DefineCallback(Template, void, String *);

#define tpl(name)                                                                \
	struct simpleConcat(name, Template);                                         \
	void Template_##name(struct simpleConcat(name, Template) *tpl, String *res); \
	static inline Template tpl##name(                                            \
		union { struct simpleConcat(name, Template) *addr } transparentUnion $ptr\
	) {                                                                          \
		return (Template) Callback(                                              \
			Generic_FromObject($ptr.addr),                                       \
			Template_##name);                                                    \
	}                                                                            \
	record(simpleConcat(name, Template))

static inline overload void Template_Print(Date date, String *res) {
	String s = Date_Format(date, false);
	String_Append(res, s);
	String_Destroy(&s);
}

static inline overload void Template_Print(DateTime dt, String *res) {
	String s = DateTime_Format(dt);
	String_Append(res, s);
	String_Destroy(&s);
}

static inline overload void Template_Print(Date_RFC822 date, String *res) {
	String s = Date_RFC822_ToString(date);
	String_Append(res, s);
	String_Destroy(&s);
}

static inline overload void Template_Print(String s, String *res) {
	String enc = HTML_Entities_Encode(s);
	String_Append(res, enc);
	String_Destroy(&enc);
}

static inline overload void Template_Print(s8 val, String *res) {
	String_Append(res, Int8_ToString(val));
}

static inline overload void Template_Print(s16 val, String *res) {
	String_Append(res, Int16_ToString(val));
}

static inline overload void Template_Print(s32 val, String *res) {
	String_Append(res, Int32_ToString(val));
}

static inline overload void Template_Print(s64 val, String *res) {
	String_Append(res, Int64_ToString(val));
}

static inline overload void Template_Print(u8 val, String *res) {
	String_Append(res, UInt8_ToString(val));
}

static inline overload void Template_Print(u16 val, String *res) {
	String_Append(res, UInt16_ToString(val));
}

static inline overload void Template_Print(u32 val, String *res) {
	String_Append(res, UInt32_ToString(val));
}

static inline overload void Template_Print(u64 val, String *res) {
	String_Append(res, UInt64_ToString(val));
}

#if defined(__x86_64__)
static inline overload void Template_Print(size_t val, String *res) {
	String_Append(res, UInt64_ToString(val));
}
#endif
