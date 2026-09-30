#include "core/reader/ReaderSessionModel.h"

#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

} // namespace

int main() {
    using atlas::reader::OpenResult;
    using atlas::reader::OpenState;
    using atlas::reader::ReaderSessionModel;

    try {
        ReaderSessionModel model;
        const auto first = model.beginOpen("one.pdf", "c:/books/one.pdf", "one");
        require(first.needsOpen && first.revision == 1 && model.activeIndex() == 0,
            "A new source must create one active loading session.");
        require(model.sessions().front().state == OpenState::loading,
            "A new session must begin in loading state.");

        const auto duplicate = model.beginOpen("ONE.pdf", "c:/books/one.pdf", "duplicate");
        require(!duplicate.needsOpen && duplicate.sessionId == first.sessionId
                && model.sessions().size() == 1,
            "Opening the same normalized source must activate rather than duplicate its tab.");

        require(model.completeOpen(first.sessionId, first.revision,
                    OpenResult{OpenState::ready, "Document title", 12, {}}),
            "The current open result must be applied.");
        require(model.sessions().front().state == OpenState::ready
                && model.sessions().front().pageCount == 12
                && model.sessions().front().title == "Document title",
            "A successful result must expose immutable reader metadata.");

        const auto reopenedRevision = model.reopen(first.sessionId);
        require(reopenedRevision.has_value() && *reopenedRevision == 2,
            "Reopen must advance the session revision.");
        require(!model.completeOpen(first.sessionId, 1,
                    OpenResult{OpenState::malformed, {}, 0, "stale"}),
            "A stale asynchronous result must never overwrite a newer open request.");
        require(model.completeOpen(first.sessionId, 2,
                    OpenResult{OpenState::passwordRequired, {}, 0, "password required"}),
            "The current error result must be applied.");
        require(model.sessions().front().state == OpenState::passwordRequired,
            "Password-required must remain distinct from malformed or missing.");

        const auto second = model.beginOpen("two.pdf", "c:/books/two.pdf", "two");
        require(second.needsOpen && model.sessions().size() == 2 && model.activeIndex() == 1,
            "A different source must create and activate another tab.");
        require(model.activate(first.sessionId) && model.activeIndex() == 0,
            "Tab activation must be explicit and deterministic.");
        require(model.close(first.sessionId) && model.sessions().size() == 1
                && model.activeSessionId() == second.sessionId,
            "Closing the active tab must select a predictable neighbour.");
        require(!model.completeOpen(first.sessionId, 2,
                    OpenResult{OpenState::ready, "late", 1, {}}),
            "A completion arriving after close must be ignored.");
        require(model.close(second.sessionId) && model.activeIndex() == -1,
            "Closing the last tab must return to the empty workspace.");

        const auto missing = model.beginOpen("missing.pdf", "c:/books/missing.pdf", "missing");
        require(model.completeOpen(missing.sessionId, missing.revision,
                    OpenResult{OpenState::missing, {}, 0, "file not found"}),
            "Missing files must be representable without collapsing into a generic failure.");
        require(model.sessions().front().state == OpenState::missing,
            "Missing state must remain observable.");
    } catch (const std::exception& error) {
        std::cerr << "Reader session model test failed: " << error.what() << '\n';
        return 1;
    }

    std::cout << "Reader session lifecycle, duplicate, stale-result and error-state checks passed.\n";
}
