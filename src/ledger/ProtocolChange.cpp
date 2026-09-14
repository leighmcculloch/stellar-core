// Copyright 2026 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "ledger/ProtocolChange.h"

#include <algorithm>
#include <stdexcept>

namespace stellar
{

char const* const PROTOCOL_CHANGE_DISABLE_BUMP_SEQUENCE =
    "disable-bump-sequence";

#ifdef BUILD_TESTS
char const* const PROTOCOL_CHANGE_TEST_NO_OP = "test-no-op";
#endif

std::vector<ProtocolChange> const&
getKnownProtocolChanges()
{
    static std::vector<ProtocolChange> const changes = {
        {PROTOCOL_CHANGE_DISABLE_BUMP_SEQUENCE,
         "Stop supporting the BumpSequence operation."},
#ifdef BUILD_TESTS
        {PROTOCOL_CHANGE_TEST_NO_OP,
         "No behavioural effect; used to test the voting mechanism."},
#endif
    };
    return changes;
}

bool
isKnownProtocolChange(std::string const& name)
{
    auto const& changes = getKnownProtocolChanges();
    return std::any_of(
        changes.begin(), changes.end(),
        [&name](ProtocolChange const& c) { return c.mName == name; });
}

std::vector<std::string>
getActivatedProtocolChanges(LedgerHeader const& header)
{
    if (header.ext.v() != 1 || header.ext.v1().ext.v() != 2)
    {
        return {};
    }

    auto const& names = header.ext.v1().ext.v2().activatedProtocolChanges;
    return std::vector<std::string>(names.begin(), names.end());
}

bool
isProtocolChangeActive(LedgerHeader const& header, std::string const& name)
{
    // Deliberately a linear scan rather than a binary search: the list comes
    // from a header that may have been produced elsewhere, and treating an
    // unsorted list as sorted would silently report an active change as
    // inactive, diverging from nodes that saw it.
    auto const activated = getActivatedProtocolChanges(header);
    return std::find(activated.begin(), activated.end(), name) !=
           activated.end();
}

std::vector<std::string>
getUnknownActivatedProtocolChanges(LedgerHeader const& header)
{
    std::vector<std::string> unknown;
    for (auto const& name : getActivatedProtocolChanges(header))
    {
        if (!isKnownProtocolChange(name))
        {
            unknown.push_back(name);
        }
    }
    return unknown;
}

void
activateProtocolChange(LedgerHeader& header, std::string const& name)
{
    if (header.ext.v() == 0)
    {
        header.ext.v(1);
    }
    if (header.ext.v1().ext.v() == 0)
    {
        header.ext.v1().ext.v(2);
    }

    auto& names = header.ext.v1().ext.v2().activatedProtocolChanges;
    auto it = std::lower_bound(names.begin(), names.end(), name);
    if (it != names.end() && *it == name)
    {
        // Already activated; nothing to do.
        return;
    }
    if (names.size() >= MAX_ACTIVATED_PROTOCOL_CHANGES)
    {
        throw std::runtime_error(
            "too many activated protocol changes for the ledger header");
    }
    names.insert(it, name);
}
}
