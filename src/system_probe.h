#pragma once
#include <istream>
#include <optional>
#include <string>
#include <utility>

/**
 * Parse the MemAvailable value in KiB from a meminfo-like stream.
 * @param in Input stream providing lines formatted as in /proc/meminfo.
 * @return Parsed value in KiB or std::nullopt if the key is absent.
 */
std::optional<long> parseMemAvailable(std::istream& in);

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
    std::optional<long> mem_available_kib;
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
    static std::optional<long> readMemAvailableKiB();
    static std::optional<std::pair<PsiValues, PsiValues>> readPsiMemory();
};
