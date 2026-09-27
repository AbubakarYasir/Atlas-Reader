#pragma once

#include "app/index/ILibraryDocumentInspector.h"

namespace atlas::index {

class QtPdfDocumentInspector final : public ILibraryDocumentInspector {
public:
    [[nodiscard]] LibraryDocumentInspection inspect(
        const std::filesystem::path& source) const override;
};

} // namespace atlas::index
