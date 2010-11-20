#import <Date.h>
#import <String.h>
#import <Macros.h>
#import <HTML/Entities.h>

DefineCallback(Template, void, String *);

#define tpl(name) \
	class(simpleConcat(name, Template))

#define Template(meth, obj)       \
	(Template) Callback(          \
		Generic_FromObject(&obj), \
		meth)

static inline void Template_PrintDate(Date date, String *res) {
	String s = Date_Format(date);
	String_Append(res, s);
	String_Destroy(&s);
}

static inline void Template_PrintHtml(String s, String *res) {
	String enc = HTML_Entities_Encode(s);
	String_Append(res, enc);
	String_Destroy(&enc);
}

static inline void Template_PrintLiteral(String s, String *res) {
	String_Append(res, s);
}

static inline overload void Template_Print(Date date, String *res) {
	Template_PrintDate(date, res);
}

static inline overload void Template_Print(String s, String *res) {
	Template_PrintHtml(s, res);
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
