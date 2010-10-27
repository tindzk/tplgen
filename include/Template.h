#import <String.h>
#import <Macros.h>

DefineCallback(Template, void, String *);

#define tpl(name) \
	class(simpleConcat(name, Template))

#define Template(meth, obj)       \
	(Template) Callback(          \
		Generic_FromObject(&obj), \
		meth)
