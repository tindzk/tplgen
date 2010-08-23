#import <String.h>
#import <Logger.h>
#import <Exception.h>

#import "Application.h"

Logger logger;
ExceptionManager exc;

void OnLogMessage(__unused void *ptr, String msg, Logger_Level level, String file, int line) {
	String slevel = Logger_LevelToString(level);
	String sline  = Integer_ToString(line);

	String_FmtPrint(String("[%] % (%:%)\n"),
		slevel, msg, file, sline);
}

int main(int argc, char **argv) {
	ExceptionManager_Init(&exc);

	File0(&exc);
	String0(&exc);
	Memory0(&exc);
	Directory0(&exc);

	Logger_Init(&logger, &OnLogMessage, NULL,
		Logger_Level_Fatal |
		Logger_Level_Crit  |
		Logger_Level_Error |
		Logger_Level_Warn  |
		Logger_Level_Info  |
		Logger_Level_Trace);

	Application app;
	Application_Init(&app);

	if (argc <= 1) {
		Logger_Log(&logger, Logger_Level_Error,
			String("No parameters specified."));

		return EXIT_FAILURE;
	}

	for (size_t i = 1; i < (size_t) argc; i++) {
		String arg = String_FromNul(argv[i]);

		ssize_t pos = String_Find(arg, '=');

		if (pos == String_NotFound) {
			continue;
		}

		String name  = String_Slice(arg, 0, pos);
		String value = String_Slice(arg, pos + 1);

		if (!Application_SetOption(&app, name, value)) {
			return EXIT_FAILURE;
		}
	}

	try(&exc) {
		Application_Process(&app);
	} catchAny(e) {
		Exception_Print(e);

#if Exception_SaveTrace
		Backtrace_PrintTrace(e->trace, e->traceItems);
#endif

		return EXIT_FAILURE;
	} finally {
		Application_Destroy(&app);
	} tryEnd;

	return EXIT_SUCCESS;
}
