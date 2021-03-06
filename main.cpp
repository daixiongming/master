#include <iostream>
#include <xmmintrin.h>
#include <pmmintrin.h>
#include <runtime_assert>

#include <Application.hpp>

using namespace std;
using namespace haste;

int main(int argc, char **argv) {
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    Options options = parseArgs(argc, argv);
    auto status = displayHelpIfNecessary(options, "0.0.1");

    if (status.first) {
        return status.second;
    }

    Application application(options);

    if (!options.batch) {
        return application.run(options.width, options.height);
    }
    else {
        return application.runBatch(options.width, options.height);
    }

    return 0;
}
