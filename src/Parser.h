#import <String.h>
#import <StreamInterface.h>

#undef self
#define self Parser

set(ref(State)) {
	ref(State_None),
	ref(State_Text),
	ref(State_Command),
	ref(State_Block)
};

Class(ref(Token)) {
	ref(State) state;

	union {
		String text;

		struct {
			String name;
			String params;
		} cmd;

		String block;
	} u;
};

#define Parser_Token() \
	(Parser_Token) {   \
		.state = Parser_State_None \
	}

Class(self) {
	StreamInterface *stream;
	void *context;

	bool proceed;
	char proceedChar;
};

def(void, Init, StreamInterface *stream, void *context);
void ref(DestroyToken)(ref(Token) *token);
def(ref(Token), Fetch);
