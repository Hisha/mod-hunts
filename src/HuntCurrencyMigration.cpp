#include "HuntCurrencyMigration.h"
#include "Field.h"
#include "QueryResult.h"
#include <algorithm>
namespace
{
std::string N(std::uint64_t n){return std::to_string(n);}
void Guard(CharacterDatabaseTransaction const& tx,std::string const& condition)
{
    tx->Append("INSERT INTO hunt_currency_realm(id,migration_version,state,seal_item) SELECT 1,1,'GUARD',0 WHERE NOT ("+condition+")");
}
}

bool CommitAndWait(CharacterDatabaseTransaction const& tx)
{
    auto callback = CharacterDatabase.AsyncCommitTransaction(tx);

    if (!callback.m_future.valid())
        return false;

    return callback.m_future.get();
}

bool HuntCurrencyMigration::Run(std::uint32_t seal,std::uint32_t stack,Prepare const& prepare,
    std::function<void()> const& committed,std::string& error)
{
    auto state=CharacterDatabase.Query("SELECT r.state FROM (SELECT 1) s LEFT JOIN hunt_currency_realm r ON r.id=1");
    if(!state){error="Cannot read migration state";return false;}
    if(state->Fetch()[0].IsNull())
    {
        auto tx=CharacterDatabase.BeginTransaction();
        tx->Append("INSERT INTO hunt_currency_realm(id,migration_version,state,seal_item) VALUES(1,1,'MIGRATING',"+N(seal)+")");
        tx->Append("UPDATE hunt_stats SET huntmaster_seals=huntmaster_seals");
        Guard(tx,"NOT EXISTS(SELECT 1 FROM hunt_currency_delivery)");
        // All existing stats, including zero balances, are snapshotted atomically.
        tx->Append("INSERT INTO hunt_currency_delivery(guid,legacy_amount) SELECT guid,huntmaster_seals FROM hunt_stats");
        tx->Append("UPDATE hunt_currency_realm SET snapshot_count=(SELECT COUNT(*) FROM hunt_currency_delivery),"
            "snapshot_total=(SELECT COALESCE(SUM(legacy_amount),0) FROM hunt_currency_delivery) WHERE id=1");
		if (!CommitAndWait(tx))
		{
		    error = "Migration snapshot transaction failed";
		    return false;
		}
        state=CharacterDatabase.Query("SELECT state FROM hunt_currency_realm WHERE id=1 AND seal_item="+N(seal));
        if(!state){error="Migration snapshot did not commit; no virtual balances changed";return false;}
    }
    if(state->Fetch()[0].Get<std::string>()=="NATIVE")return true;
    if(!stack){error="Invalid Seal stack size";return false;}
    for(unsigned batch=0;batch<10000;++batch)
    {
        // An aggregate seed distinguishes an empty queue from a failed SQL query.
        auto q=CharacterDatabase.Query("SELECT d.guid,d.legacy_amount,d.delivered_amount,c.guid FROM (SELECT 1) s LEFT JOIN "
            "(SELECT * FROM hunt_currency_delivery WHERE delivered_amount<legacy_amount ORDER BY guid LIMIT 1) d ON 1=1 "
            "LEFT JOIN characters c ON c.guid=d.guid");
        if(!q){error="Cannot read pending migration deliveries";return false;}
        if(q->Fetch()[0].IsNull())
        {
            auto tx=CharacterDatabase.BeginTransaction();
            tx->Append("UPDATE hunt_currency_realm SET id=id WHERE id=1");
            Guard(tx,"EXISTS(SELECT 1 FROM hunt_currency_realm WHERE id=1 AND state='MIGRATING' AND seal_item="+N(seal)+")");
            Guard(tx,"NOT EXISTS(SELECT 1 FROM hunt_currency_delivery WHERE delivered_amount<>legacy_amount)");
            Guard(tx,"(SELECT snapshot_count FROM hunt_currency_realm WHERE id=1)=(SELECT COUNT(*) FROM hunt_currency_delivery) AND "
                "(SELECT snapshot_total FROM hunt_currency_realm WHERE id=1)=(SELECT COALESCE(SUM(legacy_amount),0) FROM hunt_currency_delivery)");
            tx->Append("UPDATE hunt_currency_realm SET state='NATIVE',completed_at=NOW() WHERE id=1");
			if (!CommitAndWait(tx))
			{
			    error = "Migration completion transaction failed";
			    return false;
			}
            auto done=CharacterDatabase.Query("SELECT state FROM hunt_currency_realm WHERE id=1");
            if(done && done->Fetch()[0].Get<std::string>()=="NATIVE")return true;
            error="Realm completion receipt could not be verified";return false;
        }
        auto f=q->Fetch();auto guid=f[0].Get<std::uint32_t>();auto total=f[1].Get<std::uint32_t>();auto delivered=f[2].Get<std::uint32_t>();
        if(f[3].IsNull()){error="Legacy balance belongs to missing character "+N(guid)+"; administrative repair required";return false;}
        auto amount=std::uint32_t(std::min<std::uint64_t>(total-delivered,std::uint64_t(stack)*120));
        auto tx=CharacterDatabase.BeginTransaction();
        tx->Append("UPDATE hunt_currency_realm SET id=id WHERE id=1");
        Guard(tx,"EXISTS(SELECT 1 FROM hunt_currency_realm WHERE id=1 AND state='MIGRATING' AND seal_item="+N(seal)+")");
        Guard(tx,"EXISTS(SELECT 1 FROM hunt_currency_delivery WHERE guid="+N(guid)+" AND legacy_amount="+N(total)+" AND delivered_amount="+N(delivered)+")");
        if(!prepare(tx,guid,amount,error))return false;
        tx->Append("UPDATE hunt_currency_delivery SET delivered_amount="+N(delivered+amount)+" WHERE guid="+N(guid));
		if (!CommitAndWait(tx))
		{
		    error = "Physical Seal delivery transaction failed; restart resumes pending migration";
		    return false;
		}
        auto receipt=CharacterDatabase.Query("SELECT delivered_amount FROM hunt_currency_delivery WHERE guid="+N(guid));
        if(!receipt||receipt->Fetch()[0].Get<std::uint32_t>()!=delivered+amount)
        {error="Delivery commit unverified; restart resumes durable pending work without reissuing receipts";return false;}
        committed();
    }
    error="Migration batch limit reached; restart resumes pending deliveries";return false;
}
