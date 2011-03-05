#import "Application.h"

#define self Application

extern Logger logger;

rsdef(self, New) {
	self res;

	res.itf   = true;
	res.dir   = String_New(0);
	res.ext   = String_New(0);
	res.out   = String_New(0);
	res.name  = String_New(0);
	res.files = scall(TemplateArray_New, 50);

	DoublyLinkedList_Init(&res.methods);

	return res;
}

def(void, Destroy) {
	String_Destroy(&this->dir);
	String_Destroy(&this->ext);
	String_Destroy(&this->out);
	String_Destroy(&this->name);

	DoublyLinkedList_Destroy(&this->methods, ^(Method_Item *item) {
		Method_Destroy(item->method);
		Method_Free(item->method);
	});

	foreach (file, this->files) {
		String_Destroy(&file->name);
		String_Destroy(&file->file);
	}

	scall(TemplateArray_Free, this->files);
}

def(bool, SetOption, ProtString name, ProtString value) {
	if (String_Equals(name, $("name"))) {
		String_Copy(&this->name, value);
	} else if (String_Equals(name, $("itf"))) {
		this->itf = String_Equals(value, $("yes"));
	} else if (String_Equals(name, $("add"))) {
		ProtStringArray *parts = String_Split(value, ':');

		if (parts->len < 2) {
			Logger_Error(&logger,
				$("`add' requires two values separated by a colon."));
			ProtStringArray_Free(parts);
			return false;
		}

		scall(TemplateArray_Push, &this->files, (ref(TemplateItem)) {
			.name = String_Clone(parts->buf[0]),
			.file = String_Clone(parts->buf[1])
		});

		ProtStringArray_Free(parts);
	} else if (String_Equals(name, $("dir"))) {
		String_Copy(&this->dir, value);
	} else if (String_Equals(name, $("ext"))) {
		String_Copy(&this->ext, value);
	} else if (String_Equals(name, $("out"))) {
		String_Copy(&this->out, value);
	}

	return true;
}

static def(String, FormatVariables, ProtString s) {
	String tmp = String_Clone(s);

	String_ReplaceAll(&tmp, $("#"), $("tpl->"));
	String_ReplaceAll(&tmp, $("$"), $(""));

	return tmp;
}

static def(void, HandlePrintVariable, MethodInstance method, ProtString s, ProtString s2) {
	String var  = call(FormatVariables, s);
	String var2 = call(FormatVariables, s2);

	Method_AddLine(method,
		String_Format($("Template_Print(%%, res);"),
			var.prot, var2.prot));

	String_Destroy(&var2);
	String_Destroy(&var);
}

static def(void, HandleTemplate, MethodInstance method, ProtString s) {
	String var = call(FormatVariables, s);

	Method_AddLine(method,
		String_Format($("callback(%, res);"), var.prot));

	String_Destroy(&var);
}

static def(void, HandleIf, MethodInstance method, ProtString s) {
	String var = call(FormatVariables, s);

	Method_AddLine(method,
		String_Format($("if (%) {"), var.prot));
	Method_Indent(method);

	String_Destroy(&var);
}

static def(void, HandleIfEmpty, MethodInstance method, ProtString s) {
	String var = call(FormatVariables, s);

	Method_AddLine(method,
		String_Format($("if (%.len == 0) {"), var.prot));
	Method_Indent(method);

	String_Destroy(&var);
}

static def(void, HandleElse, MethodInstance method, ProtString s) {
	Method_Unindent(method);

	if (s.len == 0) {
		Method_AddLine(method, $("} else {"));
		Method_Indent(method);
	} else {
		String var = call(FormatVariables, s);

		Method_AddLine(method,
			String_Format($("} else if (%) {"), var.prot));
		Method_Indent(method);

		String_Destroy(&var);
	}
}

static def(void, HandleEnd, MethodInstance method) {
	Method_Unindent(method);
	Method_AddLine(method, $("}"));
}

static def(void, HandleBlock, MethodInstance method, ProtString params) {
	ssize_t offset = String_Find(params, ' ');

	String line;

	if (offset == String_NotFound) {
		line = String_Format($("%(res);"), params);
	} else {
		String var = call(FormatVariables,
			String_Slice(params, offset + 1));

		line = String_Format($("%(%, res);"),
			String_Slice(params, 0, offset),
			var.prot);

		String_Destroy(&var);
	}

	Method_AddLine(method, line);
}

