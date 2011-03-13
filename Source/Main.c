#import <String.h>
#import <Logger.h>
#import <Integer.h>
#import <Terminal.h>
#import <Exception.h>

#import "Application.h"

Logger logger;
Terminal term;

void OnLogMessage(__unused void *ptr, FmtString msg, Logger_Level level, String file, int line) {
	RdString slevel = Logger_ResolveLevel(level);
	String sline = Integer_ToString(line);

	Terminal_FmtPrint(&term,
		$("[%] $ (%:%)\n"),
		slevel, msg, file, sline);

	String_Destroy(&sline);
}

int main(int argc, char **argv) {
	term   = Terminal_New(File_StdIn, File_StdOut, false);
	logger = Logger_New(Callback(NULL, &OnLogMessage));

	if (argc <= 1) {
		Logger_Error(&logger, $("No parameters specified."));
		return ExitStatus_Failure;
	}

	Application app = Application_New();

	for (size_t i = 1; i < (size_t) argc; i++) {
		RdString arg = String_FromNul(argv[i]);

		ssize_t pos = String_Find(arg, '=');

		if (pos == String_NotFound) {
			continue;
		}

		RdString name  = String_Slice(arg, 0, pos);
		RdString value = String_Slice(arg, pos + 1);

		if (!Application_SetOption(&app, name, value)) {
			return ExitStatus_Failure;
		}
	}

	try {
		Application_Process(&app);
	} catchAny {
		Exception_Print(e);
		excReturn ExitStatus_Failure;
	} finally {
		Application_Destroy(&app);
		Terminal_Destroy(&term);
	} tryEnd;

	return ExitStatus_Success;
}
