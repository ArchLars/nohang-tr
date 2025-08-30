#include <catch2/catch_all.hpp>
#include <QDir>
#include "config.h"

namespace {
struct EnvGuard {
    QByteArray name;
    QByteArray old;
    bool had;
    EnvGuard(const char* n, const QByteArray& v) : name(n) {
        had = qEnvironmentVariableIsSet(n);
        if (had) old = qgetenv(n);
        if (v.isNull())
            qunsetenv(n);
        else
            qputenv(n, v);
    }
    ~EnvGuard() {
        if (had)
            qputenv(name.constData(), old);
        else
            qunsetenv(name.constData());
    }
};
} // namespace

TEST_CASE("explicit path is returned") {
    auto p = resolveConfigPath("/tmp/custom.toml");
    CHECK(p == QString("/tmp/custom.toml"));
}

TEST_CASE("XDG_CONFIG_HOME is used when set") {
    EnvGuard home("HOME", "/home/tester");
    EnvGuard xdg("XDG_CONFIG_HOME", "/xdg");
    auto p = resolveConfigPath();
    CHECK(p == QDir("/xdg").filePath("nohang-tr/nohang-tr.toml"));
}

TEST_CASE("HOME fallback when XDG_CONFIG_HOME missing") {
    EnvGuard xdg("XDG_CONFIG_HOME", QByteArray());
    EnvGuard home("HOME", "/home/tester");
    auto p = resolveConfigPath();
    CHECK(p == QDir("/home/tester/.config").filePath("nohang-tr/nohang-tr.toml"));
}
