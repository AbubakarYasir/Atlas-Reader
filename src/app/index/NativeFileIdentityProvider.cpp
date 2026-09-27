#include "app/index/NativeFileIdentityProvider.h"

#include <array>
#include <cstdint>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#else
#include <cerrno>
#include <sys/stat.h>
#endif

namespace atlas::index {
namespace {

#ifdef _WIN32

class Handle final {
public:
    explicit Handle(HANDLE value) : value_{value} {}
    ~Handle() {
        if (value_ != INVALID_HANDLE_VALUE) CloseHandle(value_);
    }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    [[nodiscard]] HANDLE get() const { return value_; }

private:
    HANDLE value_{INVALID_HANDLE_VALUE};
};

[[nodiscard]] FileIdentityState stateForWindowsError(DWORD error) {
    if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) {
        return FileIdentityState::missing;
    }
    if (error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION
        || error == ERROR_LOCK_VIOLATION) {
        return FileIdentityState::inaccessible;
    }
    return FileIdentityState::unsupported;
}

[[nodiscard]] std::string windowsIdentity(const FILE_ID_INFO& information) {
    std::ostringstream result;
    result << "win:" << std::hex << std::setfill('0') << std::setw(16)
           << information.VolumeSerialNumber << ':';
    for (const auto byte : information.FileId.Identifier) {
        result << std::setw(2) << static_cast<unsigned int>(byte);
    }
    return result.str();
}

#endif

} // namespace

FileIdentityResult NativeFileIdentityProvider::inspect(
    const std::filesystem::path& source) const {
    if (source.empty()) return {FileIdentityState::missing, {}};

#ifdef _WIN32
    Handle handle{CreateFileW(
        source.c_str(),
        FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr)};
    if (handle.get() == INVALID_HANDLE_VALUE) {
        return {stateForWindowsError(GetLastError()), {}};
    }

    FILE_ID_INFO information{};
    if (!GetFileInformationByHandleEx(
            handle.get(), FileIdInfo, &information, sizeof(information))) {
        return {stateForWindowsError(GetLastError()), {}};
    }
    return {FileIdentityState::available, windowsIdentity(information)};
#else
    struct stat information {};
    if (::stat(source.c_str(), &information) != 0) {
        if (errno == ENOENT || errno == ENOTDIR) return {FileIdentityState::missing, {}};
        if (errno == EACCES) return {FileIdentityState::inaccessible, {}};
        return {FileIdentityState::unsupported, {}};
    }
    return {
        FileIdentityState::available,
        "posix:" + std::to_string(static_cast<std::uintmax_t>(information.st_dev))
            + ':' + std::to_string(static_cast<std::uintmax_t>(information.st_ino)),
    };
#endif
}

} // namespace atlas::index

