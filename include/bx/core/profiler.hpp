#pragma once

#include "bx/engine/core/byte_types.hpp"
#include "bx/engine/core/time.hpp"
#include "bx/engine/containers/string.hpp"
#include "bx/engine/containers/list.hpp"
#include "bx/engine/containers/hash_map.hpp"

#ifndef _MSC_VER
//https://stackoverflow.com/questions/23230003/something-between-func-and-pretty-function
#include <cstring>
#include <string>
#include <algorithm>
template<size_t FL, size_t PFL>
inline const char* ComputeMethodName(const char(&function)[FL], const char(&prettyFunction)[PFL])
{
using reverse_ptr = std::reverse_iterator<const char*>;
thread_local static char result[PFL];
const char* locFuncName = std::search(prettyFunction, prettyFunction + PFL - 1, function, function + FL - 1);
const char* locClassName = std::find(reverse_ptr(locFuncName), reverse_ptr(prettyFunction), ' ').base();
const char* endFuncName = std::find(locFuncName, prettyFunction + PFL - 1, '(');
std::copy(locClassName, endFuncName, result);
return result;
}
#define __COMPACT_PRETTY_FUNCTION__ ComputeMethodName(__FUNCTION__,__PRETTY_FUNCTION__)
#else
#define __COMPACT_PRETTY_FUNCTION__ __FUNCTION__
#endif

#define PROFILE_FUNCTION() ProfilerSection s_sect(__COMPACT_PRETTY_FUNCTION__);
#define PROFILE_SECTION(id) ProfilerSection s_sect(id);

class ProfilerSection
{
public:
	ProfilerSection(const String& name);
	~ProfilerSection();
private:
	String m_name;
};

struct ProfilerData
{
	f32 min = 0;
	f32 max = 0;
	f32 avg = 0;
	List<f32> frames;
};

class Profiler
{
public:
	static void Update();

	static void BeginSection(const String& name);
	static void EndSection(const String& name);

	static const HashMap<String, ProfilerData>& GetData();
};