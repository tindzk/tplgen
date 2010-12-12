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
	Stream stream;

	bool proceed;
	char proceedChar;
};

def(void, Init, Stream stream);
void ref(DestroyToken)(ref(Token) *token);
def(ref(Token), Fetch);

#undef self
