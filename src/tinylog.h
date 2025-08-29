// tinylog.h — C++17, Linux-only, local time, no single-line ifs

#pragma once
#if !defined(__linux__)
#error "tinylog_linux.hpp is Linux-only"
#endif

#include <cstdarg>
#include <cstdio>
#include <mutex>
#include <string_view>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

inline const char* kLogLayer = "LAYER";
inline const char* kLogSync = "SYNC";


namespace tinylog {

// Shared across TUs (C++17 inline variables)
inline std::mutex g_mu;
inline int g_fd = STDERR_FILENO;

inline unsigned long tid() {
    return static_cast<unsigned long>(::syscall(SYS_gettid));
}

inline void set_fd(int fd) {
    std::lock_guard<std::mutex> l(g_mu);
    g_fd = fd;
}

// Open a file and start logging there (O_APPEND by default).
inline int set_file(const char* path, bool append = true, mode_t mode = 0644) {
    int flags = O_WRONLY | O_CREAT | O_CLOEXEC | (append ? O_APPEND : O_TRUNC);
    int fd = ::open(path, flags, mode);
    if (fd >= 0) {
        set_fd(fd);
    }
    return fd;
}

// printf-style logger. Thread-safe, single writev() to avoid interleaving.
inline void logf(const char* channel, const char* fmt, ...) noexcept {
    // TODO: Allow for filtering channels.
    return;

    timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);

    tm tmv;
    ::localtime_r(&ts.tv_sec, &tmv);

    char tbuf[32];
    int tlen = std::snprintf(
        tbuf, sizeof tbuf,
        "%04d-%02d-%02d %02d:%02d:%02d.%06ld",
        tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
        tmv.tm_hour, tmv.tm_min, tmv.tm_sec, ts.tv_nsec / 1000L
    );
    if (tlen < 0) {
        return;
    }
    if (tlen >= static_cast<int>(sizeof tbuf)) {
        tlen = static_cast<int>(sizeof tbuf) - 1;
    }

    char pbuf[32];
    int plen = std::snprintf(pbuf, sizeof pbuf, " [tid %lu] ", tid());
    if (plen < 0) {
        return;
    }
    if (plen >= static_cast<int>(sizeof pbuf)) {
        plen = static_cast<int>(sizeof pbuf) - 1;
    }

    char mbuf[4096];
    va_list ap;
    va_start(ap, fmt);
    int want = std::vsnprintf(mbuf, sizeof mbuf, fmt, ap);
    va_end(ap);
    if (want < 0) {
        return;
    }

    int mlen = want;
    if (mlen >= static_cast<int>(sizeof mbuf)) {
        mlen = static_cast<int>(sizeof mbuf) - 1;
    }

    while (mlen > 0 && (mbuf[mlen - 1] == '\n' || mbuf[mlen - 1] == '\r')) {
        mbuf[--mlen] = '\0';
    }

    iovec iov[4] = {
        { tbuf, static_cast<size_t>(tlen) },
        { pbuf, static_cast<size_t>(plen) },
        { mbuf, static_cast<size_t>(mlen) },
        { const_cast<char*>("\n"), static_cast<size_t>(1) }
    };

    // Lock while reading g_fd and writing to it to avoid data races.
    std::lock_guard<std::mutex> l(g_mu);
    int fd = g_fd;
    (void)::writev(fd, iov, 4);
}

} // namespace tinylog

// Macro alias for convenience
#define LOGF(...) ::tinylog::logf(__VA_ARGS__)
