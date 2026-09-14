// Copyright 2026 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#pragma once

#include "xdr/Stellar-ledger.h"

#include <string>
#include <vector>

namespace stellar
{

// A protocol change that validators vote on by name.
//
// Protocol changes are identified by name rather than by the protocol version
// number they produce. Accepting a change increments `ledgerVersion` by one, so
// the protocol version records how many changes have been activated rather than
// identifying any particular one. This lets several changes be proposed at the
// same time, with whichever one the quorum accepts first becoming the next
// protocol version.
struct ProtocolChange
{
    std::string mName;
    std::string mDescription;
};

// The example change used to exercise the mechanism: once activated, the
// BumpSequence operation is no longer supported.
extern char const* const PROTOCOL_CHANGE_DISABLE_BUMP_SEQUENCE;

#ifdef BUILD_TESTS
// A change with no behavioural effect, used by tests to exercise proposing two
// changes at once and accepting them in either order.
extern char const* const PROTOCOL_CHANGE_TEST_NO_OP;
#endif

// The protocol changes this build knows how to apply. A node never votes for,
// and never accepts, a change that is not in this list, so a change is only
// activated once the quorum is running software that implements it.
std::vector<ProtocolChange> const& getKnownProtocolChanges();

bool isKnownProtocolChange(std::string const& name);

// Upper bound on activated changes, matching `activatedProtocolChanges<128>`
// in the XDR.
constexpr size_t MAX_ACTIVATED_PROTOCOL_CHANGES = 128;

// The changes activated on this ledger, sorted lexicographically.
std::vector<std::string>
getActivatedProtocolChanges(LedgerHeader const& header);

bool isProtocolChangeActive(LedgerHeader const& header,
                            std::string const& name);

// Activated changes that this build does not know how to apply. A node cannot
// apply a ledger unless this is empty, since it would not reproduce the
// behaviour the rest of the network agreed on.
std::vector<std::string>
getUnknownActivatedProtocolChanges(LedgerHeader const& header);

// Records `name` as activated in `header`, keeping the list sorted. Does not
// modify `ledgerVersion`; callers are responsible for incrementing it.
void activateProtocolChange(LedgerHeader& header, std::string const& name);
}
