#import "Output.h"

#define self Output

static def(void, Open, String path, File *file, BufferedStream *stream) {
	File_Open(file, path,
		FileStatus_Create    |
		FileStatus_Truncate  |
		FileStatus_WriteOnly);

	BufferedStream_Init(stream, &FileStreamImpl, file);
	BufferedStream_SetOutputBuffer(stream, 4096);
}

def(void, Init, String file, bool itf) {
	this->itf = itf;
	this->className = HeapString(0);

	String src = String_Concat(file, String(".c"));
	String hdr = String_Concat(file, String(".h"));

	call(Open, src, &this->srcFile, &this->src);
	call(Open, hdr, &this->hdrFile, &this->hdr);

	String_Destroy(&hdr);
	String_Destroy(&src);
}

def(void, Destroy) {
	String_Destroy(&this->className);

	BufferedStream_Close(&this->src);
	BufferedStream_Close(&this->hdr);

	BufferedStream_Destroy(&this->src);
	BufferedStream_Destroy(&this->hdr);
}

static def(void, WriteSourceString, String s) {
	BufferedStream_Write(&this->src, s.buf, s.len);
}

static def(void, WriteHeaderString, String s) {
	BufferedStream_Write(&this->hdr, s.buf, s.len);
}

def(void, SetClassName, String s) {
	String_Copy(&this->className, s);
}

static def(void, WriteDeclaration, Method *method, bool src) {
	String res = HeapString(0);

	if (method->hidden) {
		String_Append(&res, String("static "));
	}

	String_Append(&res, String("void "));

	if (!method->block) {
		String_Append(&res, this->className);
		String_Append(&res, String("_"));
	}

	String_Append(&res, method->name);
	String_Append(&res, String("("));

	if (method->block) {
		String_Append(&res, method->params);

		if (method->params.len > 0) {
			String_Append(&res, String(", "));
		}
	} else {
		String_Append(&res, method->name);
		String_Append(&res, String("Template *tpl, "));
	}

	String_Append(&res, String("String *res)"));

	if (src) {
		call(WriteSourceString, res);
	} else {
		call(WriteHeaderString, res);
	}

	String_Destroy(&res);
}

static def(void, WriteSource, Method_List *methods) {
	call(WriteSourceString, Output_Warning);

	call(WriteSourceString, String("#import \""));
	call(WriteSourceString, this->className);
	call(WriteSourceString, String(".h\"\n\n"));

	call(WriteSourceString, String("#define self "));
	call(WriteSourceString, this->className);
	call(WriteSourceString, String("\n\n"));

	/* Use a reverse loop because declaring blocks' prototypes is
	 * not compulsory. */
	DoublyLinkedList_ReverseForeach(methods, node) {
		Method *method = Method_GetObject(node->method);

		/* Omit empty methods. */
		if (method->lines.first != NULL) {
			call(WriteDeclaration, method, true);
			call(WriteSourceString, String(" {\n"));

			LinkedList_Foreach(&method->lines, lineNode) {
				for (size_t i = 0; i <= lineNode->indent; i++) {
					call(WriteSourceString, String("\t"));
				}

				call(WriteSourceString, lineNode->line);
				call(WriteSourceString, String("\n"));
			}

			call(WriteSourceString, String("}\n\n"));
		}
	}

	if (this->itf) {
		call(WriteSourceString, String("TemplateInterface Templates_"));
		call(WriteSourceString, this->className);
		call(WriteSourceString, String(" = {\n"));

		LinkedList_Foreach(methods, node) {
			Method *method = Method_GetObject(node->method);

			if (method->block) {
				continue;
			}

			String name = String_Clone(method->name);
			name.buf[0] = (char) Char_ToLower(name.buf[0]);

			call(WriteSourceString, String("\t."));
			call(WriteSourceString, name);
			call(WriteSourceString, String(" = &"));
			call(WriteSourceString, this->className);
			call(WriteSourceString, String("_"));
			call(WriteSourceString, method->name);
			call(WriteSourceString, String(",\n"));

			String_Destroy(&name);
		}

		call(WriteSourceString, String("};"));
	}
}

static def(void, WriteHeader, Method_List *methods) {
	call(WriteHeaderString, Output_Warning);

	call(WriteHeaderString, String("#import <String.h>\n"));
	call(WriteHeaderString, String("#import <Integer.h>\n"));
	call(WriteHeaderString, String("#import <tplgen/Template.h>\n\n"));

	call(WriteHeaderString, String("#import \""));
	call(WriteHeaderString, this->className);
	call(WriteHeaderString, String(".private.h\"\n\n"));

	call(WriteHeaderString, String("#define self "));
	call(WriteHeaderString, this->className);
	call(WriteHeaderString, String("\n\n"));

	if (this->itf) {
		call(WriteHeaderString, String("TemplateInterface Templates_"));
		call(WriteHeaderString, this->className);
		call(WriteHeaderString, String(";"));
	} else {
		DoublyLinkedList_ReverseForeach(methods, node) {
			Method *method = Method_GetObject(node->method);

			if (method->lines.first != NULL) {
				call(WriteDeclaration, method, false);
				call(WriteHeaderString, String(";\n"));
			}
		}
	}

	call(WriteHeaderString, String("\n#undef self\n"));
}

def(void, Write, Method_List *methods) {
	call(WriteHeader, methods);
	call(WriteSource, methods);
}
