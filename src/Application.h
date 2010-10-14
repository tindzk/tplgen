#import <File.h>
#import <Array.h>
#import <Block.h>
#import <String.h>
#import <Logger.h>
#import <Directory.h>
#import <StreamInterface.h>

#import "Errors.h"
#import "Method.h"
#import "Output.h"
#import "Parser.h"

#undef self
#define self Application

extern size_t Modules_Application;

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
} Class(self);

void Application0(void);

def(void, Init);
def(void, Destroy);
def(bool, SetOption, String name, String value);
def(void, Process);
