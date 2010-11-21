#import <File.h>
#import <String.h>
#import <FileStream.h>
#import <LinkedList.h>
#import <BufferedStream.h>

#import "Method.h"

#undef self
#define self Output

#define	Output_Warning String(                   \
	"/* "                                        \
		"Warning: This file is auto-generated. " \
	"*/"                                         \
	"\n"                                         \
	"\n")

class(self) {
	bool itf;
	String className;

	File srcFile;
	BufferedStream src;

	File hdrFile;
	BufferedStream hdr;
};

ExtendClass(self);

def(void, Init, String file, bool itf);
def(void, Destroy);
def(void, SetClassName, String s);
def(void, Write, Method_List *methods);
