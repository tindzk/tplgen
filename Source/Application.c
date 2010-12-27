#import "Application.h"

#define self Application

extern Logger logger;

def(void, Init) {
	this->itf   = true;
	this->dir   = HeapString(0);
	this->ext   = HeapString(0);
	this->out   = HeapString(0);
	this->name  = HeapString(0);
	this->files = scall(TemplateArray_New, 50);

	DoublyLinkedList_Init(&this->methods);
}

def(void, Destroy) {
	String_Destroy(&this->dir);
	String_Destroy(&this->ext);
	String_Destroy(&this->out);
	String_Destroy(&this->name);

	DoublyLinkedList_Destroy(&this->methods, ^(Method_Item *item) {
		Method_Destroy(item->method);
		Method_Free(item->method);
		Memory_Free(item);
	});

	foreach (file, this->files) {
		String_Destroy(&file->name);
		String_Destroy(&file->file);
	}

	scall(TemplateArray_Free, this->files);
}

def(bool, SetOption, String name, String value) {
	if (String_Equals(name, $("name"))) {
		String_Copy(&this->name, value);
	} else if (String_Equals(name, $("itf"))) {
		this->itf = String_Equals(value, $("yes"));
	} else if (String_Equals(name, $("add"))) {
		StringArray *parts = String_Split(value, ':');

		if (parts->len < 2) {
			Logger_Error(&logger,
				$("`add' requires two values separated by a colon."));

			StringArray_Destroy(parts);

			return false;
		}

		ref(TemplateItem) insert;

		insert.name = String_Clone(parts->buf[0]);
		insert.file = String_Clone(parts->buf[1]);

		scall(TemplateArray_Push, &this->files, insert);

		StringArray_Free(parts);
	} else if (String_Equals(name, $("dir"))) {
		String_Copy(&this->dir, value);
	} else if (String_Equals(name, $("ext"))) {
		String_Copy(&this->ext, value);
	} else if (String_Equals(name, $("out"))) {
		String_Copy(&this->out, value);
	}

	return true;
}

static def(String, FormatVariables, String s) {
	String tmp = String_Clone(s);

	String_ReplaceAll(&tmp, $("#"), $("tpl->"));
	String_ReplaceAll(&tmp, $("$"), $(""));

	return tmp;
}

static def(void, HandlePrintVariable, MethodInstance method, String s, String s2) {
	String var  = call(FormatVariables, s);
	String var2 = call(FormatVariables, s2);

	String line = String_Format($("Template_Print(%%, res);"),
		var, var2);
	Method_AddLine(method, line);
	String_Destroy(&line);

	String_Destroy(&var2);
	String_Destroy(&var);
}

static def(void, HandleTemplate, MethodInstance method, String s) {
	String var  = call(FormatVariables, s);
	String line = String_Format($("callback(%, res);"), var);

	Method_AddLine(method, line);

	String_Destroy(&line);
	String_Destroy(&var);
}

static def(void, HandleIf, MethodInstance method, String s) {
	String var  = call(FormatVariables, s);
	String line = String_Format($("if (%) {"), var);

	Method_AddLine(method, line);
	Method_Indent(method);

	String_Destroy(&line);
	String_Destroy(&var);
}

static def(void, HandleIfEmpty, MethodInstance method, String s) {
	String var  = call(FormatVariables, s);
	String line = String_Format($("if (%.len == 0) {"), var);

	Method_AddLine(method, line);
	Method_Indent(method);

	String_Destroy(&line);
	String_Destroy(&var);
}

static def(void, HandleElse, MethodInstance method, String s) {
	Method_Unindent(method);

	if (s.len == 0) {
		Method_AddLine(method, $("} else {"));
		Method_Indent(method);
	} else {
		String var  = call(FormatVariables, s);
		String line = String_Format($("} else if (%) {"), var);

		Method_AddLine(method, line);
		Method_Indent(method);

		String_Destroy(&line);
		String_Destroy(&var);
	}
}

static def(void, HandleEnd, MethodInstance method) {
	Method_Unindent(method);
	Method_AddLine(method, $("}"));
}

static def(void, HandleBlock, MethodInstance method, String params) {
	ssize_t offset = String_Find(params, ' ');

	String line;

	if (offset == String_NotFound) {
		line = String_Format($("%(res);"), params);
	} else {
		String var = call(FormatVariables,
			String_Slice(params, offset + 1));

		line = String_Format($("%(%, res);"),
			String_Slice(params, 0, offset),
			var);

		String_Destroy(&var);
	}

	Method_AddLine(method, line);

	String_Destroy(&line);
}

static def(void, HandleFor, MethodInstance method, String params) {
	StringArray *parts = String_Split(params, ' ');

	if (parts->len < 3) {
		Logger_Error(&logger, $("Incomplete for-loop."));
		throw(ParsingFailed);
	}

	String iter   = call(FormatVariables, parts->buf[0]);
	String option = parts->buf[1];
	String from   = call(FormatVariables, parts->buf[2]);

	if (!String_Equals(option, $("in"))) {
		Logger_Error(&logger, $("For loops don't support '%'"),
			option);

		throw(ParsingFailed);
	}

	ssize_t pos = String_Find(from, 0, $(".."));

	bool isRange = (pos != String_NotFound);

	if (isRange) {
		String lower = String_Slice(from, 0, pos);
		String upper = String_Slice(from, pos + 2);

		String line = String_Format(
			$("range (%, %, %) {"),
			iter, lower, upper);

		Method_AddLine(method, line);
		Method_Indent(method);

		String_Destroy(&line);
	} else {
		String line1 = String_Format(
			$("forward (i, %->len) {"),
			from);

		String line2 = String_Format(
			$("typeof(%->buf[0]) % = %->buf[i];"),
			from, iter, from);

		Method_AddLine(method, line1);
		Method_Indent(method);

		Method_AddLine(method, line2);

		String_Destroy(&line2);
		String_Destroy(&line1);
	}

	String_Destroy(&from);
	String_Destroy(&iter);

	StringArray_Free(parts);
}

