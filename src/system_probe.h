#pragma once
#include <optional>
#include <string>
#include <utility>

struct ProbeSample {
    long mem_available_kib = 0;
    double psi_mem_avg10 = 0.0;
    double psi_mem_avg60 = 0.0;
};

class SystemProbe {
public:
    virtual ~SystemProbe() = default;
    virtual ProbeSample sample() const;
    static std::optional<std::pair<double,double>> parsePsiMemoryLine(const std::string& line);
private:
    static long readMemAvailableKiB();
    static std::optional<std::pair<double,double>> readPsiMemoryAvg10Avg60();
};