static def(void, HandleFor, MethodInstance method, ProtString params) {
	ProtStringArray *parts = String_Split(params, ' ');

	if (parts->len < 3) {
		Logger_Error(&logger, $("Incomplete for-loop."));
		throw(ParsingFailed);
	}

	String iter = call(FormatVariables, parts->buf[0]);
	ProtString option = parts->buf[1];
	String from = call(FormatVariables, parts->buf[2]);

	if (!String_Equals(option, $("in"))) {
		Logger_Error(&logger, $("For loops don't support '%'"),
			option);

		throw(ParsingFailed);
	}

	ssize_t pos = String_Find(from.prot, 0, $(".."));

	bool isRange = (pos != String_NotFound);

	if (isRange) {
		ProtString lower = String_Slice(from.prot, 0, pos);
		ProtString upper = String_Slice(from.prot, pos + 2);

		Method_AddLine(method, String_Format(
			$("range (%, %, %) {"),
			iter.prot, lower, upper));

		Method_Indent(method);
	} else {
		Method_AddLine(method,
			String_Format(
				$("foreach (_%, %) {"),
				iter.prot, from.prot));

		Method_Indent(method);

		Method_AddLine(method,
			String_Format(
				$("typeof(*_%) % = *_%;"),
				iter.prot, iter.prot, iter.prot));
	}

	String_Destroy(&from);
	String_Destroy(&iter);

	ProtStringArray_Free(parts);
}

static def(void, HandlePass, MethodInstance method, ProtString name, ProtString params) {
	String vars = call(FormatVariables, params);

	Method_AddLine(method,
		String_Format($("Template_Print(%(%), res);"),
			String_Slice(name, 1),
			vars.prot));

	String_Destroy(&vars);
}

static def(void, HandleCommand, MethodInstance method, ProtString name, ProtString params) {
	if (name.len == 0) {
		return;
	}

	if (name.buf[0] == '$' || name.buf[0] == '#') {
		call(HandlePrintVariable, method, name, params);
	} else if (name.buf[0] == '~') {
		call(HandlePass, method, name, params);
	} else if (String_Equals(name, $("for"))) {
		call(HandleFor, method, params);
	} else if (String_Equals(name, $("if"))) {
		call(HandleIf, method, params);
	} else if (String_Equals(name, $("empty"))) {
		call(HandleIfEmpty, method, params);
	} else if (String_Equals(name, $("else"))) {
		call(HandleElse, method, params);
	} else if (String_Equals(name, $("end"))) {
		call(HandleEnd, method);
	} else if (String_Equals(name, $("block"))) {
		call(HandleBlock, method, params);
	} else if (String_Equals(name, $("tpl"))) {
		call(HandleTemplate, method, params);
	} else {
		Logger_Error(&logger, $("Command '%' is unknown."),
			name);

		throw(ParsingFailed);
	}
}

static def(String, EscapeLine, ProtString s) {
	String res = String_New(s.len + 15);

	forward (i, s.len) {
		if (s.buf[i] != '"') {
			String_Append(&res, s.buf[i]);
		} else {
			String_Append(&res, $("\\\""));
		}
	}

	return res;
}

static def(void, FlushBuf, MethodInstance method, ProtString s) {
	if (s.len == 0) {
		return;
	}

	bool flushed = false;

	ProtStringArray *items = String_Split(s, '\n');

	forward (i, items->len) {
		ProtString line = items->buf[i];

		if (!flushed) {
			Method_AddLine(method, $("String_Append(res, $("));
			Method_Indent(method);
			flushed = true;
		}

		String escaped = call(EscapeLine, line);

		ProtString nl = $("");
		if (items->len > 1) {
			if (i + 1 != items->len) {
				nl = $("\\n");
			}
		}

		Method_AddLine(method,
			String_Format(
				$("\"%%\""), escaped.prot, nl));

		String_Destroy(&escaped);
	}

	if (flushed) {
		Method_Unindent(method);
		Method_AddLine(method, $("));"));
	}

	ProtStringArray_Free(items);
}

static def(MethodInstance, NewMethod, String name) {
	MethodInstance method = Method_New();

	Method_Init(method, name, this->itf);

	Method_Item *item = New(Method_Item);

	item->method = method;

	DoublyLinkedList_InsertEnd(&this->methods, item);

	return method;
}

static def(MethodInstance, NewBlockMethod, String name, String params, bool public) {
	MethodInstance method = Method_New();

	Method_Init(method, name, !public);

	Method_SetBlock(method, true);
	Method_SetParameters(method, params);

	Method_Item *item = New(Method_Item);

	item->method = method;

	DoublyLinkedList_InsertEnd(&this->methods, item);

	return method;
}

