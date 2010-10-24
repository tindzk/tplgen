#import "Application.h"
#import <App.h>

extern Logger logger;
extern ExceptionManager exc;

def(void, Init) {
	this->itf  = true;
	this->dir  = HeapString(0);
	this->ext  = HeapString(0);
	this->out  = HeapString(0);
	this->name = HeapString(0);

	Array_Init(this->files, 50);

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

	Array_Foreach(this->files, ^(ref(TemplateItem) *item) {
		String_Destroy(&item->name);
		String_Destroy(&item->file);
	});

	Array_Destroy(this->files);
}

def(bool, SetOption, String name, String value) {
	if (String_Equals(name, String("name"))) {
		String_Copy(&this->name, value);
	} else if (String_Equals(name, String("itf"))) {
		this->itf = String_Equals(value, String("yes"));
	} else if (String_Equals(name, String("add"))) {
		StringArray *parts = String_Split(value, ':');

		if (parts->len < 2) {
			Logger_Error(&logger,
				String("`add' requires two values separated by a colon."));

			Array_Destroy(parts);

			return false;
		}

		ref(TemplateItem) insert;

		insert.name = String_Clone(parts->buf[0]);
		insert.file = String_Clone(parts->buf[1]);

		Array_Push(this->files, insert);

		Array_Destroy(parts);
	} else if (String_Equals(name, String("dir"))) {
		String_Copy(&this->dir, value);
	} else if (String_Equals(name, String("ext"))) {
		String_Copy(&this->ext, value);
	} else if (String_Equals(name, String("out"))) {
		String_Copy(&this->out, value);
	}

	return true;
}

static def(String, FormatVariables, String s) {
	String tmp = String_Clone(s);

	String_ReplaceAll(&tmp, String("#"), String("this->"));
	String_ReplaceAll(&tmp, String("$"), String(""));

	return tmp;
}

static def(void, HandlePrintVariable, MethodInstance method, String s, bool isInt) {
	String line = HeapString(s.len);
	String_Append(&line, String("String_Append(res, "));

	if (isInt) {
		String_Append(&line, String("Integer_ToString("));
	}

	String var = call(FormatVariables, s);
	String_Append(&line, var);
	String_Destroy(&var);

	if (isInt) {
		String_Append(&line, String(")"));
	}

	String_Append(&line, String(");"));

	Method_AddLine(method, line);

	String_Destroy(&line);
}

static def(void, HandleTemplate, MethodInstance method, String s) {
	String var  = call(FormatVariables, s);
	String line = String_Format(String("%.render(%.context, res);"),
		var, var);

	Method_AddLine(method, line);

	String_Destroy(&line);
	String_Destroy(&var);
}

static def(void, HandleIf, MethodInstance method, String s) {
	String var  = call(FormatVariables, s);
	String line = String_Format(String("if (%) {"), var);

	Method_AddLine(method, line);
	Method_Indent(method);

	String_Destroy(&line);
	String_Destroy(&var);
}

static def(void, HandleIfEmpty, MethodInstance method, String s) {
	String var  = call(FormatVariables, s);
	String line = String_Format(String("if (%.len == 0) {"), var);

	Method_AddLine(method, line);
	Method_Indent(method);

	String_Destroy(&line);
	String_Destroy(&var);
}

static def(void, HandleElse, MethodInstance method, String s) {
	Method_Unindent(method);

	if (s.len == 0) {
		Method_AddLine(method, String("} else {"));
		Method_Indent(method);
	} else {
		String var  = call(FormatVariables, s);
		String line = String_Format(String("} else if (%) {"), var);

		Method_AddLine(method, line);
		Method_Indent(method);

		String_Destroy(&line);
		String_Destroy(&var);
	}
}

static def(void, HandleEnd, MethodInstance method) {
	Method_Unindent(method);
	Method_AddLine(method, String("}"));
}

