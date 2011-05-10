#import "App.h"

#define self App

rsdef(self, New, Logger *logger) {
	return (self) {
		.itf     = true,
		.dir     = String_New(0),
		.ext     = String_New(0),
		.out     = String_New(0),
		.name    = String_New(0),
		.logger  = logger,
		.files   = scall(TemplateArray_New, 50),
		.methods = DoublyLinkedList_New()
	};
}

def(void, destroyItem, Instance inst) {
	Method_Item *item = inst.addr;
	Method_Destroy(item->method);
}

odef(void, destroy) {
	String_Destroy(&this->dir);
	String_Destroy(&this->ext);
	String_Destroy(&this->out);
	String_Destroy(&this->name);

	DoublyLinkedList_Destroy(&this->methods,
		LinkedList_OnDestroy_For(this, ref(destroyItem)));

	each(file, this->files) {
		String_Destroy(&file->name);
		String_Destroy(&file->file);
	}

	scall(TemplateArray_Free, this->files);
}

odef(bool, setOption, RdString name, RdString value) {
	if (String_Equals(name, $("name"))) {
		String_Copy(&this->name, value);
	} else if (String_Equals(name, $("itf"))) {
		this->itf = String_Equals(value, $("yes"));
	} else if (String_Equals(name, $("add"))) {
		RdStringArray *parts = String_Split(value, ':');

		if (parts->len < 2) {
			Logger_Error(this->logger,
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

static odef(String, formatVariables, RdString s) {
	String tmp = String_Clone(s);

	String_ReplaceAll(&tmp, $("#"), $("tpl->"));
	String_ReplaceAll(&tmp, $("$"), $(""));

	return tmp;
}

static odef(void, handlePrintVariable, Method *method, RdString s, RdString s2) {
	String var  = formatVariables(this, s);
	String var2 = formatVariables(this, s2);

	Method_AddLine(method,
		String_Format($("Template_Print(%%, res);"),
			var.rd, var2.rd));

	String_Destroy(&var2);
	String_Destroy(&var);
}

static odef(void, handleTemplate, Method *method, RdString s) {
	String var = formatVariables(this, s);

	Method_AddLine(method,
		String_Format($("callback(%, res);"), var.rd));

	String_Destroy(&var);
}

static odef(void, handleIf, Method *method, RdString s) {
	String var = formatVariables(this, s);

	Method_AddLine(method,
		String_Format($("if (%) {"), var.rd));
	Method_Indent(method);

	String_Destroy(&var);
}

static odef(void, handleIfEmpty, Method *method, RdString s) {
	String var = formatVariables(this, s);

	Method_AddLine(method,
		String_Format($("if (%.len == 0) {"), var.rd));
	Method_Indent(method);

	String_Destroy(&var);
}

static odef(void, handleElse, Method *method, RdString s) {
	Method_Unindent(method);

	if (s.len == 0) {
		Method_AddLine(method, $$("} else {"));
		Method_Indent(method);
	} else {
		String var = formatVariables(this, s);

		Method_AddLine(method,
			String_Format($("} else if (%) {"), var.rd));
		Method_Indent(method);

		String_Destroy(&var);
	}
}

static odef(void, handleEnd, Method *method) {
	Method_Unindent(method);
	Method_AddLine(method, $$("}"));
}

static odef(void, handleBlock, Method *method, RdString params) {
	ssize_t offset = String_Find(params, ' ');

	String line;

	if (offset == String_NotFound) {
		line = String_Format($("%(res);"), params);
	} else {
		String var = formatVariables(this,
			String_Slice(params, offset + 1));

		line = String_Format($("%(%, res);"),
			String_Slice(params, 0, offset),
			var.rd);

		String_Destroy(&var);
	}

	Method_AddLine(method, line);
}

static odef(void, handleFor, Method *method, RdString params) {
	RdStringArray *parts = String_Split(params, ' ');

	if (parts->len < 3) {
		Logger_Error(this->logger, $("Incomplete for-loop."));
		throw(ParsingFailed);
	}

	String iter = formatVariables(this, parts->buf[0]);
	RdString option = parts->buf[1];
	String from = formatVariables(this, parts->buf[2]);

	if (!String_Equals(option, $("in"))) {
		Logger_Error(this->logger, $("For loops don't support '%'"),
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

static odef(void, handlePass, Method *method, RdString name, RdString params) {
	String vars = formatVariables(this, params);

	Method_AddLine(method,
		String_Format($("Template_Print(%(%), res);"),
			String_Slice(name, 1),
			vars.rd));

	String_Destroy(&vars);
}

static odef(void, handleCommand, Method *method, RdString name, RdString params) {
	if (name.len == 0) {
		return;
	}

	if (name.buf[0] == '$' || name.buf[0] == '#') {
		handlePrintVariable(this, method, name, params);
	} else if (name.buf[0] == '~') {
		handlePass(this, method, name, params);
	} else if (String_Equals(name, $("for"))) {
		handleFor(this, method, params);
	} else if (String_Equals(name, $("if"))) {
		handleIf(this, method, params);
	} else if (String_Equals(name, $("empty"))) {
		handleIfEmpty(this, method, params);
	} else if (String_Equals(name, $("else"))) {
		handleElse(this, method, params);
	} else if (String_Equals(name, $("end"))) {
		handleEnd(this, method);
	} else if (String_Equals(name, $("block"))) {
		handleBlock(this, method, params);
	} else if (String_Equals(name, $("tpl"))) {
		handleTemplate(this, method, params);
	} else {
		Logger_Error(this->logger, $("Command '%' is unknown."), name);
		throw(ParsingFailed);
	}
}

static odef(String, escapeLine, RdString s) {
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

static odef(void, flushBuf, Method *method, RdString s) {
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

		String escaped = escapeLine(this, line);

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

static odef(Method *, newMethod, String name) {
	Method *method = Method_New(name, this->itf);

	Method_Item *item = Method_Item_New(method);

	DoublyLinkedList_InsertEnd(&this->methods, item);

	return method;
}

static odef(Method *, newBlockMethod, String name, String params, bool public) {
	Method *method = Method_New(name, !public);
	Method_SetBlock(method, true);
	Method_SetParameters(method, params);

	Method_Item *item = Method_Item_New(method);

	DoublyLinkedList_InsertEnd(&this->methods, item);

	return method;
}

static odef(bool, startsCommandBlock, RdString cmd) {
	return String_Equals(cmd, $("if"))
		|| String_Equals(cmd, $("for"))
		|| String_Equals(cmd, $("else"))
		|| String_Equals(cmd, $("empty"));
}

static odef(bool, endsCommandBlock, RdString cmd) {
	return String_Equals(cmd, $("end"))
		|| String_Equals(cmd, $("else"));
}

static odef(void, parseTemplate, Parser *parser, bool inBlock, Method *method) {
	Parser_Token prev = Parser_Token();
	Parser_Token cur  = Parser_Token();
	Parser_Token next = Parser_Token();

	bool first = inBlock;

	do {
		if (cur.state == Parser_State_Text) {
			RdString text = cur.text.rd;

			if ((prev.state == Parser_State_Command && startsCommandBlock(this, prev.cmd.name.rd))
			  || first)
			{
				text  = String_Trim(text, String_TrimLeft);
				first = false;
			}

			next = Parser_Fetch(parser);

			if ((next.state == Parser_State_Command && endsCommandBlock(this, next.cmd.name.rd))
			  || next.state == Parser_State_Block
			  || next.state == Parser_State_None)
			{
				text = String_Trim(text, String_TrimRight);
			}

			flushBuf(this, method, text);
		} else {
			if (cur.state == Parser_State_Command) {
				handleCommand(this, method,
					cur.cmd.name.rd,
					cur.cmd.params.rd);
			} else if (cur.state == Parser_State_Block) {
				if (inBlock) {
					if (String_Equals(cur.block.rd, $("end"))) {
						goto out;
					}

					Logger_Error(this->logger,
						$("'%' not understood."),
						cur.block.rd);

					throw(ParsingFailed);
				}

				ssize_t pos = String_Find(cur.block.rd, $(": "));

				RdString blkname = cur.block.rd;
				String blkparams = String_New(0);

				if (pos != String_NotFound) {
					blkname   = String_Slice(cur.block.rd, 0, pos);
					blkparams = formatVariables(this,
						String_Slice(cur.block.rd, pos + 2));
				}

				RdString tmp;
				bool public = String_BeginsWith(blkname, tmp = $("public "));

				if (public) {
					blkname = String_Slice(blkname, tmp.len);
				}

				Method *blockMethod =
					newBlockMethod(this, String_Clone(blkname), blkparams, public);
				parseTemplate(this, parser, true, blockMethod);
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

odef(void, scan) {
	Directory_Entry item;

	Directory dir = Directory_New(this->dir.rd);

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

odef(void, process) {
	if (this->out.len == 0) {
		Logger_Error(this->logger, $("No output path is set."));
		throw(InvalidParameter);
	}

	if (this->dir.len > 0) {
		scan(this);
	}

	Output output;
	init(&output, this->logger, this->out.rd, this->itf);
	setClassName(&output, String_Clone(this->name.rd));

	fwd(i, this->files->len) {
		Logger_Info(this->logger, $("Processing %..."),
			this->files->buf[i].file.rd);

		File tplFile = File_New(this->files->buf[i].file.rd, FileStatus_ReadOnly);

		BufferedStream stream = BufferedStream_New(File_AsStream(&tplFile));
		BufferedStream_SetInputBuffer(&stream, 4096, 256);

		Parser parser;
		Parser_Init(&parser, BufferedStream_AsStream(&stream));

		Method *method = newMethod(this,
			String_Clone(this->files->buf[i].name.rd));
		parseTemplate(this, &parser, false, method);

		BufferedStream_Close(&stream);
		BufferedStream_Destroy(&stream);
	}

	writeList(&output, &this->methods);
	destroy(&output);
}
