#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace atlas::reader {

enum class OpenState {
    loading,
    ready,
    passwordRequired,
    missing,
    malformed,
    unsupportedSecurity,
    cancelled,
    failed,
};

struct OpenResult {
    OpenState state{OpenState::failed};
    std::string title;
    int pageCount{};
    std::string detail;
};

struct Session {
    std::uint64_t id{};
    std::uint64_t revision{};
    std::filesystem::path source;
    std::string sourceKey;
    std::string title;
    OpenState state{OpenState::loading};
    int pageCount{};
    std::string detail;
};

struct BeginOpenResult {
    std::uint64_t sessionId{};
    std::uint64_t revision{};
    bool needsOpen{};
};

class ReaderSessionModel final {
public:
    [[nodiscard]] BeginOpenResult beginOpen(
        std::filesystem::path source,
        std::string sourceKey,
        std::string fallbackTitle);
    [[nodiscard]] std::optional<std::uint64_t> reopen(std::uint64_t sessionId);
    [[nodiscard]] bool completeOpen(
        std::uint64_t sessionId,
        std::uint64_t revision,
        OpenResult result);
    [[nodiscard]] bool close(std::uint64_t sessionId);
    [[nodiscard]] bool activate(std::uint64_t sessionId);

    [[nodiscard]] const std::vector<Session>& sessions() const noexcept;
    [[nodiscard]] std::optional<std::uint64_t> activeSessionId() const noexcept;
    [[nodiscard]] int activeIndex() const noexcept;

private:
    [[nodiscard]] std::vector<Session>::iterator find(std::uint64_t sessionId);
    [[nodiscard]] std::vector<Session>::const_iterator find(std::uint64_t sessionId) const;

    std::vector<Session> sessions_;
    std::optional<std::uint64_t> activeSessionId_;
    std::uint64_t nextSessionId_{1};
};

[[nodiscard]] const char* openStateName(OpenState state) noexcept;

} // namespace atlas::reader
