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
 * @brief Pressure stall information values.
 */
struct PsiValues {
    double avg10 = 0.0;
    double avg60 = 0.0;
    double avg300 = 0.0;
    long total = 0;
};

/**
 * @brief Snapshot of memory availability and PSI readings.
 */
struct ProbeSample {
    std::optional<long> mem_available_kib; ///< MemAvailable in KiB if readable.
    PsiValues some;                       ///< PSI "some" memory values.
    PsiValues full;                       ///< PSI "full" memory values.
};

/**
 * @brief Reads memory statistics from the system.
 */
class SystemProbe {
public:
    /** Virtual destructor for polymorphic use. */
    virtual ~SystemProbe() = default;

    /**
     * @brief Obtain a single sample of current memory statistics.
     * @return ProbeSample with current readings or std::nullopt on failure.
     */
    virtual std::optional<ProbeSample> sample() const;

    /// PSI memory line type indicator.
    enum class PsiType { Some, Full };

    /**
     * @brief Parse a line from /proc/pressure/memory.
     * @param line Line to parse.
     * @return Pair of PSI type and values or std::nullopt on failure.
     */
    static std::optional<std::pair<PsiType, PsiValues>> parsePsiMemoryLine(const std::string& line);
private:
    static std::optional<long> readMemAvailableKiB();
    static std::optional<std::pair<PsiValues, PsiValues>> readPsiMemory();
};