static def(void, HandlePass, MethodInstance method, String name, String params) {
	String vars = call(FormatVariables, params);

	String line = String_Format($("Template_Print(%(%), res);"),
		String_Slice(name, 1),
		vars);

	Method_AddLine(method, line);
	String_Destroy(&line);

	String_Destroy(&vars);
}

static def(void, HandleCommand, MethodInstance method, String name, String params) {
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

static def(String, EscapeLine, String s) {
	String res = HeapString(s.len + 15);

	for (size_t i = 0; i < s.len; i++) {
		if (s.buf[i] != '"') {
			String_Append(&res, s.buf[i]);
		} else {
			String_Append(&res, $("\\\""));
		}
	}

	return res;
}

static def(void, FlushBuf, MethodInstance method, String s) {
	if (s.len == 0) {
		return;
	}

	bool flushed = false;

	StringArray *items = String_Split(s, '\n');

	for (size_t i = 0; i < items->len; i++) {
		String line = items->buf[i];

		if (!flushed) {
			Method_AddLine(method, $("String_Append(res, $("));
			Method_Indent(method);
			flushed = true;
		}

		String escaped = call(EscapeLine, line);

		String_Prepend(&escaped, '"');

		if (items->len > 1) {
			if (i + 1 != items->len) {
				String_Append(&escaped, $("\\n"));
			}
		}

		String_Append(&escaped, '"');

		Method_AddLine(method, escaped);

		String_Destroy(&escaped);
	}

	if (flushed) {
		Method_Unindent(method);
		Method_AddLine(method, $("));"));
	}

	StringArray_Free(items);
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

static def(bool, StartsCommandBlock, String cmd) {
	return String_Equals(cmd, $("if"))
		|| String_Equals(cmd, $("for"))
		|| String_Equals(cmd, $("else"))
		|| String_Equals(cmd, $("empty"));
}

static def(bool, EndsCommandBlock, String cmd) {
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
			String text = cur.text;

			if ((prev.state == Parser_State_Command && call(StartsCommandBlock, prev.cmd.name))
			  || first)
			{
				text  = String_Trim(text, String_TrimLeft);
				first = false;
			}

			next = Parser_Fetch(parser);

			if ((next.state == Parser_State_Command && call(EndsCommandBlock, next.cmd.name))
			  || next.state == Parser_State_Block
			  || next.state == Parser_State_None)
			{
				text = String_Trim(text, String_TrimRight);
			}

			call(FlushBuf, method, text);
		} else {
			if (cur.state == Parser_State_Command) {
				call(HandleCommand, method,
					cur.cmd.name,
					cur.cmd.params);
			} else if (cur.state == Parser_State_Block) {
				if (inBlock) {
					if (String_Equals(cur.block, $("end"))) {
						goto out;
					}

					Logger_Error(&logger,
						$("'%' not understood."),
						cur.block);

					throw(ParsingFailed);
				}

				ssize_t pos = String_Find(cur.block, $(": "));

				String blkname   = cur.block;
				String blkparams = HeapString(0);

				if (pos != String_NotFound) {
					blkname   = String_Slice(cur.block, 0, pos);
					blkparams = String_Slice(cur.block, pos + 2);
					blkparams = call(FormatVariables, blkparams);
				}

				String tmp;
				bool public = String_BeginsWith(blkname, tmp = $("public "));

				if (public) {
					blkname = String_Slice(blkname, tmp.len);
				}

				MethodInstance blockMethod =
					call(NewBlockMethod, blkname, blkparams, public);
				call(ParseTemplate, parser, true, blockMethod);

				String_Destroy(&blkparams);
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
	Directory_Init(&dir, this->dir);

	while (Directory_Read(&dir, &item)) {
		if (item.type != Directory_ItemType_Symlink
		 && item.type != Directory_ItemType_Regular) {
			continue;
		}

		if (!String_EndsWith(item.name, this->ext)) {
			continue;
		}

		ref(TemplateItem) insert;

		insert.name =
			String_Clone(
				String_Slice(
					item.name, 0, -this->ext.len));

		insert.file = String_Format($("%/%"), this->dir, item.name);

		scall(TemplateArray_Push, &this->files, insert);
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
	Output_Init(&output, this->out, this->itf);
	Output_SetClassName(&output, this->name);

	for (size_t i = 0; i < this->files->len; i++) {
		Logger_Info(&logger, $("Processing %..."),
			this->files->buf[i].file);

		File tplFile;
		FileStream_Open(&tplFile, this->files->buf[i].file,
			FileStatus_ReadOnly);

		BufferedStream stream;
		BufferedStream_Init(&stream, File_AsStream(&tplFile));
		BufferedStream_SetInputBuffer(&stream, 4096, 256);

		Parser parser;
		Parser_Init(&parser, BufferedStream_AsStream(&stream));

		MethodInstance method = call(NewMethod, this->files->buf[i].name);
		call(ParseTemplate, &parser, false, method);

		BufferedStream_Close(&stream);
		BufferedStream_Destroy(&stream);
	}

	Output_Write(&output, &this->methods);
	Output_Destroy(&output);
}
