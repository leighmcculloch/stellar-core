// Copyright 2022 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "ledger/LedgerCloseMetaFrame.h"
#include "crypto/SHA.h"
#include "transactions/TransactionMetaFrame.h"
#include "util/GlobalChecks.h"
#include "util/ProtocolVersion.h"

namespace stellar
{
LedgerCloseMetaFrame::LedgerCloseMetaFrame(uint32_t protocolVersion)
{
    // The LedgerCloseMeta v() switch can be in 3 positions, 0, 1, and 2. We
    // currently support all of these cases, depending on both compile time
    // and runtime conditions.
    mVersion = 0;

#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
    if (protocolVersionStartsFrom(protocolVersion, ProtocolVersion::V_20))
    {
        mVersion = 2;
    }
    else
#endif
        if (protocolVersionStartsFrom(protocolVersion,
                                      GENERALIZED_TX_SET_PROTOCOL_VERSION))
    {
        mVersion = 1;
    }
    mLedgerCloseMeta.v(mVersion);
}

LedgerHeaderHistoryEntry&
LedgerCloseMetaFrame::ledgerHeader()
{
    switch (mVersion)
    {
    case 0:
        return mLedgerCloseMeta.v0().ledgerHeader;
    case 1:
        return mLedgerCloseMeta.v1().ledgerHeader;
#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
    case 2:
        return mLedgerCloseMeta.v2().ledgerHeader;
#endif
    default:
        releaseAssert(false);
    }
}

void
LedgerCloseMetaFrame::reserveTxProcessing(size_t n)
{
    switch (mVersion)
    {
    case 0:
        mLedgerCloseMeta.v0().txProcessing.reserve(n);
        break;
    case 1:
        mLedgerCloseMeta.v1().txProcessing.reserve(n);
        break;
#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
    case 2:
        mLedgerCloseMeta.v2().txProcessing.reserve(n);
        break;
#endif
    default:
        releaseAssert(false);
    }
}

void
LedgerCloseMetaFrame::pushTxProcessingEntry()
{
    switch (mVersion)
    {
    case 0:
        mLedgerCloseMeta.v0().txProcessing.emplace_back();
        break;
    case 1:
        mLedgerCloseMeta.v1().txProcessing.emplace_back();
        break;
#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
    case 2:
        mLedgerCloseMeta.v2().txProcessing.emplace_back();
        break;
#endif
    default:
        releaseAssert(false);
    }
}

void
LedgerCloseMetaFrame::setLastTxProcessingFeeProcessingChanges(
    LedgerEntryChanges const& changes)
{
    switch (mVersion)
    {
    case 0:
        mLedgerCloseMeta.v0().txProcessing.back().feeProcessing = changes;
        break;
    case 1:
        mLedgerCloseMeta.v1().txProcessing.back().feeProcessing = changes;
        break;
#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
    case 2:
        mLedgerCloseMeta.v2().txProcessing.back().feeProcessing = changes;
        break;
#endif
    default:
        releaseAssert(false);
    }
}

void
LedgerCloseMetaFrame::setTxProcessingMetaAndResultPair(
    TransactionMeta const& tm, TransactionResultPair&& rp, int index)
{
    switch (mVersion)
    {
    case 0:
    {
        auto& txp = mLedgerCloseMeta.v0().txProcessing.at(index);
        txp.txApplyProcessing = tm;
        txp.result = std::move(rp);
    }
    break;
    case 1:
    {
        auto& txp = mLedgerCloseMeta.v1().txProcessing.at(index);
        txp.txApplyProcessing = tm;
        txp.result = std::move(rp);
    }
    break;
#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
    case 2:
    {
        auto& txp = mLedgerCloseMeta.v2().txProcessing.at(index);
        txp.txApplyProcessing = tm;
        txp.result = std::move(rp);
    }
    break;
#endif
    default:
        releaseAssert(false);
    }
}

xdr::xvector<UpgradeEntryMeta>&
LedgerCloseMetaFrame::upgradesProcessing()
{
    switch (mVersion)
    {
    case 0:
        return mLedgerCloseMeta.v0().upgradesProcessing;
    case 1:
        return mLedgerCloseMeta.v1().upgradesProcessing;
#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
    case 2:
        return mLedgerCloseMeta.v2().upgradesProcessing;
#endif
    default:
        releaseAssert(false);
    }
}

void
LedgerCloseMetaFrame::populateTxSet(TxSetFrame const& txSet)
{
    switch (mVersion)
    {
    case 0:
        txSet.toXDR(mLedgerCloseMeta.v0().txSet);
        break;
    case 1:
        txSet.toXDR(mLedgerCloseMeta.v1().txSet);
        break;
#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
    case 2:
        txSet.toXDR(mLedgerCloseMeta.v2().txSet);
        break;
#endif
    default:
        releaseAssert(false);
    }
}

#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
void
LedgerCloseMetaFrame::setTotalByteSizeOfBucketList(uint64_t size)
{
    switch (mVersion)
    {
    case 2:
        mLedgerCloseMeta.v2().totalByteSizeOfBucketList = size;
        break;
    default:
        releaseAssert(false);
    }
}

void
LedgerCloseMetaFrame::populateEvictedEntries(
    LedgerEntryChanges const& evictionChanges)
{
    releaseAssert(mVersion == 2);
    for (auto const& change : evictionChanges)
    {
        switch (change.type())
        {
        case LEDGER_ENTRY_CREATED:
            throw std::runtime_error("unexpected create in eviction meta");
        case LEDGER_ENTRY_STATE:
            continue;
        case LEDGER_ENTRY_UPDATED:
            // The scan also updates the eviction iterator, but should only
            // update the eviction iterator
            releaseAssert(change.updated().data.type() == CONFIG_SETTING);
            continue;
        case LEDGER_ENTRY_REMOVED:
            auto const& key = change.removed();
            releaseAssert(isTemporaryEntry(key) || key.type() == EXPIRATION);
            mLedgerCloseMeta.v2().evictedTemporaryLedgerKeys.push_back(key);
            break;
        }
    }
}
#endif

LedgerCloseMeta const&
LedgerCloseMetaFrame::getXDR() const
{
    return mLedgerCloseMeta;
}
}
