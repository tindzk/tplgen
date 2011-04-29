#import "Method.h"

#define self Method

rsdef(self, New, String name, bool hidden) {
	return (self) {
		.name   = name,
		.params = String_New(0),
		.hidden = hidden,
		.block  = false,
		.indent = 0,
		.lines  = LinkedList_New()
	};
}

def(void, SetBlock, bool value) {
	this->block = value;
}

def(void, SetParameters, String value) {
	String_Assign(&this->params, value);
}

def(void, DestroyLineItem, ref(LineItem) *item) {
	CarrierString_Destroy(&item->line);
}

def(void, Destroy) {
	String_Destroy(&this->name);
	String_Destroy(&this->params);

	LinkedList_Destroy(&this->lines,
		LinkedList_OnDestroy_For(this, ref(DestroyLineItem)));
}

overload def(void, AddLine, String line) {
	ref(LineItem) *item = scall(LineItem_Alloc);

	item->line   = String_ToCarrier(line);
	item->indent = this->indent;

	LinkedList_Push(&this->lines, item);
}

overload def(void, AddLine, OmniString line) {
	ref(LineItem) *item = scall(LineItem_Alloc);

	item->line   = String_ToCarrier(line);
	item->indent = this->indent;

	LinkedList_Push(&this->lines, item);
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
