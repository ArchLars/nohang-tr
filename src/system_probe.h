#pragma once
#include <istream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

/**
 * Parse the MemAvailable value in KiB from a meminfo-like stream.
 * @param in Input stream providing lines formatted as in /proc/meminfo.
 * @return Parsed value in KiB or std::nullopt if the key is absent.
 */
std::optional<long> parseMemAvailable(std::istream& in);

/**
 * Parse the MemTotal value in KiB from a meminfo-like stream.
 * @param in Input stream providing lines formatted as in /proc/meminfo.
 * @return Parsed value in KiB or std::nullopt if the key is absent.
 */
std::optional<long> parseMemTotal(std::istream& in);

/**
 * Parse the SwapFree value in KiB from a meminfo-like stream.
 * @param in Input stream providing lines formatted as in /proc/meminfo.
 * @return Parsed value in KiB or std::nullopt if the key is absent.
 */
std::optional<long> parseSwapFree(std::istream& in);

/**
 * Parse the MemFree value in KiB from a meminfo-like stream.
 * @param in Input stream providing lines formatted as in /proc/meminfo.
 * @return Parsed value in KiB or std::nullopt if the key is absent.
 */
std::optional<long> parseMemFree(std::istream& in);

/**
 * Parse the Cached value in KiB from a meminfo-like stream.
 * @param in Input stream providing lines formatted as in /proc/meminfo.
 * @return Parsed value in KiB or std::nullopt if the key is absent.
 */
std::optional<long> parseCached(std::istream& in);

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
    std::optional<long> mem_total_kib;     ///< MemTotal in KiB if readable.
    std::optional<long> mem_free_kib;      ///< MemFree in KiB if readable.
    std::optional<long> swap_free_kib;     ///< SwapFree in KiB if readable.
    std::optional<long> cached_kib;        ///< Cached in KiB if readable.
    PsiValues some;                       ///< PSI "some" memory values.
    PsiValues full;                       ///< PSI "full" memory values.
};

/**
 * @brief Reads memory statistics from the system.
 */
class SystemProbe {
public:
    /// PSI memory line type indicator.
    enum class PsiType { Some, Full };

    /** PSI trigger specification. */
    struct Trigger {
        PsiType type;      ///< Trigger type (some/full).
        long stall_us;     ///< Stall amount in microseconds.
        long window_us;    ///< Time window in microseconds.
    };

    /**
     * Construct a probe reading from provided paths.
     * @param meminfoPath Path to meminfo-like file.
     * @param psiPath Path to psi memory file.
     */
    explicit SystemProbe(std::string meminfoPath = "/proc/meminfo",
                         std::string psiPath = "/proc/pressure/memory");

    /** Virtual destructor for polymorphic use. */
    virtual ~SystemProbe();

    /**
     * @brief Enable PSI triggers at the specified path.
     * @param path File path to register triggers (typically /proc/pressure/memory).
     * @param triggers Collection of trigger thresholds to register.
     * @return True on success, false otherwise.
     */
    bool enableTriggers(const std::string& path, const std::vector<Trigger>& triggers);

    /**
     * @brief Enable PSI triggers using the probe's configured path.
     * @param triggers Collection of trigger thresholds to register.
     * @return True on success, false otherwise.
     */
    bool enableTriggers(const std::vector<Trigger>& triggers);

    /**
     * @brief Obtain a single sample of current memory statistics.
     * @return ProbeSample with current readings or std::nullopt on failure.
     */
    virtual std::optional<ProbeSample> sample() const;

    /**
     * @brief Parse a line from /proc/pressure/memory.
     * @param line Line to parse.
     * @return Pair of PSI type and values or std::nullopt on failure.
     */
    static std::optional<std::pair<PsiType, PsiValues>> parsePsiMemoryLine(const std::string& line);

private:
    std::optional<std::string> readMeminfo() const;
    std::optional<std::pair<PsiValues, PsiValues>> readPsiMemory() const;

    std::string meminfoPath_;
    std::string psiPath_;
    mutable int meminfoFd_ = -1;
    mutable int psiFd_ = -1;
    mutable int triggerFd_ = -1;
};
