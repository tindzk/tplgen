#import "Parser.h"
#import <App.h>

def(void, Init, StreamInterface *stream, void *context) {
	this->stream  = stream;
	this->context = context;

	this->proceed     = false;
	this->proceedChar = '\0';
}

void ref(DestroyToken)(ref(Token) *token) {
	switch (token->state) {
		case ref(State_Text):
			String_Destroy(&token->text);
			break;

		case ref(State_Command):
			String_Destroy(&token->cmd.name);
			String_Destroy(&token->cmd.params);
			break;

		case ref(State_Block):
			String_Destroy(&token->block);
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
		token->cmd.name   = String_Clone(String_Slice(buf, 0, pos));
		token->cmd.params = String_Clone(String_Slice(buf, pos + 1));
	} else {
		token->cmd.name   = String_Clone(String_Slice(buf, 0));
		token->cmd.params = HeapString(0);
	}

	String_Destroy(&buf);
}

static def(void, ParseBlock, ref(Token) *token) {
	char cur = '\0';

	token->block = HeapString(64);

	while (!this->stream->isEof(this->context)) {
		this->stream->read(this->context, &cur, 1);

		if (cur == ']') {
			break;
		}

		String_Append(&token->block, cur);
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
				call(ParseBlock, &token);
			} else {
				token.state = ref(State_Command);
				call(ParseCommand, &token);
			}

			return token;
		} else {
			if (token.state == ref(State_None)) {
				token.state = ref(State_Text);
				token.text  = HeapString(128);
			}

			String_Append(&token.text, cur);
		}

		prev = cur;
	}

	return token;
}
