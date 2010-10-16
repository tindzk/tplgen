#import <File.h>
#import <String.h>
#import <FileStream.h>
#import <LinkedList.h>
#import <BufferedStream.h>

#import "Method.h"

#undef self
#define self Output

record {
	bool itf;

	File file;
	BufferedStream output;

	String className;
} Class(self);

def(void, Init, String file, bool itf);
def(void, Destroy);
def(void, WriteString, String s);
def(void, SetClassName, String s);
def(void, Write, Method_List *methods);
