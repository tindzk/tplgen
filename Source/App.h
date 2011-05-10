#import <File.h>
#import <Array.h>
#import <Block.h>
#import <Logger.h>
#import <Stream.h>
#import <String.h>
#import <Directory.h>

#import "Method.h"
#import "Output.h"
#import "Parser.h"

#define self App

// @exc InvalidParameter
// @exc ParsingFailed

record(ref(TemplateItem)) {
	String name;
	String file;
};

Array(ref(TemplateItem), ref(TemplateArray));

class {
	bool itf;
	String dir;
	String ext;
	String out;
	String name;

	Logger *logger;
	Method_List methods;

	ref(TemplateArray) *files;
};

rsdef(self, New, Logger *logger);
odef(void, destroy);
odef(bool, setOption, RdString name, RdString value);
odef(void, process);

#undef self
