#ifndef CONTENT_CAPABILITY_API_V1_H
#define CONTENT_CAPABILITY_API_V1_H
#include <cstdint>
#include <string>
#include <vector>
// Public optional ABI, owned by Content Manager. Vendored verbatim by consumers.
// Discover once at world startup through WorldScript RTTI; no module link dependency.
namespace ContentCapabilitiesV1
{
struct Resource { std::string symbol, kind; std::uint32_t value=0; };
struct Vendor { std::uint32_t creatureEntry=0, itemEntry=0, extendedCostId=0; };
enum class Result { Inactive, Ready, Invalid };
class Provider
{
public:
    virtual ~Provider() = default;
    // An ACTIVE, APPLIED, validated vendor declaration is the activation signal.
    // STAGED artifacts and retained leases alone never provide capabilities.
    virtual Result Resolve(std::string const& package, std::string const& vendorSymbol,
        std::vector<Resource>& resources, Vendor& vendor, std::string& reason) const = 0;
};
}
#endif
