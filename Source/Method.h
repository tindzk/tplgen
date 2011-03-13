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

MemoryHelpers(ref(LineItem));

LinkedList_DeclareList(ref(LineItem), ref(LineList));

class {
	bool block;
	bool hidden;

	String name;
	String params;

	size_t indent;

	ref(LineList) lines;
};

MemoryHelpers(self);

record(ref(Item)) {
	Method *method;
	DoublyLinkedList_DeclareRef(ref(Item));
};

MemoryHelpers(ref(Item));

DoublyLinkedList_DeclareList(ref(Item), ref(List));

def(void, Init, String name, bool hidden);
def(void, SetBlock, bool value);
def(void, SetParameters, String value);
def(void, Destroy);
overload def(void, AddLine, String line);
overload def(void, AddLine, OmniString line);
def(void, Indent);
def(void, Unindent);

#undef self
