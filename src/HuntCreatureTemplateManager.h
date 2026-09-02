#ifndef MOD_HUNTS_CREATURE_TEMPLATE_MANAGER_H
#define MOD_HUNTS_CREATURE_TEMPLATE_MANAGER_H

#include "Define.h"

#include <unordered_map>

namespace hunts
{
class HuntCreatureTemplateManager
{
public:
    static HuntCreatureTemplateManager& Instance();

    // Must run before AzerothCore loads creature_template into ObjectMgr.
    bool MaterializeStartupTemplates();

    [[nodiscard]] uint32 ResolveEntry(uint32 lwTemplateId) const;
    [[nodiscard]] uint32 GetMappedTemplateCount() const;

private:
    HuntCreatureTemplateManager() = default;

    bool RetireInactiveMappings();
    bool MaterializeEnabledDefinitions();
    uint32 AllocateEntry() const;
    bool MaterializeDefinition(uint32 lwTemplateId, uint32 allocatedEntry);
    void LoadMappings();

    std::unordered_map<uint32, uint32> _entryByLwTemplate;
};
}

#define sHuntCreatureTemplateMgr hunts::HuntCreatureTemplateManager::Instance()

#endif