static def(bool, StartsCommandBlock, ProtString cmd) {
	return String_Equals(cmd, $("if"))
		|| String_Equals(cmd, $("for"))
		|| String_Equals(cmd, $("else"))
		|| String_Equals(cmd, $("empty"));
}

static def(bool, EndsCommandBlock, ProtString cmd) {
	return String_Equals(cmd, $("end"))
		|| String_Equals(cmd, $("else"));
}

static def(void, ParseTemplate, ParserInstance parser, bool inBlock, MethodInstance method) {
	Parser_Token prev = Parser_Token();
	Parser_Token cur  = Parser_Token();
	Parser_Token next = Parser_Token();

	bool first = inBlock;

	do {
		if (cur.state == Parser_State_Text) {
			ProtString text = cur.text.prot;

			if ((prev.state == Parser_State_Command && call(StartsCommandBlock, prev.cmd.name.prot))
			  || first)
			{
				text  = String_Trim(text, String_TrimLeft);
				first = false;
			}

			next = Parser_Fetch(parser);

			if ((next.state == Parser_State_Command && call(EndsCommandBlock, next.cmd.name.prot))
			  || next.state == Parser_State_Block
			  || next.state == Parser_State_None)
			{
				text = String_Trim(text, String_TrimRight);
			}

			call(FlushBuf, method, text);
		} else {
			if (cur.state == Parser_State_Command) {
				call(HandleCommand, method,
					cur.cmd.name.prot,
					cur.cmd.params.prot);
			} else if (cur.state == Parser_State_Block) {
				if (inBlock) {
					if (String_Equals(cur.block.prot, $("end"))) {
						goto out;
					}

					Logger_Error(&logger,
						$("'%' not understood."),
						cur.block.prot);

					throw(ParsingFailed);
				}

				ssize_t pos = String_Find(cur.block.prot, $(": "));

				ProtString blkname = cur.block.prot;
				String blkparams = String_New(0);

				if (pos != String_NotFound) {
					blkname   = String_Slice(cur.block.prot, 0, pos);
					blkparams = call(FormatVariables,
						String_Slice(cur.block.prot, pos + 2));
				}

				ProtString tmp;
				bool public = String_BeginsWith(blkname, tmp = $("public "));

				if (public) {
					blkname = String_Slice(blkname, tmp.len);
				}

				MethodInstance blockMethod =
					call(NewBlockMethod, String_Clone(blkname), blkparams, public);
				call(ParseTemplate, parser, true, blockMethod);
			}

			next = Parser_Fetch(parser);
		}

		Parser_DestroyToken(&prev);

		prev = cur;
		cur  = next;
	} while (cur.state != Parser_State_None);

	Parser_DestroyToken(&next);

out:
	Parser_DestroyToken(&prev);
	Parser_DestroyToken(&cur);
}

def(void, Scan) {
	Directory dir;
	Directory_Entry item;
	Directory_Init(&dir, this->dir.prot);

	while (Directory_Read(&dir, &item)) {
		if (item.type != Directory_ItemType_Symlink &&
			item.type != Directory_ItemType_Regular)
		{
			continue;
		}

		if (!String_EndsWith(item.name, this->ext.prot)) {
			continue;
		}

		scall(TemplateArray_Push, &this->files, (ref(TemplateItem)) {
			.name =
				String_Clone(
					String_Slice(
						item.name, 0, -this->ext.len)),

			.file = String_Format($("%/%"), this->dir.prot, item.name)
		});
	}

	Directory_Destroy(&dir);
}

def(void, Process) {
	if (this->out.len == 0) {
		Logger_Error(&logger, $("No output path is set."));
		throw(InvalidParameter);
	}

	if (this->dir.len > 0) {
		call(Scan);
	}

	Output output;
	Output_Init(&output, this->out.prot, this->itf);
	Output_SetClassName(&output, String_Clone(this->name.prot));

	forward (i, this->files->len) {
		Logger_Info(&logger, $("Processing %..."),
			this->files->buf[i].file.prot);

		File tplFile;
		FileStream_Open(&tplFile, this->files->buf[i].file.prot,
			FileStatus_ReadOnly);

		BufferedStream stream;
		BufferedStream_Init(&stream, File_AsStream(&tplFile));
		BufferedStream_SetInputBuffer(&stream, 4096, 256);

		Parser parser;
		Parser_Init(&parser, BufferedStream_AsStream(&stream));

		MethodInstance method = call(NewMethod,
			String_Clone(this->files->buf[i].name.prot));
		call(ParseTemplate, &parser, false, method);

		BufferedStream_Close(&stream);
		BufferedStream_Destroy(&stream);
	}

	Output_Write(&output, &this->methods);
	Output_Destroy(&output);
}
