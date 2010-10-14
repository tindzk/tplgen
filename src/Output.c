#import "Output.h"
#import <App.h>

def(void, Init, String file, bool itf) {
	this->itf = itf;

	this->className = HeapString(0);

	File_Open(&this->file, file,
		FileStatus_Create    |
		FileStatus_WriteOnly |
		FileStatus_Truncate);

	BufferedStream_Init(&this->output, &FileStream_Methods, &this->file);
	BufferedStream_SetOutputBuffer(&this->output, 4096);
}

def(void, Destroy) {
	String_Destroy(&this->className);

	BufferedStream_Close(&this->output);
	BufferedStream_Destroy(&this->output);
}

def(void, WriteString, String s) {
	BufferedStream_Write(&this->output, s.buf, s.len);
}

def(void, SetClassName, String s) {
	String_Copy(&this->className, s);
}

def(void, Write, Method_List *methods) {
	call(WriteString, String(
		"/* "
			"Warning: This file is auto-generated. "
		"*/"
		"\n"
		"\n"));

	call(WriteString, String("#import \""));
	call(WriteString, this->className);
	call(WriteString, String(".h\"\n\n"));

	/* Use a reverse loop because declaring blocks' prototypes is
	 * not compulsory. */
	DoublyLinkedList_ReverseForeach(methods, node) {
		Method *method = Method_GetPtr(node->method);

		if (method->hidden) {
			call(WriteString, String("static "));
		}

		call(WriteString, String("void "));

		if (!method->block) {
			call(WriteString, this->className);
			call(WriteString, String("_"));
		}

		call(WriteString, method->name);
		call(WriteString, String("("));

		if (method->block) {
			call(WriteString, method->params);

			if (method->params.len > 0) {
				call(WriteString, String(", "));
			}
		} else {
			call(WriteString, method->name);
			call(WriteString, String("Template *this, "));
		}

		call(WriteString, String("String *res) {\n"));

		LinkedList_Foreach(&method->lines, lineNode) {
			for (size_t i = 0; i <= lineNode->indent; i++) {
				call(WriteString, String("\t"));
			}

			call(WriteString, lineNode->line);
			call(WriteString, String("\n"));
		}

		call(WriteString, String("}\n\n"));
	}

	if (this->itf) {
		call(WriteString, String("TemplateInterface Templates_"));
		call(WriteString, this->className);
		call(WriteString, String(" = {\n"));

		LinkedList_Foreach(methods, node) {
			Method *method = Method_GetPtr(node->method);

			if (method->block) {
				continue;
			}

			String name = String_Clone(method->name);
			name.buf[0] = (char) Char_ToLower(name.buf[0]);

			call(WriteString, String("\t."));
			call(WriteString, name);
			call(WriteString, String(" = &"));
			call(WriteString, this->className);
			call(WriteString, String("_"));
			call(WriteString, method->name);
			call(WriteString, String(",\n"));

			String_Destroy(&name);
		}

		call(WriteString, String("};"));
	}

	BufferedStream_Flush(&this->output);
}
