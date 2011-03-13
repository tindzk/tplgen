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

	each(file, this->files) {
		String_Destroy(&file->name);
		String_Destroy(&file->file);
	}

	scall(TemplateArray_Free, this->files);
}

def(bool, SetOption, RdString name, RdString value) {
	if (String_Equals(name, $("name"))) {
		String_Copy(&this->name, value);
	} else if (String_Equals(name, $("itf"))) {
		this->itf = String_Equals(value, $("yes"));
	} else if (String_Equals(name, $("add"))) {
		RdStringArray *parts = String_Split(value, ':');

		if (parts->len < 2) {
			Logger_Error(&logger,
				$("`add' requires two values separated by a colon."));
			RdStringArray_Free(parts);
			return false;
		}

		scall(TemplateArray_Push, &this->files, (ref(TemplateItem)) {
			.name = String_Clone(parts->buf[0]),
			.file = String_Clone(parts->buf[1])
		});

		RdStringArray_Free(parts);
	} else if (String_Equals(name, $("dir"))) {
		String_Copy(&this->dir, value);
	} else if (String_Equals(name, $("ext"))) {
		String_Copy(&this->ext, value);
	} else if (String_Equals(name, $("out"))) {
		String_Copy(&this->out, value);
	}

	return true;
}

static def(String, FormatVariables, RdString s) {
	String tmp = String_Clone(s);

	String_ReplaceAll(&tmp, $("#"), $("tpl->"));
	String_ReplaceAll(&tmp, $("$"), $(""));

	return tmp;
}

static def(void, HandlePrintVariable, MethodInstance method, RdString s, RdString s2) {
	String var  = call(FormatVariables, s);
	String var2 = call(FormatVariables, s2);

	Method_AddLine(method,
		String_Format($("Template_Print(%%, res);"),
			var.rd, var2.rd));

	String_Destroy(&var2);
	String_Destroy(&var);
}

static def(void, HandleTemplate, MethodInstance method, RdString s) {
	String var = call(FormatVariables, s);

	Method_AddLine(method,
		String_Format($("callback(%, res);"), var.rd));

	String_Destroy(&var);
}

static def(void, HandleIf, MethodInstance method, RdString s) {
	String var = call(FormatVariables, s);

	Method_AddLine(method,
		String_Format($("if (%) {"), var.rd));
	Method_Indent(method);

	String_Destroy(&var);
}

static def(void, HandleIfEmpty, MethodInstance method, RdString s) {
	String var = call(FormatVariables, s);

	Method_AddLine(method,
		String_Format($("if (%.len == 0) {"), var.rd));
	Method_Indent(method);

	String_Destroy(&var);
}

static def(void, HandleElse, MethodInstance method, RdString s) {
	Method_Unindent(method);

	if (s.len == 0) {
		Method_AddLine(method, $$("} else {"));
		Method_Indent(method);
	} else {
		String var = call(FormatVariables, s);

		Method_AddLine(method,
			String_Format($("} else if (%) {"), var.rd));
		Method_Indent(method);

		String_Destroy(&var);
	}
}

static def(void, HandleEnd, MethodInstance method) {
	Method_Unindent(method);
	Method_AddLine(method, $$("}"));
}

static def(void, HandleBlock, MethodInstance method, RdString params) {
	ssize_t offset = String_Find(params, ' ');

	String line;

	if (offset == String_NotFound) {
		line = String_Format($("%(res);"), params);
	} else {
		String var = call(FormatVariables,
			String_Slice(params, offset + 1));

		line = String_Format($("%(%, res);"),
			String_Slice(params, 0, offset),
			var.rd);

		String_Destroy(&var);
	}

	Method_AddLine(method, line);
}

static def(void, HandleFor, MethodInstance method, RdString params) {
	RdStringArray *parts = String_Split(params, ' ');

	if (parts->len < 3) {
		Logger_Error(&logger, $("Incomplete for-loop."));
		throw(ParsingFailed);
	}

	String iter = call(FormatVariables, parts->buf[0]);
	RdString option = parts->buf[1];
	String from = call(FormatVariables, parts->buf[2]);

	if (!String_Equals(option, $("in"))) {
		Logger_Error(&logger, $("For loops don't support '%'"),
			option);

		throw(ParsingFailed);
	}

	ssize_t pos = String_Find(from.rd, 0, $(".."));

	bool isRange = (pos != String_NotFound);

	if (isRange) {
		RdString lower = String_Slice(from.rd, 0, pos);
		RdString upper = String_Slice(from.rd, pos + 2);

		Method_AddLine(method, String_Format(
			$("range (%, %, %) {"),
			iter.rd, lower, upper));

		Method_Indent(method);
	} else {
		Method_AddLine(method,
			String_Format(
				$("each(_%, %) {"),
				iter.rd, from.rd));

		Method_Indent(method);

		Method_AddLine(method,
			String_Format(
				$("typeof(*_%) % = *_%;"),
				iter.rd, iter.rd, iter.rd));
	}

	String_Destroy(&from);
	String_Destroy(&iter);

	RdStringArray_Free(parts);
}

static def(void, HandlePass, MethodInstance method, RdString name, RdString params) {
	String vars = call(FormatVariables, params);

	Method_AddLine(method,
		String_Format($("Template_Print(%(%), res);"),
			String_Slice(name, 1),
			vars.rd));

	String_Destroy(&vars);
}

