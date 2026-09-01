#include "core/OpenMpRuntime.hpp"

#include <omp.h>

namespace fruitcat {

OpenMpRuntimeInfo configureAndInspectOpenMp(int requestedThreads) {
    OpenMpRuntimeInfo info;
    info.requestedThreads = requestedThreads;
    info.processorCount = omp_get_num_procs();

    omp_set_dynamic(0);
    omp_set_num_threads(requestedThreads);
    info.maximumThreads = omp_get_max_threads();

    // OpenMP usa un modelo fork-join: el hilo inicial crea aqui un equipo de
    // hilos que comparte memoria y vuelve a unirse al terminar la region.
    #pragma omp parallel
    {
        // single registra el tamano del equipo una sola vez. Al finalizar la
        // region existe una barrera implicita antes de continuar en serie.
        #pragma omp single
        {
            info.activeThreads = omp_get_num_threads();
        }
    }

    return info;
}

} // namespace fruitcat
