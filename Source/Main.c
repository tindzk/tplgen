#import <Main.h>
#import <String.h>
#import <Logger.h>
#import <Integer.h>
#import <Terminal.h>
#import <Exception.h>

#import "App.h"

#define self Application

def(bool, Run) {
	if (this->args->len == 0) {
		Logger_Error(&this->logger, $("No parameters specified."));
		return false;
	}

	App app = App_New(&this->logger);

	fwd(i, this->args->len) {
		RdString name, value;

		if (String_Parse($("%=%"), this->args->buf[i], &name, &value)) {
			if (!setOption(&app, name, value)) {
				return false;
			}
		}
	}

	try {
		process(&app);
	} finally {
		destroy(&app);
	} tryEnd;

	return true;
}
