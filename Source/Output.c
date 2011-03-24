#import "Output.h"

extern Logger logger;

#define self Output

static odef(void, open, RdString path, File *file, BufferedStream *stream) {
	try {
		File_Open(file, path,
			FileStatus_Create    |
			FileStatus_Truncate  |
			FileStatus_WriteOnly);

		*stream = BufferedStream_New(File_AsStream(file));
		BufferedStream_SetOutputBuffer(stream, 4096);
	} catchModule(File) {
		Logger_Error(&logger,
			$("Couldn't open file % for writing."),
			path);

		__exc_rethrow = true;
	} finally {

	} tryEnd;
}

odef(void, init, RdString file, bool itf) {
	this->itf = itf;
	this->className = String_New(0);

	String src = String_Concat(file, $(".c"));
	String hdr = String_Concat(file, $(".h"));

	open(this, src.rd, &this->srcFile, &this->src);
	open(this, hdr.rd, &this->hdrFile, &this->hdr);

	String_Destroy(&hdr);
	String_Destroy(&src);
}

odef(void, destroy) {
	String_Destroy(&this->className);

	BufferedStream_Close(&this->src);
	BufferedStream_Close(&this->hdr);

	BufferedStream_Destroy(&this->src);
	BufferedStream_Destroy(&this->hdr);
}

static odef(void, writeSourceString, RdString s) {
	BufferedStream_Write(&this->src, s.buf, s.len);
}

static odef(void, writeHeaderString, RdString s) {
	BufferedStream_Write(&this->hdr, s.buf, s.len);
}

odef(void, setClassName, String s) {
	String_Assign(&this->className, s);
}

static odef(void, writeDeclaration, Method *method, bool src) {
	String res = String_New(0);

	if (method->hidden) {
		String_Append(&res, $("static "));
	}

	String_Append(&res, $("void "));

	if (!method->block) {
		String_Append(&res, this->className.rd);
		String_Append(&res, $("_"));
	}

	String_Append(&res, method->name.rd);
	String_Append(&res, $("("));

	if (method->block) {
		String_Append(&res, method->params.rd);

		if (method->params.len > 0) {
			String_Append(&res, $(", "));
		}
	} else {
		String_Append(&res, method->name.rd);
		String_Append(&res, $("Template *tpl, "));
	}

	String_Append(&res, $("String *res)"));

	if (src) {
		writeSourceString(this, res.rd);
	} else {
		writeHeaderString(this, res.rd);
	}

	String_Destroy(&res);
}

static odef(void, writeSource, Method_List *methods) {
	writeSourceString(this, Output_Warning);

	writeSourceString(this, $("#import \""));
	writeSourceString(this, this->className.rd);
	writeSourceString(this, $(".h\"\n\n"));

	writeSourceString(this, $("#define self "));
	writeSourceString(this, this->className.rd);
	writeSourceString(this, $("\n\n"));

	/* Use a reverse loop because declaring blocks' prototypes is
	 * not compulsory. */
	DoublyLinkedList_ReverseForeach(methods, node) {
		Method *method = Method_GetObject(node->method);

		/* Omit empty methods. */
		if (method->lines.first != NULL) {
			writeDeclaration(this, method, true);
			writeSourceString(this, $(" {\n"));

			LinkedList_Foreach(&method->lines, lineNode) {
				rpt(lineNode->indent + 1) {
					writeSourceString(this, $("\t"));
				}

				writeSourceString(this, lineNode->line.rd);
				writeSourceString(this, $("\n"));
			}

			writeSourceString(this, $("}\n\n"));
		}
	}

	if (this->itf) {
		writeSourceString(this, $("TemplateInterface Templates_"));
		writeSourceString(this, this->className.rd);
		writeSourceString(this, $(" = {\n"));

		LinkedList_Foreach(methods, node) {
			Method *method = Method_GetObject(node->method);

			if (method->block) {
				continue;
			}

			String name = String_Clone(method->name.rd);
			name.buf[0] = (char) Char_ToLower(name.buf[0]);

			writeSourceString(this, $("\t."));
			writeSourceString(this, name.rd);
			writeSourceString(this, $(" = &"));
			writeSourceString(this, this->className.rd);
			writeSourceString(this, $("_"));
			writeSourceString(this, method->name.rd);
			writeSourceString(this, $(",\n"));

			String_Destroy(&name);
		}

		writeSourceString(this, $("};"));
	}
}

static odef(void, writeHeader, Method_List *methods) {
	writeHeaderString(this, Output_Warning);

	writeHeaderString(this, $("#import <String.h>\n"));
	writeHeaderString(this, $("#import <Integer.h>\n"));
	writeHeaderString(this, $("#import <tplgen/Template.h>\n\n"));

	writeHeaderString(this, $("#import \""));
	writeHeaderString(this, this->className.rd);
	writeHeaderString(this, $(".private.h\"\n\n"));

	writeHeaderString(this, $("#define self "));
	writeHeaderString(this, this->className.rd);
	writeHeaderString(this, $("\n\n"));

	if (this->itf) {
		writeHeaderString(this, $("TemplateInterface Templates_"));
		writeHeaderString(this, this->className.rd);
		writeHeaderString(this, $(";"));
	} else {
		DoublyLinkedList_ReverseForeach(methods, node) {
			Method *method = Method_GetObject(node->method);

			if (method->lines.first != NULL) {
				writeDeclaration(this, method, false);
				writeHeaderString(this, $(";\n"));
			}
		}
	}

	writeHeaderString(this, $("\n#undef self\n"));
}

odef(void, write, Method_List *methods) {
	writeHeader(this, methods);
	writeSource(this, methods);
}
