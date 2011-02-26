#import "Output.h"

extern Logger logger;

#define self Output

static def(void, Open, ProtString path, File *file, BufferedStream *stream) {
	try {
		File_Open(file, path,
			FileStatus_Create    |
			FileStatus_Truncate  |
			FileStatus_WriteOnly);

		BufferedStream_Init(stream, File_AsStream(file));
		BufferedStream_SetOutputBuffer(stream, 4096);
	} clean catchModule(File) {
		Logger_Error(&logger,
			$("Couldn't open file % for writing."),
			path);

		__exc_rethrow = true;
	} finally {

	} tryEnd;
}

def(void, Init, ProtString file, bool itf) {
	this->itf = itf;
	this->className = String_New(0);

	String src = String_Concat(file, $(".c"));
	String hdr = String_Concat(file, $(".h"));

	call(Open, src.prot, &this->srcFile, &this->src);
	call(Open, hdr.prot, &this->hdrFile, &this->hdr);

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

static def(void, WriteSourceString, ProtString s) {
	BufferedStream_Write(&this->src, s.buf, s.len);
}

static def(void, WriteHeaderString, ProtString s) {
	BufferedStream_Write(&this->hdr, s.buf, s.len);
}

def(void, SetClassName, String s) {
	String_Assign(&this->className, &s);
}

static def(void, WriteDeclaration, Method *method, bool src) {
	String res = String_New(0);

	if (method->hidden) {
		String_Append(&res, $("static "));
	}

	String_Append(&res, $("void "));

	if (!method->block) {
		String_Append(&res, this->className.prot);
		String_Append(&res, $("_"));
	}

	String_Append(&res, method->name.prot);
	String_Append(&res, $("("));

	if (method->block) {
		String_Append(&res, method->params.prot);

		if (method->params.len > 0) {
			String_Append(&res, $(", "));
		}
	} else {
		String_Append(&res, method->name.prot);
		String_Append(&res, $("Template *tpl, "));
	}

	String_Append(&res, $("String *res)"));

	if (src) {
		call(WriteSourceString, res.prot);
	} else {
		call(WriteHeaderString, res.prot);
	}

	String_Destroy(&res);
}

static def(void, WriteSource, Method_List *methods) {
	call(WriteSourceString, Output_Warning);

	call(WriteSourceString, $("#import \""));
	call(WriteSourceString, this->className.prot);
	call(WriteSourceString, $(".h\"\n\n"));

	call(WriteSourceString, $("#define self "));
	call(WriteSourceString, this->className.prot);
	call(WriteSourceString, $("\n\n"));

	/* Use a reverse loop because declaring blocks' prototypes is
	 * not compulsory. */
	DoublyLinkedList_ReverseForeach(methods, node) {
		Method *method = Method_GetObject(node->method);

		/* Omit empty methods. */
		if (method->lines.first != NULL) {
			call(WriteDeclaration, method, true);
			call(WriteSourceString, $(" {\n"));

			LinkedList_Foreach(&method->lines, lineNode) {
				repeat (lineNode->indent + 1) {
					call(WriteSourceString, $("\t"));
				}

				call(WriteSourceString, lineNode->line.prot);
				call(WriteSourceString, $("\n"));
			}

			call(WriteSourceString, $("}\n\n"));
		}
	}

	if (this->itf) {
		call(WriteSourceString, $("TemplateInterface Templates_"));
		call(WriteSourceString, this->className.prot);
		call(WriteSourceString, $(" = {\n"));

		LinkedList_Foreach(methods, node) {
			Method *method = Method_GetObject(node->method);

			if (method->block) {
				continue;
			}

			String name = String_Clone(method->name.prot);
			name.buf[0] = (char) Char_ToLower(name.buf[0]);

			call(WriteSourceString, $("\t."));
			call(WriteSourceString, name.prot);
			call(WriteSourceString, $(" = &"));
			call(WriteSourceString, this->className.prot);
			call(WriteSourceString, $("_"));
			call(WriteSourceString, method->name.prot);
			call(WriteSourceString, $(",\n"));

			String_Destroy(&name);
		}

		call(WriteSourceString, $("};"));
	}
}

static def(void, WriteHeader, Method_List *methods) {
	call(WriteHeaderString, Output_Warning);

	call(WriteHeaderString, $("#import <String.h>\n"));
	call(WriteHeaderString, $("#import <Integer.h>\n"));
	call(WriteHeaderString, $("#import <tplgen/Template.h>\n\n"));

	call(WriteHeaderString, $("#import \""));
	call(WriteHeaderString, this->className.prot);
	call(WriteHeaderString, $(".private.h\"\n\n"));

	call(WriteHeaderString, $("#define self "));
	call(WriteHeaderString, this->className.prot);
	call(WriteHeaderString, $("\n\n"));

	if (this->itf) {
		call(WriteHeaderString, $("TemplateInterface Templates_"));
		call(WriteHeaderString, this->className.prot);
		call(WriteHeaderString, $(";"));
	} else {
		DoublyLinkedList_ReverseForeach(methods, node) {
			Method *method = Method_GetObject(node->method);

			if (method->lines.first != NULL) {
				call(WriteDeclaration, method, false);
				call(WriteHeaderString, $(";\n"));
			}
		}
	}

	call(WriteHeaderString, $("\n#undef self\n"));
}

def(void, Write, Method_List *methods) {
	call(WriteHeader, methods);
	call(WriteSource, methods);
}
