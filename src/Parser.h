#import <String.h>
#import <StreamInterface.h>

#undef self
#define self Parser

set {
	ref(State_None),
	ref(State_Text),
	ref(State_Command),
	ref(State_Block)
} ref(State);

record {
	ref(State) state;

	union {
		String text;

		struct {
			String name;
			String params;
		} cmd;

		String block;
	} u;
} ref(Token);

#define Parser_Token() \
	(Parser_Token) {   \
		.state = Parser_State_None \
	}

record {
	StreamInterface *stream;
	void *context;

	bool proceed;
	char proceedChar;
} Class(self);

def(void, Init, StreamInterface *stream, void *context);
void ref(DestroyToken)(ref(Token) *token);
def(ref(Token), Fetch);
