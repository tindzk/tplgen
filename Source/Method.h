#import <String.h>
#import <Exception.h>
#import <LinkedList.h>
#import <DoublyLinkedList.h>

#define self Method

// @exc InvalidDepth

record(ref(LineItem)) {
	String line;
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

ExtendClass;

record(ref(Item)) {
	MethodInstance method;
	DoublyLinkedList_DeclareRef(ref(Item));
};

DoublyLinkedList_DeclareList(ref(Item), ref(List));

def(void, Init, String name, bool hidden);
def(void, SetBlock, bool value);
def(void, SetParameters, String value);
def(void, Destroy);
def(void, AddLine, String line);
def(void, Indent);
def(void, Unindent);

#undef self
