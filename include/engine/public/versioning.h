#pragma once

#define ENGINE_PUBLIC_API_VERSION_MAJOR 1
#define ENGINE_PUBLIC_API_VERSION_MINOR 0
#define ENGINE_PUBLIC_API_VERSION_PATCH 0

#define ENGINE_DEPRECATED(msg) [[deprecated(msg)]]

namespace engine::public_api {

struct ApiVersion {
    int major {ENGINE_PUBLIC_API_VERSION_MAJOR};
    int minor {ENGINE_PUBLIC_API_VERSION_MINOR};
    int patch {ENGINE_PUBLIC_API_VERSION_PATCH};
};

// Versioning policy:
// - PATCH: bug fixes, no source/binary API break.
// - MINOR: additive API changes only.
// - MAJOR: breaking public API changes.
// Deprecation policy:
// - APIs are first marked ENGINE_DEPRECATED for at least one MINOR release before removal in next MAJOR.

} // namespace engine::public_api
