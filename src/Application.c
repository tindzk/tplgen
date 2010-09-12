#import "Application.h"

extern Logger logger;
extern ExceptionManager exc;

size_t Modules_Application;

void Application0(void) {
	Modules_Application = Module_Register(String("Application"));
}

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
		Memory_Free(item->method);

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
			Logger_Log(&logger, Logger_Level_Error,
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

static String ref(FormatVariables)(String s) {
	String tmp = String_Clone(s);

	String_ReplaceAll(&tmp, String("#"), String("this->"));
	String_ReplaceAll(&tmp, String("$"), String(""));

	return tmp;
}

static void ref(HandlePrintVariable)(Method *method, String s, bool isInt) {
	String line = HeapString(s.len);
	String_Append(&line, String("String_Append(res, "));

	if (isInt) {
		String_Append(&line, String("Integer_ToString("));
	}

	String var = ref(FormatVariables)(s);
	String_Append(&line, var);
	String_Destroy(&var);

	if (isInt) {
		String_Append(&line, String(")"));
	}

	String_Append(&line, String(");"));

	Method_AddLine(method, line);

	String_Destroy(&line);
}

static void ref(HandleTemplate)(Method *method, String s) {
	String var  = ref(FormatVariables)(s);
	String line = String_Format(String("%.render(%.context, res);"),
		var, var);

	Method_AddLine(method, line);

	String_Destroy(&line);
	String_Destroy(&var);
}

static void ref(HandleIf)(Method *method, String s) {
	String var  = ref(FormatVariables)(s);
	String line = String_Format(String("if (%) {"), var);

	Method_AddLine(method, line);
	Method_Indent(method);

	String_Destroy(&line);
	String_Destroy(&var);
}

static void ref(HandleIfEmpty)(Method *method, String s) {
	String var  = ref(FormatVariables)(s);
	String line = String_Format(String("if (%.len == 0) {"), var);

	Method_AddLine(method, line);
	Method_Indent(method);

	String_Destroy(&line);
	String_Destroy(&var);
}

static void ref(HandleElse)(Method *method, String s) {
	if (s.len == 0) {
		Method_Unindent(method);
		Method_AddLine(method, String("} else {"));
		Method_Indent(method);
	} else {
		String var  = ref(FormatVariables)(s);
		String line = String_Format(String("} else if (%) {"), var);

		Method_AddLine(method, line);
		Method_Indent(method);

		String_Destroy(&line);
		String_Destroy(&var);
	}
}

static void ref(HandleEnd)(Method *method) {
	Method_Unindent(method);
	Method_AddLine(method, String("}"));
}

static void ref(HandleBlock)(Method *method, String params) {
	ssize_t offset = String_Find(params, ' ');

	String line;

	if (offset == String_NotFound) {
		line = String_Format(String("%(res);"), params);
	} else {
		String var = ref(FormatVariables)(
			String_Slice(params, offset + 1));

		line = String_Format(String("%(%, res);"),
			String_Slice(params, 0, offset),
			var);

		String_Destroy(&var);
	}

	Method_AddLine(method, line);

	String_Destroy(&line);
}

static void ref(HandleFor)(Method *method, String params) {
	StringArray *parts = String_Split(params, ' ');

	if (parts->len < 3) {
		Logger_Log(&logger, Logger_Level_Error,
			String("Incomplete for-loop."));

		throw(&exc, excParsingFailed);
	}

	String iter   = ref(FormatVariables)(parts->buf[0]);
	String option = parts->buf[1];
	String from   = parts->buf[2];

	if (!String_Equals(option, String("in"))) {
		Logger_LogFmt(&logger, Logger_Level_Error,
			String("For loops don't support '%'"),
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
		String var = ref(FormatVariables)(from);

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

static void ref(HandlePass)(Method *method, String params) {
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
		String var = ref(FormatVariables)(String_Slice(params, pos + 1));

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

static void ref(HandleCommand)(Method *method, String s) {
	if (s.buf[0] == '$' || s.buf[0] == '#') {
		ref(HandlePrintVariable)(method, s, false);
	} else {
		ssize_t pos = String_Find(s, ' ');

		String cmd, params;

		if (pos != String_NotFound) {
			cmd    = String_Slice(s, 0, pos);
			params = String_Slice(s, pos + 1);
		} else {
			cmd    = String_Slice(s, 0);
			params = String("");
		}

		if (String_Equals(cmd, String("for"))) {
			ref(HandleFor)(method, params);
		} else if (String_Equals(cmd, String("if"))) {
			ref(HandleIf)(method, params);
		} else if (String_Equals(cmd, String("empty"))) {
			ref(HandleIfEmpty)(method, params);
		} else if (String_Equals(cmd, String("else"))) {
			ref(HandleElse)(method, params);
		} else if (String_Equals(cmd, String("end"))) {
			ref(HandleEnd)(method);
		} else if (String_Equals(cmd, String("block"))) {
			ref(HandleBlock)(method, params);
		} else if (String_Equals(cmd, String("int"))) {
			ref(HandlePrintVariable)(method, params, true);
		} else if (String_Equals(cmd, String("tpl"))) {
			ref(HandleTemplate)(method, params);
		} else if (String_Equals(cmd, String("pass"))) {
			ref(HandlePass)(method, params);
		} else {
			Logger_LogFmt(&logger, Logger_Level_Error,
				String("Command '%' is unknown."), cmd);

			throw(&exc, excParsingFailed);
		}
	}
}

static String ref(EscapeLine)(String s) {
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

static void ref(FlushBuf)(Method *method, String s) {
	size_t pos = 0;
	bool flushed = false;

	for (size_t i = 0; i < s.len; i++) {
		if (s.buf[i] == '\n' || i == s.len - 1) {
			if (pos == 0 || i - pos != 0) {
				String orig;

				if (i != s.len - 1) {
					orig = String_Slice(s, pos, i - pos);
				} else {
					orig = String_Slice(s, pos);
				}

				String trimmed = String_Trim(orig);

				String line;

				if (Char_IsSpace(orig.buf[0])) { /* Pad the beginning */
					String tmp = HeapString(trimmed.len + 1);

					/* TODO Make output non-HTML friendly. */
					if (orig.buf[0] != '\n') {
						String_Append(&tmp, orig.buf[0]);
					}

					String_Append(&tmp, trimmed);

					line = tmp;
				} else {
					line = String_Clone(trimmed);
				}

				if (i > 1 && s.buf[i] == '\n' && s.buf[i - 1] != '\n') {
					String_Append(&line, String("\\n"));
				} else if (s.buf[i] == ' ') {
					String_Append(&line, ' ');
				} else if (s.buf[pos] == ' ') {
					String_Prepend(&line, String(" "));
				}

				if (line.len > 0) {
					if (!flushed) {
						Method_AddLine(method, String("String_Append(res, String("));
						Method_Indent(method);
						flushed = true;
					}

					String escaped = ref(EscapeLine)(line);

					String tmp = StackString(escaped.len + 3);
					String_Append(&tmp, '"');
					String_Append(&tmp, escaped);
					String_Append(&tmp, '"');

					Method_AddLine(method, tmp);

					String_Destroy(&escaped);
				}

			out:
				String_Destroy(&line);
			}

			pos = i;
		}
	}

	if (flushed) {
		Method_Unindent(method);
		Method_AddLine(method, String("));"));
	}
}

static def(Method *, NewMethod, String name) {
	Method *method = New(Method);

	Method_Init(method, name, this->itf);

	Method_Item *item = New(Method_Item);

	item->method = method;

	DoublyLinkedList_InsertEnd(&this->methods, item);

	return method;
}

static def(Method *, NewBlockMethod, String name, String params) {
	Method *method = New(Method);

	Method_Init(method, name, true);

	Method_SetBlock(method, true);
	Method_SetParameters(method, params);

	Method_Item *item = New(Method_Item);

	item->method = method;

	DoublyLinkedList_InsertEnd(&this->methods, item);

	return method;
}

static def(void, ParseTemplate, StreamInterface *stream, void *context, bool inBlock, Method *method) {
	String blkbuf = HeapString(256);
	String cmdbuf = HeapString(512);
	String txtbuf = HeapString(4096);

	char cur  = '\0';
	char prev = '\0';

	enum state_t { NONE, COMMAND, BLOCK };
	enum state_t state = NONE;

	while (!stream->isEof(context)) {
		stream->read(context, &cur, 1);

		switch (state) {
			case NONE:
				if (cur == '{' || cur == '[' && prev != '\\') {
					ref(FlushBuf)(method, txtbuf);
					txtbuf.len = 0;

					state = (cur == '[')
						? BLOCK
						: COMMAND;
				} else {
					String_Append(&txtbuf, cur);
				}

				break;

			case BLOCK:
				if (cur == ']') {
					if (inBlock) {
						if (String_Equals(String("end"), blkbuf)) {
							goto out;
						} else {
							Logger_Log(&logger, Logger_Level_Error,
								String("Only 'end' tags within blocks are allowed."));

							throw(&exc, excParsingFailed);
						}
					}

					ssize_t pos = String_Find(blkbuf, String(": "));

					String blkname;
					String blkparams;

					if (pos == String_NotFound) {
						blkname   = blkbuf;
						blkparams = String("");
					} else {
						blkname = String_Slice(blkbuf, 0, pos);

						blkparams = ref(FormatVariables)(
							String_Slice(blkbuf, pos + 2));
					}

					ref(ParseTemplate)(this, stream, context, true,
						ref(NewBlockMethod)(this, blkname, blkparams));

					String_Destroy(&blkparams);

					blkbuf.len = 0;

					state = NONE;
				} else {
					String_Append(&blkbuf, cur);
				}

				break;

			case COMMAND:
				if (cur == '}') {
					ref(HandleCommand)(method, cmdbuf);
					cmdbuf.len = 0;

					state = NONE;
				} else {
					String_Append(&cmdbuf, cur);
				}

				break;
		}

		prev = cur;
	}

out:
	if (txtbuf.len > 0) {
		ref(FlushBuf)(method, txtbuf);
	}

	String_Destroy(&txtbuf);
	String_Destroy(&cmdbuf);
	String_Destroy(&blkbuf);
}

def(void, Scan) {
	Directory dir;
	Directory_Entry item;
	Directory_Init(&dir, this->dir);

	while (Directory_Read(&dir, &item)) {
		if (item.type != DT_LNK
		 && item.type != DT_REG) {
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
		Logger_Log(&logger, Logger_Level_Error,
			String("No output path is set."));

		throw(&exc, excInvalidParameter);
	}

	if (this->dir.len > 0) {
		ref(Scan)(this);
	}

	Output output;
	Output_Init(&output, this->out, this->itf);
	Output_SetClassName(&output, this->name);

	for (size_t i = 0; i < this->files->len; i++) {
		Logger_LogFmt(&logger, Logger_Level_Info,
			String("Processing %..."),
			this->files->buf[i].file);

		FileStream tplFile;
		FileStream_Open(&tplFile, this->files->buf[i].file,
			FileStatus_ReadOnly);

		BufferedStream stream;
		BufferedStream_Init(&stream, &FileStream_Methods, &tplFile);
		BufferedStream_SetInputBuffer(&stream, 4096, 256);

		ref(ParseTemplate)(this, &BufferedStream_Methods, &stream, false,
			ref(NewMethod)(this, this->files->buf[i].name));

		BufferedStream_Close(&stream);
		BufferedStream_Destroy(&stream);
	}

	Output_Write(&output, &this->methods);

	Output_Destroy(&output);
}
