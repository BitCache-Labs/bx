#include "bx/engine/modules/graphics/backend/vulkan/spirv_compiler.hpp"

#include "bx/engine/core/macros.hpp"

//#include <glslang/Include/glslang_c_interface.h>
//#include <glslang/Public/resource_limits_c.h>
//#include <glslang/Public/ResourceLimits.h>

#include <glslang/Public/ShaderLang.h>
#include <glslang/Include/intermediate.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/MachineIndependent/localintermediate.h>

static const TBuiltInResource DefaultTBuiltInResource =
{
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxMeshOutputVerticesEXT = */ 256,
    /* .maxMeshOutputPrimitivesEXT = */ 256,
    /* .maxMeshWorkGroupSizeX_EXT = */ 128,
    /* .maxMeshWorkGroupSizeY_EXT = */ 128,
    /* .maxMeshWorkGroupSizeZ_EXT = */ 128,
    /* .maxTaskWorkGroupSizeX_EXT = */ 128,
    /* .maxTaskWorkGroupSizeY_EXT = */ 128,
    /* .maxTaskWorkGroupSizeZ_EXT = */ 128,
    /* .maxMeshViewCountEXT = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,

    /* .limits = */
    {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }
};

namespace Vk
{
    struct HBE_Includer : glslang::TShader::Includer
    {
        std::string path;
        std::set<IncludeResult*> results;

        HBE_Includer(const std::string& path)
        {
            this->path = path.substr(0, path.find_last_of("\\/") + 1);
        }

        IncludeResult* includeSystem(const char* file_path, const char* includer_name, size_t inclusion_depth) override {
            std::string* source = new std::string();
            source->erase(std::find(source->begin(), source->end(), '\0'), source->end());
            IncludeResult* result = new IncludeResult(path, source->c_str(), source->size() - 2, source);
            results.emplace(result);
            return result;
        }

        IncludeResult* includeLocal(const char* file_path, const char* includer_name, size_t inclusion_depth) override {
            std::string* source = new std::string();
            source->erase(std::find(source->begin(), source->end(), '\0'), source->end());
            IncludeResult* result = new IncludeResult(path + file_path, source->c_str(), source->size(), source);
            results.emplace(result);
            return result;
        }

        void releaseInclude(IncludeResult* result) override {
            if (result != NULL)
            {
                results.erase(result);
                delete (std::string*)result->userData;
                delete result;
            }
        }

        ~HBE_Includer() {
            for (auto r : results) {
                delete[] r->headerData;
                delete r;
            }
        }
    };


    SpirVCompiler& SpirVCompiler::Instance()
    {
        static SpirVCompiler instance{};
        return instance;
    }

    SpirVCompiler::SpirVCompiler()
    {
        glslang::InitializeProcess();
    }

    SpirVCompiler::~SpirVCompiler()
    {
        glslang::FinalizeProcess();
    }

