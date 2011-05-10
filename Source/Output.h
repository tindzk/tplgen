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

	Logger *logger;

	File srcFile;
	BufferedStream src;

	File hdrFile;
	BufferedStream hdr;
};

odef(void, init, Logger *logger, RdString file, bool itf);
odef(void, destroy);
odef(void, setClassName, String s);
odef(void, writeList, Method_List *methods);

#undef self
