#include "system_probe.h"
#include <sstream>
#include <string>
#include <vector>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>

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

std::optional<long> parseMemAvailable(std::istream& in) {
    std::string k, unit;
    long v = 0;
    while (in >> k >> v >> unit) {
        if (k == "MemAvailable:") return v;
    }
    return std::nullopt;
}

std::optional<long> parseMemTotal(std::istream& in) {
    std::string k, unit;
    long v = 0;
    while (in >> k >> v >> unit) {
        if (k == "MemTotal:") return v;
    }
    return std::nullopt;
}

SystemProbe::SystemProbe(std::string meminfoPath, std::string psiPath)
    : meminfoPath_(std::move(meminfoPath)), psiPath_(std::move(psiPath)) {
    meminfoFd_ = open(meminfoPath_.c_str(), O_RDONLY | O_CLOEXEC);
    psiFd_ = open(psiPath_.c_str(), O_RDONLY | O_CLOEXEC);
}

SystemProbe::~SystemProbe() {
    if (triggerFd_ >= 0) close(triggerFd_);
    if (meminfoFd_ >= 0) close(meminfoFd_);
    if (psiFd_ >= 0) close(psiFd_);
}

std::optional<std::string> SystemProbe::readMeminfo() const {
    if (meminfoFd_ < 0) {
        meminfoFd_ = open(meminfoPath_.c_str(), O_RDONLY | O_CLOEXEC);
        if (meminfoFd_ < 0) return std::nullopt;
    }
    if (lseek(meminfoFd_, 0, SEEK_SET) < 0) return std::nullopt;
    std::string content;
    char buf[4096];
    ssize_t n;
    while ((n = read(meminfoFd_, buf, sizeof(buf))) > 0) {
        content.append(buf, n);
    }
    if (n < 0) return std::nullopt;
    return content;
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

std::optional<std::pair<PsiValues, PsiValues>> SystemProbe::readPsiMemory() const {
    if (psiFd_ < 0) {
        psiFd_ = open(psiPath_.c_str(), O_RDONLY | O_CLOEXEC);
        if (psiFd_ < 0) {
            std::cerr << "PSI unavailable: cannot open " << psiPath_ << ": "
                      << std::strerror(errno) << "\n";
            return std::nullopt;
        }
    }
    if (lseek(psiFd_, 0, SEEK_SET) < 0) {
        std::cerr << "PSI unavailable: seek failed for " << psiPath_ << ": "
                  << std::strerror(errno) << "\n";
        return std::nullopt;
    }
    std::string content;
    char buf[4096];
    ssize_t n;
    while ((n = read(psiFd_, buf, sizeof(buf))) > 0) {
        content.append(buf, n);
    }
    if (n < 0) {
        std::cerr << "PSI unavailable: read error from " << psiPath_ << ": "
                  << std::strerror(errno) << "\n";
        return std::nullopt;
    }
    std::istringstream f(content);
    std::string line;
    std::optional<PsiValues> some, full;
    while (std::getline(f, line)) {
        auto parsed = parsePsiMemoryLine(line);
        if (!parsed) continue;
        if (parsed->first == PsiType::Some) some = parsed->second;
        else if (parsed->first == PsiType::Full) full = parsed->second;
    }
    if (some && full) return std::make_pair(*some, *full);
    std::cerr << "PSI unavailable: incomplete data in " << psiPath_ << "\n";
    return std::nullopt;
}

bool SystemProbe::enableTriggers(const std::string& path, const std::vector<Trigger>& triggers) {
    int fd = open(path.c_str(), O_RDWR | O_NONBLOCK);
    if (fd < 0) return false;
    for (const auto& t : triggers) {
        std::string line = (t.type == PsiType::Some ? "some " : "full ") +
                           std::to_string(t.stall_us) + " " + std::to_string(t.window_us) + "\n";
        if (write(fd, line.c_str(), line.size()) < 0) {
            close(fd);
            return false;
        }
    }
    triggerFd_ = fd;
    return true;
}

bool SystemProbe::enableTriggers(const std::vector<Trigger>& triggers) {
    return enableTriggers(psiPath_, triggers);
}

std::optional<ProbeSample> SystemProbe::sample() const {
    if (triggerFd_ >= 0) {
        struct pollfd pfd { triggerFd_, POLLPRI, 0 };
        if (poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLPRI)) {
            char buf[128];
            lseek(triggerFd_, 0, SEEK_SET);
            while (read(triggerFd_, buf, sizeof(buf)) > 0) {
            }
        }
    }
    ProbeSample s;
    if (auto mem = readMeminfo()) {
        std::istringstream ss(*mem);
        s.mem_available_kib = parseMemAvailable(ss);
        ss.clear();
        ss.seekg(0);
        s.mem_total_kib = parseMemTotal(ss);
    }
    auto psi = readPsiMemory();
    if (!psi) return std::nullopt;
    s.some = psi->first;
    s.full = psi->second;
    return s;
}