    List<u32> SpirVCompiler::Compile(const String& name, EShLanguage stage, const String& src, const List<ShaderIncludeRange>& includeRanges)
    {
#ifdef _DEBUG
        bool debug = true;
#else
        bool debug = false;
#endif

        auto spirv_target = glslang::EShTargetSpv_1_4;

        glslang::TShader shader(stage);

        const char* source_str_ptr = src.c_str();
        const char* const* source_ptr = &source_str_ptr;
        int lenght = src.size();
        shader.setEnvClient(glslang::EShClient::EShClientVulkan, glslang::EShTargetVulkan_1_3);
        shader.setEnvTarget(glslang::EShTargetSpv, spirv_target);
        shader.setStringsWithLengths(source_ptr, &lenght, 1);
        shader.setSourceEntryPoint("main");
        shader.setEntryPoint("main");

        shader.getIntermediate()->setSource(glslang::EShSourceGlsl);
        shader.getIntermediate()->setEntryPointName("main");
        if (debug)
        {
            shader.getIntermediate()->addSourceText(src.c_str(), src.size());
            shader.getIntermediate()->setSourceFile(name.c_str());
        }

        HBE_Includer includer("");

        EShMessages message = static_cast<EShMessages>(EShMessages::EShMsgVulkanRules | EShMessages::EShMsgSpvRules);
        if (debug)
        {
            message = static_cast<EShMessages>(message | EShMessages::EShMsgDebugInfo);
        }

        if (!shader.parse(&DefaultTBuiltInResource,
            460,
            ENoProfile,
            false,
            false,
            message,
            includer))
        {
            String error(shader.getInfoLog());

            String firstErrorLine = error.substr(0, error.find('\n'));
            i32 lineStart = StringFindNth(firstErrorLine, ":", 2);
            i32 lineEnd = StringFindNth(firstErrorLine, ":", 3);
            BX_ENSURE(lineStart >= 0 && lineEnd > lineStart);
            i32 globalErrorLine = std::stoi(firstErrorLine.substr(lineStart + 1, lineEnd - lineStart));

            u32 localErrorLine = 0;
            String localErrorFile = "Not Found";
            for (u32 i = 0; i < includeRanges.size(); i++)
            {
                if (globalErrorLine >= includeRanges[i].startLine && globalErrorLine < includeRanges[i].endLine)
                {
                    localErrorLine = globalErrorLine - includeRanges[i].startLine;
                    localErrorFile = includeRanges[i].name;
                    break;
                }
            }

            BX_LOGE("GLSL Parsing: `{}`\n{}\n({}: {})",
                name,
                error,
                localErrorFile.c_str(),
                localErrorLine);
            return List<u32>{};
        }

        glslang::TProgram program;
        program.addShader(&shader);
        if (!program.link(message))
        {
            BX_LOGE("GLSL Linking: `{}`\n{}\n{}",
                name,
                program.getInfoLog(),
                program.getInfoDebugLog());
        }

        glslang::SpvOptions options{};
        options.validate = true;
        if (debug)
        {
            options.generateDebugInfo = true;
            options.stripDebugInfo = false;
            options.disableOptimizer = true;
        }
        else
        {
            options.generateDebugInfo = false;
            options.stripDebugInfo = true;
            options.disableOptimizer = false;
        }

        List<u32> spirvData{};
        glslang::GlslangToSpv(*program.getIntermediate(stage), spirvData, &options);
        return spirvData;

        /*glslang_messages_t messages = GLSLANG_MSG_HLSL_OFFSETS_BIT | GLSLANG_MSG_VULKAN_RULES_BIT | GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_DEBUG_INFO_BIT;

        glslang_input_t input{};
        input.language = GLSLANG_SOURCE_GLSL;
        input.stage = stage;
        input.client = GLSLANG_CLIENT_VULKAN;
        input.client_version = GLSLANG_TARGET_VULKAN_1_2;
        input.target_language = GLSLANG_TARGET_SPV;
        input.target_language_version = GLSLANG_TARGET_SPV_1_4;
        input.code = src.c_str();
        input.default_version = 100;
        input.default_profile = GLSLANG_CORE_PROFILE;
        input.force_default_version_and_profile = false;
        input.forward_compatible = false;
        input.messages = messages;
        input.resource = reinterpret_cast<const glslang_resource_t*>(&DefaultTBuiltInResource);

        glslang_shader_t* shader = glslang_shader_create(&input);

        if (!glslang_shader_preprocess(shader, &input))
        {
            BX_LOGE("GLSL Preprocessing: `{}`\n{}\n{}",
                name,
                glslang_shader_get_info_log(shader),
                glslang_shader_get_info_debug_log(shader));
            glslang_shader_delete(shader);
            return List<u32>{};
        }

        if (!glslang_shader_parse(shader, &input))
        {
            String error(glslang_shader_get_info_log(shader));

            String firstErrorLine = error.substr(0, error.find('\n'));
            i32 lineStart = StringFindNth(firstErrorLine, ":", 2);
            i32 lineEnd = StringFindNth(firstErrorLine, ":", 3);
            BX_ENSURE(lineStart >= 0 && lineEnd > lineStart);
            i32 globalErrorLine = std::stoi(firstErrorLine.substr(lineStart + 1, lineEnd - lineStart));

            u32 localErrorLine = 0;
            String localErrorFile = "Not Found";
            for (u32 i = 0; i < includeRanges.size(); i++)
            {
                if (globalErrorLine >= includeRanges[i].startLine && globalErrorLine < includeRanges[i].endLine)
                {
                    localErrorLine = globalErrorLine - includeRanges[i].startLine;
                    localErrorFile = includeRanges[i].name;
                    break;
                }
            }

            BX_LOGE("GLSL Parsing: `{}`\n{}\n({}: {})\n{}",
                name,
                error,
                localErrorFile.c_str(),
                localErrorLine,
                glslang_shader_get_info_debug_log(shader));
            glslang_shader_delete(shader);
            return List<u32>{};
        }

        glslang_program_t* program = glslang_program_create();
        glslang_program_add_shader(program, shader);

        if (!glslang_program_link(program, messages))
        {
            BX_LOGE("GLSL Linking: `{}`\n{}\n{}",
                name,
                glslang_shader_get_info_log(shader),
                glslang_shader_get_info_debug_log(shader));
            glslang_shader_delete(shader);
            glslang_program_delete(program);
            glslang_shader_delete(shader);
            return List<u32>{};
        }

        glslang_program_SPIRV_generate(program, stage);

        SizeType size = glslang_program_SPIRV_get_size(program);
        List<u32> words(size);
        glslang_program_SPIRV_get(program, words.data());

        const char* spirv_messages = glslang_program_SPIRV_get_messages(program);
        if (spirv_messages)
        {
            BX_LOGW("GLSL: `{}`\n{}", name, spirv_messages);
        }

        glslang_program_delete(program);
        glslang_shader_delete(shader);

        return words;*/
    }
}