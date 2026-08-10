#pragma once

#include <cstdint>

namespace mxh::services {

// Minimal Phase B contract consumed by quest dialogs.  The service owns
// persistence, reward validation, and side effects; the UI only requests a
// claim for a completed quest and receives success/failure.
class IQuestService {
public:
    virtual ~IQuestService() = default;
    virtual bool claimQuest(std::uint32_t quest_id) = 0;
};

} // namespace mxh::services
