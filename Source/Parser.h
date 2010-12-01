#import <String.h>
#import <StreamInterface.h>

#define self Parser

set(ref(State)) {
	ref(State_None),
	ref(State_Text),
	ref(State_Command),
	ref(State_Block)
};

record(ref(Token)) {
	ref(State) state;

	union {
		String text;

		struct {
			String name;
			String params;
		} cmd;

		String block;
	};
};

#define Parser_Token() \
	(Parser_Token) {   \
		.state = Parser_State_None \
	}

class {
	StreamInterface *stream;
	void *context;

	bool proceed;
	char proceedChar;
};

def(void, Init, StreamInterface *stream, void *context);
void ref(DestroyToken)(ref(Token) *token);
def(ref(Token), Fetch);

#undef self
