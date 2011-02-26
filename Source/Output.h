#import <File.h>
#import <String.h>
#import <Logger.h>
#import <FileStream.h>
#import <LinkedList.h>
#import <BufferedStream.h>

#import "Method.h"

#define self Output

#define	Output_Warning $(                        \
	"/* "                                        \
		"Warning: This file is auto-generated. " \
	"*/"                                         \
	"\n"                                         \
	"\n")

class {
	bool itf;
	String className;

	File srcFile;
	BufferedStream src;

	File hdrFile;
	BufferedStream hdr;
};

def(void, Init, ProtString file, bool itf);
def(void, Destroy);
def(void, SetClassName, String s);
def(void, Write, Method_List *methods);

#undef self
