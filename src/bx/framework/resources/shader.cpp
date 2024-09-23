#include "bx/framework/resources/shader.hpp"
#include "bx/framework/resources/shader.serial.hpp"

#include <bx/engine/core/file.hpp>
#include <bx/engine/core/guard.hpp>
#include <bx/engine/core/stream.hpp>
#include <bx/engine/containers/hash_set.hpp>
#include <bx/engine/modules/graphics.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cstring>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>


static String ResolveIncludes(const String& fileName, const String& source, HashSet<String>& includedFiles, List<ShaderIncludeRange>& includeRanges, u32 lineNumber)
{
    InputStringStream shaderStream(source);
    OutputStringStream resolvedShader;

    u32 startLineNumber = lineNumber;

    // Start include range
    includeRanges.push_back(ShaderIncludeRange{ fileName, startLineNumber, 0 });

    String line;
    while (shaderStream.GetLine(line))
    {
        SizeType includePos = line.find("#include");
        if (includePos != std::string::npos && includePos == 0)
        {
            // Extract include file path
            SizeType start = line.find("\"", includePos) + 1;
            SizeType end = line.find("\"", start);
            String includeFile = line.substr(start, end - start);

            // Prevent circular includes
            if (includedFiles.find(includeFile) != includedFiles.end())
            {
                throw Exception("Circular include detected: " + includeFile);
            }

            // End current include range to make room for include file
            for (u32 i = 0; i < includeRanges.size(); i++)
            {
                if (includeRanges[i].startLine == startLineNumber)
                {
                    includeRanges[i].endLine = lineNumber;

                    // Remove if range is empty
                    if (includeRanges[i].startLine == includeRanges[i].endLine)
                    {
                        includeRanges.erase(includeRanges.begin() + i);
                    }

                    break;
                }
            }

            includedFiles.insert(includeFile);
            String includedSource = ResolveIncludes(includeFile, File::ReadTextFile(includeFile), includedFiles, includeRanges, lineNumber);
            lineNumber += StringLineCount(includedSource);
            resolvedShader << includedSource;
            includedFiles.erase(includeFile);

            // Restart include range after inserting include src
            startLineNumber = lineNumber;
            includeRanges.push_back(ShaderIncludeRange{ fileName, startLineNumber, 0 });
        }
        else
        {
            resolvedShader << line << "\n";
            lineNumber++;
        }
    }

    for (u32 i = 0; i < includeRanges.size(); i++)
    {
        if (includeRanges[i].startLine == startLineNumber)
        {
            includeRanges[i].endLine = lineNumber;

            // Remove if range is empty
            if (includeRanges[i].startLine == includeRanges[i].endLine)
            {
                includeRanges.erase(includeRanges.begin() + i);
            }

            break;
        }
    }

    return resolvedShader.GetString();
}

ShaderSrc ResolveShaderIncludes(const String& source)
{
    HashSet<String> includedFiles{};
    List<ShaderIncludeRange> includeRanges{};
    String src = ResolveIncludes("Root", source, includedFiles, includeRanges, 0);

    ShaderSrc result;
    result.src = src;
    result.includeRanges = includeRanges;
    return result;
}

template<>
bool Resource<Shader>::Save(const String& filename, const Shader& data)
{
    std::ofstream stream(File::GetPath(filename));
    if (stream.fail())
        return false;

    cereal::JSONOutputArchive archive(stream);
    archive(data);

    return true;
}

template<>
bool Resource<Shader>::Load(const String& filename, Shader& data)
{
    std::ifstream stream(File::GetPath(filename));
    if (stream.fail() || !stream.good())
        return false;
    std::stringstream ss;
    ss << stream.rdbuf();

    String source = ResolveShaderIncludes(ss.str()).src;
    data.SetSource(source);

    ShaderCreateInfo vertexCreateInfo{};
    vertexCreateInfo.name = Log::Format("{} Vertex Shader", filename);
    vertexCreateInfo.shaderType = ShaderType::VERTEX;
    vertexCreateInfo.src = "#define VERTEX\n" + data.m_source; // TODO: remove this cardinal sin
    data.m_vertexShader = Graphics::CreateShader(vertexCreateInfo);

    ShaderCreateInfo fragmentCreateInfo{};
    fragmentCreateInfo.name = Log::Format("{} Fragment Shader", filename);
    fragmentCreateInfo.shaderType = ShaderType::FRAGMENT;
    fragmentCreateInfo.src = "#define PIXEL\n" + data.m_source;
    data.m_fragmentShader = Graphics::CreateShader(fragmentCreateInfo);

    return true;
}

template<>
void Resource<Shader>::Unload(Shader& data)
{
    Graphics::DestroyShader(data.m_vertexShader);
    Graphics::DestroyShader(data.m_fragmentShader);
}