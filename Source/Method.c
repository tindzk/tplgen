#import "Method.h"

#define self Method

def(void, Init, String name, bool hidden) {
	LinkedList_Init(&this->lines);

	this->name   = name;
	this->params = String_New(0);
	this->hidden = hidden;
	this->block  = false;
	this->indent = 0;
}

def(void, SetBlock, bool value) {
	this->block = value;
}

def(void, SetParameters, String value) {
	String_Assign(&this->params, &value);
}

def(void, Destroy) {
	String_Destroy(&this->name);
	String_Destroy(&this->params);

	LinkedList_Destroy(&this->lines, ^(ref(LineItem) *item) {
		String_Destroy(&item->line);
	});
}

def(void, AddLine, String line) {
	ref(LineItem) *item = New(ref(LineItem));

	item->line   = line;
	item->indent = this->indent;

	LinkedList_InsertEnd(&this->lines, item);
}

inline def(void, Indent) {
	this->indent++;
}

inline def(void, Unindent) {
	if (this->indent == 0) {
		throw(InvalidDepth);
	}

	this->indent--;
}
