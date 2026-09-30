#include "core/reader/ReaderSessionModel.h"

#include <algorithm>
#include <utility>

namespace atlas::reader {

BeginOpenResult ReaderSessionModel::beginOpen(
    std::filesystem::path source,
    std::string sourceKey,
    std::string fallbackTitle) {
    const auto duplicate = std::find_if(sessions_.begin(), sessions_.end(), [&sourceKey](const Session& session) {
        return session.sourceKey == sourceKey;
    });
    if (duplicate != sessions_.end()) {
        activeSessionId_ = duplicate->id;
        return {duplicate->id, duplicate->revision, false};
    }

    Session session;
    session.id = nextSessionId_++;
    session.revision = 1;
    session.source = std::move(source);
    session.sourceKey = std::move(sourceKey);
    session.title = std::move(fallbackTitle);
    const auto id = session.id;
    sessions_.push_back(std::move(session));
    activeSessionId_ = id;
    return {id, 1, true};
}

std::optional<std::uint64_t> ReaderSessionModel::reopen(std::uint64_t sessionId) {
    const auto session = find(sessionId);
    if (session == sessions_.end()) return std::nullopt;
    ++session->revision;
    session->state = OpenState::loading;
    session->pageCount = 0;
    session->detail.clear();
    activeSessionId_ = sessionId;
    return session->revision;
}

bool ReaderSessionModel::completeOpen(
    std::uint64_t sessionId,
    std::uint64_t revision,
    OpenResult result) {
    const auto session = find(sessionId);
    if (session == sessions_.end() || session->revision != revision) return false;
    session->state = result.state;
    if (!result.title.empty()) session->title = std::move(result.title);
    session->pageCount = result.state == OpenState::ready ? result.pageCount : 0;
    session->detail = std::move(result.detail);
    return true;
}

bool ReaderSessionModel::close(std::uint64_t sessionId) {
    const auto session = find(sessionId);
    if (session == sessions_.end()) return false;
    const auto removedIndex = static_cast<std::size_t>(std::distance(sessions_.begin(), session));
    const bool wasActive = activeSessionId_ == sessionId;
    sessions_.erase(session);
    if (wasActive) {
        if (sessions_.empty()) {
            activeSessionId_.reset();
        } else {
            const auto replacement = std::min(removedIndex, sessions_.size() - 1);
            activeSessionId_ = sessions_[replacement].id;
        }
    }
    return true;
}

bool ReaderSessionModel::activate(std::uint64_t sessionId) {
    if (find(sessionId) == sessions_.end()) return false;
    activeSessionId_ = sessionId;
    return true;
}

const std::vector<Session>& ReaderSessionModel::sessions() const noexcept { return sessions_; }

std::optional<std::uint64_t> ReaderSessionModel::activeSessionId() const noexcept {
    return activeSessionId_;
}

int ReaderSessionModel::activeIndex() const noexcept {
    if (!activeSessionId_.has_value()) return -1;
    const auto session = find(*activeSessionId_);
    return session == sessions_.end()
        ? -1
        : static_cast<int>(std::distance(sessions_.cbegin(), session));
}

std::vector<Session>::iterator ReaderSessionModel::find(std::uint64_t sessionId) {
    return std::find_if(sessions_.begin(), sessions_.end(), [sessionId](const Session& session) {
        return session.id == sessionId;
    });
}

std::vector<Session>::const_iterator ReaderSessionModel::find(std::uint64_t sessionId) const {
    return std::find_if(sessions_.cbegin(), sessions_.cend(), [sessionId](const Session& session) {
        return session.id == sessionId;
    });
}

const char* openStateName(OpenState state) noexcept {
    switch (state) {
    case OpenState::loading: return "loading";
    case OpenState::ready: return "ready";
    case OpenState::passwordRequired: return "passwordRequired";
    case OpenState::missing: return "missing";
    case OpenState::malformed: return "malformed";
    case OpenState::unsupportedSecurity: return "unsupportedSecurity";
    case OpenState::cancelled: return "cancelled";
    case OpenState::failed: return "failed";
    }
    return "failed";
}

} // namespace atlas::reader
