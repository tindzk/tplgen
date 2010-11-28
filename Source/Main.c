#import <String.h>
#import <Logger.h>
#import <Integer.h>
#import <Exception.h>

#import "Application.h"

Logger logger;

void OnLogMessage(__unused void *ptr, String msg, Logger_Level level, String file, int line) {
	String slevel = Logger_ResolveLevel(level);
	String sline  = Integer_ToString(line);

	String_FmtPrint($("[%] % (%:%)\n"), slevel, msg, file, sline);
}

int main(int argc, char **argv) {
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
	} tryEnd;

	return ExitStatus_Success;
}
