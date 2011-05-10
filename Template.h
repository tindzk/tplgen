#import <Date.h>
#import <String.h>
#import <Macros.h>
#import <DateTime.h>
#import <Date/RFC822.h>
#import <HTML/Entities.h>

Callback(Template, void, String *);

#define tpl(name)                                                                  \
	struct simpleConcat(name, Template);                                           \
	void Template_##name(struct simpleConcat(name, Template) *tpl, String *res);   \
	static overload alwaysInline Template render(                                  \
		union { struct simpleConcat(name, Template) *addr; } transparentUnion $ptr \
	) {                                                                            \
		return (Template) {                                                        \
			.context.addr = $ptr.addr,                                             \
			.cb = (void *) Template_##name                                         \
		};                                                                         \
	}                                                                              \
	record(simpleConcat(name, Template))

static inline overload void Template_Print(Date date, String *res) {
	String s = Date_Format(date, false);
	String_Append(res, s.rd);
	String_Destroy(&s);
}

static inline overload void Template_Print(DateTime dt, String *res) {
	String s = DateTime_Format(dt);
	String_Append(res, s.rd);
	String_Destroy(&s);
}

static inline overload void Template_Print(Date_RFC822 date, String *res) {
	String s = Date_RFC822_ToString(date);
	String_Append(res, s.rd);
	String_Destroy(&s);
}

static inline overload void Template_Print(String s, String *res) {
	HTML_Entities_Encode(s.rd, res);
}

static inline overload void Template_Print(RdString s, String *res) {
	HTML_Entities_Encode(s, res);
}

static inline overload void Template_Print(s8 val, String *res) {
	String s = Int8_ToString(val);
	String_Append(res, s.rd);
	String_Destroy(&s);
}

static inline overload void Template_Print(s16 val, String *res) {
	String s = Int16_ToString(val);
	String_Append(res, s.rd);
	String_Destroy(&s);
}

static inline overload void Template_Print(s32 val, String *res) {
	String s = Int32_ToString(val);
	String_Append(res, s.rd);
	String_Destroy(&s);
}

static inline overload void Template_Print(s64 val, String *res) {
	String s = Int64_ToString(val);
	String_Append(res, s.rd);
	String_Destroy(&s);
}

static inline overload void Template_Print(u8 val, String *res) {
	String s = UInt8_ToString(val);
	String_Append(res, s.rd);
	String_Destroy(&s);
}

static inline overload void Template_Print(u16 val, String *res) {
	String s = UInt16_ToString(val);
	String_Append(res, s.rd);
	String_Destroy(&s);
}

static inline overload void Template_Print(u32 val, String *res) {
	String s = UInt32_ToString(val);
	String_Append(res, s.rd);
	String_Destroy(&s);
}

static inline overload void Template_Print(u64 val, String *res) {
	String s = UInt64_ToString(val);
	String_Append(res, s.rd);
	String_Destroy(&s);
}

#if defined(__x86_64__)
static inline overload void Template_Print(size_t val, String *res) {
	String s = UInt64_ToString(val);
	String_Append(res, s.rd);
	String_Destroy(&s);
}
#endif
