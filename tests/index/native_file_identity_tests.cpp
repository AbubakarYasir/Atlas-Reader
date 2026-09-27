#include "app/index/NativeFileIdentityProvider.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

class TemporaryDirectory final {
public:
    TemporaryDirectory() {
        static std::atomic<std::uint64_t> sequence{};
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path()
            / ("atlas-file-identity-" + std::to_string(nonce) + '-' + std::to_string(sequence++));
        std::filesystem::create_directory(path_);
    }
    ~TemporaryDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path_, ignored);
    }
    [[nodiscard]] const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

void checkRenameAndCopyIdentity() {
    TemporaryDirectory temporary;
    const auto original = temporary.path() / "original.pdf";
    const auto renamed = temporary.path() / "كتاب-renamed.pdf";
    const auto copied = temporary.path() / "copy.pdf";
    {
        std::ofstream output{original, std::ios::binary};
        output << "%PDF-1.4\nfixture\n";
    }

    atlas::index::NativeFileIdentityProvider provider;
    const auto before = provider.inspect(original);
    require(before.state == atlas::index::FileIdentityState::available && !before.identity.empty(),
        "A native file identity must be available for an ordinary PDF.");
    require(provider.inspect(original).identity == before.identity,
        "Repeated inspection of an unchanged file must return a stable identity.");

    std::filesystem::rename(original, renamed);
    const auto afterRename = provider.inspect(renamed);
    require(afterRename.state == atlas::index::FileIdentityState::available
            && afterRename.identity == before.identity,
        "Renaming a file on the same filesystem must preserve its native identity.");
    require(provider.inspect(original).state == atlas::index::FileIdentityState::missing,
        "The old path must be reported missing after rename.");

    std::filesystem::copy_file(renamed, copied);
    const auto copy = provider.inspect(copied);
    require(copy.state == atlas::index::FileIdentityState::available
            && copy.identity != before.identity,
        "A copied file must have a distinct native identity even when its bytes match.");
    require(provider.inspect(temporary.path() / "missing.pdf").state
            == atlas::index::FileIdentityState::missing,
        "A nonexistent path must have an explicit missing state.");
}

} // namespace

int main() {
    try {
        checkRenameAndCopyIdentity();
    } catch (const std::exception& error) {
        std::cerr << "Native file identity test failed: " << error.what() << '\n';
        return 1;
    }
    std::cout << "Native file identity stability, rename, copy and missing-path checks passed.\n";
}

