#include <bx/editor/build/compiler.hpp>

#include <cstdlib>
#include <iostream>

Compiler::Compiler(const std::string& projectPath)
    : m_projectPath(projectPath) {}

bool Compiler::Configure() const
{
    std::string configureCommand = "cmake -S " + m_projectPath + " -B " + m_projectPath + "/build";
    int result = std::system(configureCommand.c_str());

    if (result != 0)
    {
        std::cerr << "Error: Failed to configure the project at " << m_projectPath << std::endl;
        return false;
    }

    std::cout << "Project configured successfully at " << m_projectPath << std::endl;
    return true;
}

bool Compiler::Build() const
{
    std::string buildCommand = "cmake --build " + m_projectPath + "/build --target game";
    int result = std::system(buildCommand.c_str());

    if (result != 0)
    {
        std::cerr << "Error: Failed to build project at " << m_projectPath << std::endl;
        return false;
    }

    std::cout << "Project built successfully at " << m_projectPath << std::endl;
    return true;
}