#include "system_probe.h"
#include <fstream>
#include <sstream>
#include <string>

static double parseAvgN(const std::string& line, const char* key) {
    auto pos = line.find(key);
    if (pos == std::string::npos) return 0.0;
    pos += std::string(key).size();
    while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) ++pos;
    std::stringstream ss(line.substr(pos));
    double v = 0.0;
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

std::optional<std::pair<double,double>> SystemProbe::readPsiMemoryAvg10Avg60() {
    std::ifstream f("/proc/pressure/memory");
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("some", 0) == 0) {
            double avg10 = parseAvgN(line, "avg10=");
            double avg60 = parseAvgN(line, "avg60=");
            return std::make_pair(avg10, avg60);
        }
    }
    return std::nullopt;
}

ProbeSample SystemProbe::sample() const {
    ProbeSample s;
    s.mem_available_kib = readMemAvailableKiB();
    auto psi = readPsiMemoryAvg10Avg60();
    if (psi) {
        s.psi_mem_avg10 = psi->first;
        s.psi_mem_avg60 = psi->second;
    }
    return s;
}
