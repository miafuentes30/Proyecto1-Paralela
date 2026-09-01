#pragma once

namespace fruitcat {

struct OpenMpRuntimeInfo {
    int requestedThreads = 0;
    int activeThreads = 0;
    int maximumThreads = 0;
    int processorCount = 0;
};

OpenMpRuntimeInfo configureAndInspectOpenMp(int requestedThreads);

} // namespace fruitcat
