#include "bx/engine/core/profiler.hpp"
#include "bx/engine/core/math.hpp"

#include <deque>

struct ProfilerEntry
{
    TimePoint start{};
    TimePoint end{};
    TimeSpan accum{};
    u64 samples = 0;
    std::deque<f32> history;
};

static HashMap<String, ProfilerData> s_data;
static HashMap<String, ProfilerEntry> s_entries;
static Timer timer;
static f32 g_time = 0;

ProfilerSection::ProfilerSection(const String& name) : m_name(name)
{
    Profiler::BeginSection(m_name);
}

ProfilerSection::~ProfilerSection()
{
    Profiler::EndSection(m_name);
}

void Profiler::Update()
{
    const f32 delta = 1.0f / 30.0f;

    g_time += Time::GetDeltaTime();
    if (g_time > delta)
    {
        g_time = Math::FMod(g_time, delta);

        for (auto& itr : s_entries)
        {
            auto& data = s_data[itr.first];

            auto& e = itr.second;
            float duration = (float)((double)e.accum.count() / 1000000.0);

            f32 avg = duration / e.samples;
            itr.second.accum = {};
            itr.second.samples = 0;

            e.history.push_back(avg);
            if (e.history.size() > 100)
                e.history.pop_front();

            data.avg = avg;
            data.frames.clear();
            data.frames.reserve(e.history.size());
            for (auto avg : e.history)
                data.frames.emplace_back(avg);
        }
    }
}

void Profiler::BeginSection(const String& name)
{
//#ifdef BUILD_DEBUG
    s_entries[name].start = Clock::now();
//#endif
}

void Profiler::EndSection(const String& name)
{
//#ifdef BUILD_DEBUG
    auto& e = s_entries[name];
    e.end = Clock::now();
    e.accum += e.end - e.start;
    e.samples++;
//#endif
}

const HashMap<String, ProfilerData>& Profiler::GetData()
{
    return s_data;
}