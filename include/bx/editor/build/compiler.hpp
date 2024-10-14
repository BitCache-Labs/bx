#pragma once

#include <string>

class Compiler
{
public:
    Compiler(const std::string& projectPath);

    bool Configure() const;
    bool Build() const;

private:
    std::string m_projectPath;
};