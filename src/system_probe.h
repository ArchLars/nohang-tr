#pragma once
#include <optional>
#include <string>

struct ProbeSample {
    long mem_available_kib = 0;
    double psi_mem_avg10 = 0.0;
    double psi_mem_avg60 = 0.0;
};

class SystemProbe {
public:
    ProbeSample sample() const;
private:
    static long readMemAvailableKiB();
    static std::optional<std::pair<double,double>> readPsiMemoryAvg10Avg60();
};
