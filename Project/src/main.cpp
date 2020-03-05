#include "Common/Debug.h"
#include "Core/Application.h"

int main(int argc, const char* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);

#if defined(_DEBUG) && defined(_WIN32)
	_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	Application app;
	app.init();
	app.run();
	app.release();
	return 0;
}
