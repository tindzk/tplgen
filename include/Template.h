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

// ---

static inline overload void Template_Print(Date date, String *res) {
	Template_PrintDate(date, res);
}

static inline overload void Template_Print(String s, String *res) {
	Template_PrintHtml(s, res);
}

static inline overload void Template_Print(int val, String *res) {
	String_Append(res, Integer_ToString(val));
}
