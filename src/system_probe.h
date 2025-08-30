#pragma once
#include <optional>
#include <string>
#include <utility>

/**
 * Pressure stall information values.
 */
struct PsiValues {
    double avg10 = 0.0;
    double avg60 = 0.0;
    double avg300 = 0.0;
    long total = 0;
};

struct ProbeSample {
    long mem_available_kib = 0;
    PsiValues some;
    PsiValues full;
};

class SystemProbe {
public:
    virtual ~SystemProbe() = default;
    virtual std::optional<ProbeSample> sample() const;

    enum class PsiType { Some, Full };
    static std::optional<std::pair<PsiType, PsiValues>> parsePsiMemoryLine(const std::string& line);
private:
    static long readMemAvailableKiB();
    static std::optional<std::pair<PsiValues, PsiValues>> readPsiMemory();
};
