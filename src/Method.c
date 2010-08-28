#import "Method.h"

extern ExceptionManager exc;

size_t Modules_Method;

void Method0(void) {
	Modules_Method = Module_Register(String("Method"));
}

def(void, Init, String name, bool hidden) {
	LinkedList_Init(&this->lines);

	this->name   = String_Clone(name);
	this->params = HeapString(0);
	this->hidden = hidden;
	this->block  = false;
	this->indent = 0;
}

def(void, SetBlock, bool value) {
	this->block = value;
}

def(void, SetParameters, String value) {
	this->params = String_Clone(value);
}

def(void, Destroy) {
	String_Destroy(&this->name);
	String_Destroy(&this->params);

	LinkedList_Destroy(&this->lines, ^(ref(LineItem) *item) {
		String_Destroy(&item->line);
		Memory_Free(item);
	});
}

def(void, AddLine, String line) {
	ref(LineItem) *item = New(ref(LineItem));

	item->line   = String_Clone(line);
	item->indent = this->indent;

	LinkedList_InsertEnd(&this->lines, item);
}

inline def(void, Indent) {
	this->indent++;
}

inline def(void, Unindent) {
	if (this->indent == 0) {
		throw(&exc, excInvalidDepth);
	}

	this->indent--;
}
