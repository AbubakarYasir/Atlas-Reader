#include "core/document/DocumentCapabilities.h"

#include <cassert>

int main() {
    using atlas::document::Availability;
    using atlas::document::DocumentCapabilities;
    using atlas::document::MutationSafety;

    const DocumentCapabilities writable{
        .availability = Availability::available,
        .mutation = MutationSafety::writable,
        .canRead = true,
        .canSearchText = true,
        .canRender = true,
        .canEmbedOutline = true,
        .canEmbedAnnotations = true,
    };

    assert(writable.canRead);
    assert(writable.canEmbedOutline);
    assert(writable.mutation == MutationSafety::writable);

    const DocumentCapabilities restricted{
        .availability = Availability::readOnly,
        .mutation = MutationSafety::permissionRestricted,
        .canRead = true,
        .canSearchText = true,
        .canRender = true,
        .canEmbedOutline = false,
        .canEmbedAnnotations = false,
    };

    assert(restricted.canRead);
    assert(!restricted.canEmbedOutline);
    assert(restricted.mutation == MutationSafety::permissionRestricted);

    return 0;
}
