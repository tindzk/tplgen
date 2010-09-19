#import "Parser.h"

def(void, Init, StreamInterface *stream, void *context) {
	this->stream  = stream;
	this->context = context;

	this->proceed     = false;
	this->proceedChar = '\0';
}

void ref(DestroyToken)(ref(Token) *token) {
	switch (token->state) {
		case ref(State_Text):
			String_Destroy(&token->u.text);
			break;

		case ref(State_Command):
			String_Destroy(&token->u.cmd.name);
			String_Destroy(&token->u.cmd.params);
			break;

		case ref(State_Block):
			String_Destroy(&token->u.block);
			break;

		case ref(State_None):
			break;
	}

	token->state = ref(State_None);
}

static def(void, ParseCommand, ref(Token) *token) {
	char cur = '\0';

	String buf = HeapString(64);

	while (!this->stream->isEof(this->context)) {
		this->stream->read(this->context, &cur, 1);

		if (cur == '}') {
			break;
		}

		String_Append(&buf, cur);
	}

	ssize_t pos = String_Find(buf, ' ');

	if (pos != String_NotFound) {
		token->u.cmd.name   = String_Clone(String_Slice(buf, 0, pos));
		token->u.cmd.params = String_Clone(String_Slice(buf, pos + 1));
	} else {
		token->u.cmd.name   = String_Clone(String_Slice(buf, 0));
		token->u.cmd.params = String("");
	}

	String_Destroy(&buf);
}

static def(void, ParseBlock, ref(Token) *token) {
	char cur = '\0';

	token->u.block = HeapString(64);

	while (!this->stream->isEof(this->context)) {
		this->stream->read(this->context, &cur, 1);

		if (cur == ']') {
			break;
		}

		String_Append(&token->u.block, cur);
	}
}

def(ref(Token), Fetch) {
	char cur  = '\0';
	char prev = '\0';

	ref(Token) token;

	token.state = ref(State_None);

	if (this->proceed) {
		cur = this->proceedChar;
		this->proceed = false;

		goto next;
	}

	while (!this->stream->isEof(this->context)) {
		this->stream->read(this->context, &cur, 1);

		if (cur == '{' || cur == '[' && prev != '\\') {
			if (token.state != ref(State_None)) {
				this->proceed     = true;
				this->proceedChar = cur;

				break;
			}

		next:
			if (cur == '[') {
				token.state = ref(State_Block);
				ref(ParseBlock)(this, &token);
			} else {
				token.state = ref(State_Command);
				ref(ParseCommand)(this, &token);
			}

			return token;
		} else {
			if (token.state == ref(State_None)) {
				token.state  = ref(State_Text);
				token.u.text = HeapString(128);
			}

			String_Append(&token.u.text, cur);
		}

		prev = cur;
	}

	return token;
}