static def(void, HandleBlock, MethodInstance method, String params) {
	ssize_t offset = String_Find(params, ' ');

	String line;

	if (offset == String_NotFound) {
		line = String_Format(String("%(res);"), params);
	} else {
		String var = call(FormatVariables,
			String_Slice(params, offset + 1));

		line = String_Format(String("%(%, res);"),
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
		Logger_Error(&logger, String("Incomplete for-loop."));
		throw(&exc, excParsingFailed);
	}

	String iter   = call(FormatVariables, parts->buf[0]);
	String option = parts->buf[1];
	String from   = parts->buf[2];

	if (!String_Equals(option, String("in"))) {
		Logger_Error(&logger, String("For loops don't support '%'"),
			option);

		throw(&exc, excParsingFailed);
	}

	ssize_t pos = String_Find(from, 0, String(".."));

	bool isRange = (pos != String_NotFound);

	if (isRange) {
		String lower = String_Slice(from, 0, pos);
		String upper = String_Slice(from, pos + 2);

		String line = String_Format(
			String("for (int % = %; i <= %; i++) {"),
				iter, lower, upper);

		Method_AddLine(method, line);
		Method_Indent(method);

		String_Destroy(&line);
	} else {
		String var = call(FormatVariables, from);

		String line1 = String_Format(
			String("for (size_t i = 0; i < %->len; i++) {"),
			var);

		String line2 = String_Format(
			String("typeof(%->buf[0]) % = %->buf[i];"),
			var, iter, var);

		Method_AddLine(method, line1);
		Method_Indent(method);

		Method_AddLine(method, line2);

		String_Destroy(&line2);
		String_Destroy(&line1);
		String_Destroy(&var);
	}

	String_Destroy(&iter);
	Array_Destroy(parts);
}

static def(void, HandlePass, MethodInstance method, String params) {
	String fmt;
	ssize_t pos = String_Find(params, ' ');

	if (pos == String_NotFound) {
		fmt = String_Format(String(
			"{ "
				"String tmp = %(); "
				"String_Append(res, tmp); "
				"String_Destroy(&tmp); "
			"}"),

			params);
	} else {
		String var = call(FormatVariables, String_Slice(params, pos + 1));

		fmt = String_Format(String(
			"{ "
				"String tmp = %(%); "
				"String_Append(res, tmp); "
				"String_Destroy(&tmp); "
			"}"),

			String_Slice(params, 0, pos),
			var);

		String_Destroy(&var);
	}

	Method_AddLine(method, fmt);

	String_Destroy(&fmt);
}

static def(void, HandleCommand, MethodInstance method, String name, String params) {
	if (name.buf[0] == '$' || name.buf[0] == '#') {
		call(HandlePrintVariable, method, name, false);
	} else if (String_Equals(name, String("for"))) {
		call(HandleFor, method, params);
	} else if (String_Equals(name, String("if"))) {
		call(HandleIf, method, params);
	} else if (String_Equals(name, String("empty"))) {
		call(HandleIfEmpty, method, params);
	} else if (String_Equals(name, String("else"))) {
		call(HandleElse, method, params);
	} else if (String_Equals(name, String("end"))) {
		call(HandleEnd, method);
	} else if (String_Equals(name, String("block"))) {
		call(HandleBlock, method, params);
	} else if (String_Equals(name, String("int"))) {
		call(HandlePrintVariable, method, params, true);
	} else if (String_Equals(name, String("tpl"))) {
		call(HandleTemplate, method, params);
	} else if (String_Equals(name, String("pass"))) {
		call(HandlePass, method, params);
	} else {
		Logger_Error(&logger, String("Command '%' is unknown."),
			name);

		throw(&exc, excParsingFailed);
	}
}

static def(String, EscapeLine, String s) {
	String res = HeapString(s.len + 15);

	for (size_t i = 0; i < s.len; i++) {
		if (s.buf[i] != '"') {
			String_Append(&res, s.buf[i]);
		} else {
			String_Append(&res, String("\\\""));
		}
	}

	return res;
}

static def(void, FlushBuf, MethodInstance method, String s) {
	bool flushed = false;

	StringArray *items = String_Split(s, '\n');

	for (size_t i = 0; i < items->len; i++) {
		String line = items->buf[i];

		if (!flushed) {
			Method_AddLine(method, String("String_Append(res, String("));
			Method_Indent(method);
			flushed = true;
		}

		String escaped = call(EscapeLine, line);

		String_Prepend(&escaped, '"');

		if (items->len > 1) {
			if (i + 1 != items->len) {
				String_Append(&escaped, String("\\n"));
			}
		}

		String_Append(&escaped, '"');

		Method_AddLine(method, escaped);

		String_Destroy(&escaped);
	}

	if (flushed) {
		Method_Unindent(method);
		Method_AddLine(method, String("));"));
	}

	Array_Destroy(items);
}

static def(MethodInstance, NewMethod, String name) {
	MethodInstance method = Method_New();

	Method_Init(method, name, this->itf);

	Method_Item *item = New(Method_Item);

	item->method = method;

	DoublyLinkedList_InsertEnd(&this->methods, item);

	return method;
}

static def(MethodInstance, NewBlockMethod, String name, String params) {
	MethodInstance method = Method_New();

	Method_Init(method, name, true);

	Method_SetBlock(method, true);
	Method_SetParameters(method, params);

	Method_Item *item = New(Method_Item);

	item->method = method;

	DoublyLinkedList_InsertEnd(&this->methods, item);

	return method;
}

static def(bool, StartsCommandBlock, String cmd) {
	return String_Equals(cmd, String("if"))
		|| String_Equals(cmd, String("for"))
		|| String_Equals(cmd, String("else"))
		|| String_Equals(cmd, String("empty"));
}

static def(bool, EndsCommandBlock, String cmd) {
	return String_Equals(cmd, String("end"))
		|| String_Equals(cmd, String("else"));
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
			  || next.state == Parser_State_Block)
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
					if (String_Equals(cur.block, String("end"))) {
						goto out;
					}

					Logger_Error(&logger,
						String("'%' not understood."),
						cur.block);

					throw(&exc, excParsingFailed);
				}

				ssize_t pos = String_Find(cur.block, String(": "));

				String blkname   = cur.block;
				String blkparams = String("");

				if (pos != String_NotFound) {
					blkname   = String_Slice(cur.block, 0, pos);
					blkparams = String_Slice(cur.block, pos + 2);
					blkparams = call(FormatVariables, blkparams);
				}

				MethodInstance blockMethod =
					call(NewBlockMethod, blkname, blkparams);
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

		insert.file = String_Format(String("%/%"), this->dir, item.name);

		Array_Push(this->files, insert);
	}

	Directory_Destroy(&dir);
}

def(void, Process) {
	if (this->out.len == 0) {
		Logger_Error(&logger, String("No output path is set."));
		throw(&exc, excInvalidParameter);
	}

	if (this->dir.len > 0) {
		call(Scan);
	}

	Output output;
	Output_Init(&output, this->out, this->itf);
	Output_SetClassName(&output, this->name);

	for (size_t i = 0; i < this->files->len; i++) {
		Logger_Info(&logger, String("Processing %..."),
			this->files->buf[i].file);

		FileStream tplFile;
		FileStream_Open(&tplFile, this->files->buf[i].file,
			FileStatus_ReadOnly);

		BufferedStream stream;
		BufferedStream_Init(&stream, &FileStream_Methods, &tplFile);
		BufferedStream_SetInputBuffer(&stream, 4096, 256);

		Parser parser;
		Parser_Init(&parser, &BufferedStream_Methods, &stream);

		MethodInstance method = call(NewMethod, this->files->buf[i].name);
		call(ParseTemplate, &parser, false, method);

		BufferedStream_Close(&stream);
		BufferedStream_Destroy(&stream);
	}

	Output_Write(&output, &this->methods);
	Output_Destroy(&output);
}
