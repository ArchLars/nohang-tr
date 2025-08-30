#include "system_probe.h"
#include <fstream>
#include <sstream>
#include <string>

static double parseDouble(const std::string& line, const char* key) {
    auto pos = line.find(key);
    if (pos == std::string::npos) return 0.0;
    pos += std::string(key).size();
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
    std::stringstream ss(line.substr(pos));
    double v = 0.0;
    ss >> v;
    return v;
}

static long parseLong(const std::string& line, const char* key) {
    auto pos = line.find(key);
    if (pos == std::string::npos) return 0;
    pos += std::string(key).size();
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
    std::stringstream ss(line.substr(pos));
    long v = 0;
    ss >> v;
    return v;
}

long SystemProbe::readMemAvailableKiB() {
    std::ifstream f("/proc/meminfo");
    std::string k, unit;
    long v = 0;
    while (f >> k >> v >> unit) {
        if (k == "MemAvailable:") return v;
    }
    return 0;
}

std::optional<std::pair<SystemProbe::PsiType, PsiValues>> SystemProbe::parsePsiMemoryLine(const std::string& line) {
    PsiType type;
    if (line.rfind("some", 0) == 0) {
        type = PsiType::Some;
    } else if (line.rfind("full", 0) == 0) {
        type = PsiType::Full;
    } else {
        return std::nullopt;
    }
    PsiValues v;
    v.avg10 = parseDouble(line, "avg10=");
    v.avg60 = parseDouble(line, "avg60=");
    v.avg300 = parseDouble(line, "avg300=");
    v.total = parseLong(line, "total=");
    return std::make_pair(type, v);
}

std::optional<std::pair<PsiValues, PsiValues>> SystemProbe::readPsiMemory() {
    std::ifstream f("/proc/pressure/memory");
    std::string line;
    std::optional<PsiValues> some, full;
    while (std::getline(f, line)) {
        auto parsed = parsePsiMemoryLine(line);
        if (!parsed) continue;
        if (parsed->first == PsiType::Some) some = parsed->second;
        else if (parsed->first == PsiType::Full) full = parsed->second;
    }
    if (some && full) return std::make_pair(*some, *full);
    return std::nullopt;
}

std::optional<ProbeSample> SystemProbe::sample() const {
    ProbeSample s;
    s.mem_available_kib = readMemAvailableKiB();
    auto psi = readPsiMemory();
    if (!psi) return std::nullopt;
    s.some = psi->first;
    s.full = psi->second;
    return s;
}
