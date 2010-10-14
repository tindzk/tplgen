#import <String.h>
#import <Exception.h>
#import <LinkedList.h>
#import <DoublyLinkedList.h>

#import "Errors.h"

#undef self
#define self Method

extern size_t Modules_Method;

typedef struct ref(LineItem) {
	String line;
	size_t indent;
	LinkedList_DeclareRef(ref(LineItem));
} ref(LineItem);

LinkedList_DeclareList(ref(LineItem), ref(LineList));

typedef struct {
	bool block;
	bool hidden;

	String name;
	String params;

	size_t indent;

	ref(LineList) lines;
} Class(self);

typedef struct ref(Item) {
	MethodClass method;
	DoublyLinkedList_DeclareRef(ref(Item));
} ref(Item);

DoublyLinkedList_DeclareList(ref(Item), ref(List));

def(void, Init, String name, bool hidden);
def(void, SetBlock, bool value);
def(void, SetParameters, String value);
def(void, Destroy);
def(void, AddLine, String line);
def(void, Indent);
def(void, Unindent);
