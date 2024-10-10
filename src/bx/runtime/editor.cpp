#include <bx/engine/application.hpp>

int main(int argc, char** argv)
{
	return Application::Launch(AppConfig{ argc, argv });
}