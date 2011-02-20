#import <String.h>
#import <Logger.h>
#import <Integer.h>
#import <Terminal.h>
#import <Exception.h>

#import "Application.h"

Logger logger;
Terminal term;

void OnLogMessage(__unused void *ptr, FmtString msg, Logger_Level level, String file, int line) {
	String slevel = Logger_ResolveLevel(level);
	String sline  = Integer_ToString(line);

	Terminal_FmtPrint(&term,
		$("[%] $ (%:%)\n"),
		slevel, msg, file, sline);

	String_Destroy(&sline);
}

int main(int argc, char **argv) {
	Terminal_Init(&term, File_StdIn, File_StdOut, false);

	Logger_Init(&logger, Callback(NULL, &OnLogMessage),
		Logger_Level_Fatal |
		Logger_Level_Crit  |
		Logger_Level_Error |
		Logger_Level_Warn  |
		Logger_Level_Info  |
		Logger_Level_Trace);

	if (argc <= 1) {
		Logger_Error(&logger, $("No parameters specified."));
		return ExitStatus_Failure;
	}

	Application app;
	Application_Init(&app);

	for (size_t i = 1; i < (size_t) argc; i++) {
		String arg = String_FromNul(argv[i]);

		ssize_t pos = String_Find(arg, '=');

		if (pos == String_NotFound) {
			continue;
		}

		String name  = String_Slice(arg, 0, pos);
		String value = String_Slice(arg, pos + 1);

		if (!Application_SetOption(&app, name, value)) {
			return ExitStatus_Failure;
		}
	}

	try {
		Application_Process(&app);
	} clean catchAny {
		Exception_Print(e);

#if Exception_SaveTrace
		Backtrace_PrintTrace(__exc_mgr.e.trace, __exc_mgr.e.traceItems);
#endif

		excReturn ExitStatus_Failure;
	} finally {
		Application_Destroy(&app);
		Terminal_Destroy(&term);
	} tryEnd;

	return ExitStatus_Success;
}