static def(void, HandleCommand, MethodInstance method, RdString name, RdString params) {
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

static def(String, EscapeLine, RdString s) {
	String res = String_New(s.len + 15);

	fwd(i, s.len) {
		if (s.buf[i] != '"') {
			String_Append(&res, s.buf[i]);
		} else {
			String_Append(&res, $("\\\""));
		}
	}

	return res;
}

static def(void, FlushBuf, MethodInstance method, RdString s) {
	if (s.len == 0) {
		return;
	}

	bool flushed = false;

	RdStringArray *items = String_Split(s, '\n');

	fwd(i, items->len) {
		RdString line = items->buf[i];

		if (!flushed) {
			Method_AddLine(method, $$("String_Append(res, $("));
			Method_Indent(method);
			flushed = true;
		}

		String escaped = call(EscapeLine, line);

		RdString nl = $("");
		if (items->len > 1) {
			if (i + 1 != items->len) {
				nl = $("\\n");
			}
		}

		Method_AddLine(method,
			String_Format(
				$("\"%%\""), escaped.rd, nl));

		String_Destroy(&escaped);
	}

	if (flushed) {
		Method_Unindent(method);
		Method_AddLine(method, $$("));"));
	}

	RdStringArray_Free(items);
}

static def(Method *, NewMethod, String name) {
	Method *method = Method_Alloc();

	Method_Init(method, name, this->itf);

	Method_Item *item = Method_Item_Alloc();

	item->method = method;

	DoublyLinkedList_InsertEnd(&this->methods, item);

	return method;
}

static def(Method *, NewBlockMethod, String name, String params, bool public) {
	Method *method = Method_Alloc();

	Method_Init(method, name, !public);

	Method_SetBlock(method, true);
	Method_SetParameters(method, params);

	Method_Item *item = Method_Item_Alloc();

	item->method = method;

	DoublyLinkedList_InsertEnd(&this->methods, item);

	return method;
}

static def(bool, StartsCommandBlock, RdString cmd) {
	return String_Equals(cmd, $("if"))
		|| String_Equals(cmd, $("for"))
		|| String_Equals(cmd, $("else"))
		|| String_Equals(cmd, $("empty"));
}

static def(bool, EndsCommandBlock, RdString cmd) {
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
			RdString text = cur.text.rd;

			if ((prev.state == Parser_State_Command && call(StartsCommandBlock, prev.cmd.name.rd))
			  || first)
			{
				text  = String_Trim(text, String_TrimLeft);
				first = false;
			}

			next = Parser_Fetch(parser);

			if ((next.state == Parser_State_Command && call(EndsCommandBlock, next.cmd.name.rd))
			  || next.state == Parser_State_Block
			  || next.state == Parser_State_None)
			{
				text = String_Trim(text, String_TrimRight);
			}

			call(FlushBuf, method, text);
		} else {
			if (cur.state == Parser_State_Command) {
				call(HandleCommand, method,
					cur.cmd.name.rd,
					cur.cmd.params.rd);
			} else if (cur.state == Parser_State_Block) {
				if (inBlock) {
					if (String_Equals(cur.block.rd, $("end"))) {
						goto out;
					}

					Logger_Error(&logger,
						$("'%' not understood."),
						cur.block.rd);

					throw(ParsingFailed);
				}

				ssize_t pos = String_Find(cur.block.rd, $(": "));

				RdString blkname = cur.block.rd;
				String blkparams = String_New(0);

				if (pos != String_NotFound) {
					blkname   = String_Slice(cur.block.rd, 0, pos);
					blkparams = call(FormatVariables,
						String_Slice(cur.block.rd, pos + 2));
				}

				RdString tmp;
				bool public = String_BeginsWith(blkname, tmp = $("public "));

				if (public) {
					blkname = String_Slice(blkname, tmp.len);
				}

				Method *blockMethod =
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
	Directory_Init(&dir, this->dir.rd);

	while (Directory_Read(&dir, &item)) {
		if (item.type != Directory_ItemType_Symlink &&
			item.type != Directory_ItemType_Regular)
		{
			continue;
		}

		if (!String_EndsWith(item.name, this->ext.rd)) {
			continue;
		}

		scall(TemplateArray_Push, &this->files, (ref(TemplateItem)) {
			.name =
				String_Clone(
					String_Slice(
						item.name, 0, -this->ext.len)),

			.file = String_Format($("%/%"), this->dir.rd, item.name)
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
	Output_Init(&output, this->out.rd, this->itf);
	Output_SetClassName(&output, String_Clone(this->name.rd));

	fwd(i, this->files->len) {
		Logger_Info(&logger, $("Processing %..."),
			this->files->buf[i].file.rd);

		File tplFile;
		FileStream_Open(&tplFile, this->files->buf[i].file.rd,
			FileStatus_ReadOnly);

		BufferedStream stream = BufferedStream_New(File_AsStream(&tplFile));
		BufferedStream_SetInputBuffer(&stream, 4096, 256);

		Parser parser;
		Parser_Init(&parser, BufferedStream_AsStream(&stream));

		Method *method = call(NewMethod,
			String_Clone(this->files->buf[i].name.rd));
		call(ParseTemplate, &parser, false, method);

		BufferedStream_Close(&stream);
		BufferedStream_Destroy(&stream);
	}

	Output_Write(&output, &this->methods);
	Output_Destroy(&output);
}
