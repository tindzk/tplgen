#import <File.h>
#import <Array.h>
#import <Block.h>
#import <String.h>
#import <Logger.h>
#import <Directory.h>
#import <StreamInterface.h>

#import "Method.h"
#import "Output.h"

#undef self
#define self Application

Exception_Export(excParsingFailed);
Exception_Export(excInvalidParameter);

typedef struct {
	String name;
	String file;
} ref(TemplateItem);

typedef struct {
	bool itf;
	String dir;
	String ext;
	String out;
	String name;

	Method_List methods;

	Array(ref(TemplateItem), *files);
} self;

def(void, Init);
def(void, Destroy);
def(bool, SetOption, String name, String value);
def(void, Process);
