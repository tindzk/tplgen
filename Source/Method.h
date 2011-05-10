#import <String.h>
#import <Exception.h>
#import <LinkedList.h>
#import <DoublyLinkedList.h>

#define self Method

// @exc InvalidDepth

record(ref(LineItem)) {
	CarrierString line;
	size_t indent;
	LinkedList_DeclareRef(ref(LineItem));
};

LinkedList_DeclareList(ref(LineItem), ref(LineList));

class {
	bool block;
	bool hidden;

	String name;
	String params;

	size_t indent;

	ref(LineList) lines;
};

record(ref(Item)) {
	Method *method;
	DoublyLinkedList_DeclareRef(ref(Item));
};

static inline ref(Item)* ref(Item_New)(self *method) {
	ref(Item) *res = Memory_New(sizeof(ref(Item)));
	res->method = method;
	return res;
}

static inline ref(LineItem)* ref(LineItem_New)(CarrierString line, size_t indent) {
	ref(LineItem) *res = Memory_New(sizeof(ref(LineItem)));

	res->line   = line;
	res->indent = indent;

	return res;
}

DoublyLinkedList_DeclareList(ref(Item), ref(List));

rsdef(self *, New, String name, bool hidden);
def(void, Destroy);
def(void, SetBlock, bool value);
def(void, SetParameters, String value);
overload def(void, AddLine, String line);
overload def(void, AddLine, OmniString line);
def(void, Indent);
def(void, Unindent);

#undef self
