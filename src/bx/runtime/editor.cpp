#include <bx/engine/application.hpp>

int main(int argc, char** argv)
{
//#ifdef PROJECT_PATH
//	File::Initialize(config.argv[1]);
//#else
//	BX_ENSURE(config.argc >= 2);
//	File::Initialize(config.argv[1]);
//#endif

	return Application::Launch(AppConfig{ argc, argv });
}