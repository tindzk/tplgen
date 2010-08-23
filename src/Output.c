#import "Output.h"

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
	ref(WriteString)(this, String(
		"/* "
			"Warning: This file is auto-generated. "
		"*/"
		"\n"
		"\n"));

	ref(WriteString)(this, String("#import \""));
	ref(WriteString)(this, this->className);
	ref(WriteString)(this, String(".h\"\n\n"));

	/* Use a reverse loop because declaring blocks' prototypes is
	 * not compulsory. */
	DoublyLinkedList_ReverseForeach(methods, node) {
		Method *method = node->method;

		if (method->hidden) {
			ref(WriteString)(this, String("static "));
		}

		ref(WriteString)(this, String("void "));

		if (!method->block) {
			ref(WriteString)(this, this->className);
			ref(WriteString)(this, String("_"));
		}

		ref(WriteString)(this, method->name);
		ref(WriteString)(this, String("("));

		if (method->block) {
			ref(WriteString)(this, method->params);

			if (method->params.len > 0) {
				ref(WriteString)(this, String(", "));
			}
		} else {
			ref(WriteString)(this, method->name);
			ref(WriteString)(this, String("Template *this, "));
		}

		ref(WriteString)(this, String("String *res) {\n"));

		LinkedList_Foreach(&method->lines, lineNode) {
			for (size_t i = 0; i <= lineNode->indent; i++) {
				ref(WriteString)(this, String("\t"));
			}

			ref(WriteString)(this, lineNode->line);
			ref(WriteString)(this, String("\n"));
		}

		ref(WriteString)(this, String("}\n\n"));
	}

	if (this->itf) {
		ref(WriteString)(this, String("TemplateInterface Templates_"));
		ref(WriteString)(this, this->className);
		ref(WriteString)(this, String(" = {\n"));

		LinkedList_Foreach(methods, node) {
			Method *method = node->method;

			if (method->block) {
				continue;
			}

			String name = String_Clone(method->name);
			name.buf[0] = (char) Char_ToLower(name.buf[0]);

			ref(WriteString)(this, String("\t."));
			ref(WriteString)(this, name);
			ref(WriteString)(this, String(" = &"));
			ref(WriteString)(this, this->className);
			ref(WriteString)(this, String("_"));
			ref(WriteString)(this, method->name);
			ref(WriteString)(this, String(",\n"));

			String_Destroy(&name);
		}

		ref(WriteString)(this, String("};"));
	}

	BufferedStream_Flush(&this->output);
}
