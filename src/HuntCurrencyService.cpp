#include "HuntCurrencyService.h"
#include "HuntCurrencyMigration.h"
#include "HuntCharacterTransaction.h"
#include "ScriptMgr.h"
#include "DatabaseEnv.h"
#include "Player.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "Mail.h"
#include "MailMgr.h"
#include "GameTime.h"
#include "DBCStores.h"
#include "Log.h"
#include <algorithm>
#include <limits>
#include <memory>
#include <vector>
namespace
{
std::string N(std::uint64_t n){return std::to_string(n);}
void Guard(CharacterDatabaseTransaction const& tx,std::string const& condition)
{
    // The singleton must exist. Duplicate primary key aborts the ENTIRE tx.
    tx->Append("INSERT INTO hunt_currency_realm(id,migration_version,state,seal_item) SELECT 1,1,'GUARD',0 WHERE NOT ("+condition+")");
}
struct Delivery
{
    std::vector<std::unique_ptr<Item>> items;
    std::vector<std::unique_ptr<Mail>> mails;
    bool Prepare(CharacterDatabaseTransaction const& tx,std::uint32_t guid,std::uint32_t entry,std::uint32_t amount,std::string& error)
    {
        auto proto=sObjectMgr->GetItemTemplate(entry);
        if(!proto || proto->Stackable<=0){error="Resolved Seal item cannot be stacked";return false;}
        std::uint32_t const stack=std::uint32_t(proto->Stackable);
        if(std::uint64_t(amount)>std::uint64_t(stack)*120){error="Delivery batch exceeds 120 stacks";return false;}
        while(amount)
        {
            auto mail=std::make_unique<Mail>();
            mail->messageID=sObjectMgr->GenerateMailID();mail->messageType=MAIL_NORMAL;
            mail->stationery=MAIL_STATIONERY_GM;mail->mailTemplateId=0;mail->sender=0;mail->receiver=guid;
            mail->subject="Huntmaster's Seals";
            mail->body="Your earned Huntmaster's Seals. Take the attachments to use your physical currency.";
            mail->deliver_time=GameTime::GetGameTime().count();
            // Last representable database timestamp; no normal 30-day expiry.
            mail->expire_time=std::numeric_limits<std::uint32_t>::max();
            mail->money=0;mail->COD=0;mail->checked=MAIL_CHECK_MASK_COPIED;mail->state=MAIL_STATE_UNCHANGED;
            for(unsigned i=0;i<MAX_MAIL_ITEMS && amount;++i)
            {
                auto count=std::min(stack,amount);
                std::unique_ptr<Item> item(Item::CreateItem(entry,count,nullptr));
                if(!item){error="Could not create complete physical Seal delivery";return false;}
                item->SetOwnerGUID(ObjectGuid::Create<HighGuid::Player>(guid));
                item->SaveToDB(tx);
                mail->AddItem(item->GetGUID().GetCounter(),entry);
                auto attachment=CharacterDatabase.GetPreparedStatement(CHAR_INS_MAIL_ITEM);
                attachment->SetData(0,mail->messageID);attachment->SetData(1,item->GetGUID().GetCounter());attachment->SetData(2,guid);
                tx->Append(attachment);items.push_back(std::move(item));amount-=count;
            }
            auto stmt=CharacterDatabase.GetPreparedStatement(CHAR_INS_MAIL);
            stmt->SetData(0,mail->messageID);stmt->SetData(1,mail->messageType);stmt->SetData(2,std::int8_t(mail->stationery));
            stmt->SetData(3,mail->mailTemplateId);stmt->SetData(4,mail->sender);stmt->SetData(5,guid);
            stmt->SetData(6,mail->subject);stmt->SetData(7,mail->body);stmt->SetData(8,true);
            stmt->SetData(9,std::uint32_t(mail->expire_time));stmt->SetData(10,std::uint32_t(mail->deliver_time));
            stmt->SetData(11,std::uint32_t(0));stmt->SetData(12,std::uint32_t(0));stmt->SetData(13,std::uint8_t(mail->checked));
            tx->Append(stmt);mails.push_back(std::move(mail));
        }
        return true;
    }
    void Publish(Player* player)
    {
        // Unlike MailDraft::SendMailTo, publish ONLINE caches only after verified commit.
        if(mails.empty())return;
        sMailMgr->OnMailSent(mails.front()->receiver);
        if(!player)return;
        for(auto& item:items)player->AddMItem(item.release());
        for(auto& mail:mails){player->AddNewMailDeliverTime(mail->deliver_time);player->AddMail(mail.release());}
    }
};
}
HuntCurrencyService& HuntCurrencyService::Instance(){static HuntCurrencyService service;return service;}
void HuntCurrencyService::Pause(std::string const& reason)
{
    _mode=Mode::Paused;_reason=reason;
    LOG_ERROR("module.hunts","mod-hunts: currency mode = PAUSED; {}. Preserved virtual balances are not spendable.",reason);
}
void HuntCurrencyService::Initialize()
{
    // Startup only; no login hooks, session approvals, polling or runtime fallback.
    auto schema=CharacterDatabase.Query("SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME IN "
        "('hunt_currency_realm','hunt_currency_delivery','hunt_stats','hunt_runtime','characters','item_instance','mail','mail_items') AND ENGINE='InnoDB'");
    if(!schema||schema->Fetch()[0].Get<std::uint64_t>()!=8){Pause("Apply the Hunt currency character SQL; required InnoDB schema is unavailable");return;}
    auto state=CharacterDatabase.Query("SELECT r.state,r.migration_version,r.seal_item FROM (SELECT 1) s LEFT JOIN hunt_currency_realm r ON r.id=1");
    if(!state){Pause("Cannot read realm migration state");return;}
    bool const latched=!state->Fetch()[0].IsNull();
    ContentCapabilitiesV1::Provider const* provider=nullptr;
    for(auto const& script:ScriptRegistry<WorldScript>::ScriptPointerList)
        if(auto candidate=dynamic_cast<ContentCapabilitiesV1::Provider const*>(script.second))
        {if(provider){Pause("Multiple Content Manager capability providers");return;}provider=candidate;}
    std::vector<ContentCapabilitiesV1::Resource> resources{{"seal","item.id",0},{"seal-cost-5","item-extended-cost.id",0}};
    auto result=ContentCapabilitiesV1::Result::Inactive;
    _reason="Content Manager unavailable; native Hunt content never activated";
    if(provider)result=provider->Resolve("mod-hunts","seal-proof-vendor",resources,_vendor,_reason);
    if(result!=ContentCapabilitiesV1::Result::Ready)
    {
        if(latched || result==ContentCapabilitiesV1::Result::Invalid){Pause(_reason);return;}
        _mode=Mode::Legacy;LOG_INFO("module.hunts","mod-hunts: currency mode = LEGACY; {}",_reason);return;
    }
    _seal=resources[0].value;
    auto cost=sItemExtendedCostStore.LookupEntry(resources[1].value);
    if(!_seal||!cost||_vendor.extendedCostId!=resources[1].value||cost->reqitem[0]!=_seal||cost->reqitemcount[0]!=5
        ||cost->reqhonorpoints||cost->reqarenapoints||cost->reqarenaslot||cost->reqpersonalarenarating)
    {Pause("Native proof requires exactly five resolved physical Seals");return;}
    for(unsigned i=1;i<MAX_ITEM_EXTENDED_COST_REQUIREMENTS;++i)
        if(cost->reqitem[i]||cost->reqitemcount[i]){Pause("Native proof has extra cost requirements");return;}
    auto merchandise=sObjectMgr->GetItemTemplate(_vendor.itemEntry);
    if(!merchandise||merchandise->BuyCount!=1||merchandise->BuyPrice!=0||merchandise->Bonding!=BIND_WHEN_PICKED_UP)
    {Pause("Native proof merchandise must have one BOP unit and no gold price");return;}
    if(latched && (state->Fetch()[1].Get<std::uint32_t>()!=1 || state->Fetch()[2].Get<std::uint32_t>()!=_seal))
    {Pause("Realm migration version/Seal identity mismatch; explicit administrative repair required");return;}
    if(latched && state->Fetch()[0].Get<std::string>()!="MIGRATING" && state->Fetch()[0].Get<std::string>()!="NATIVE")
    {Pause("Unknown realm migration state");return;}
    if(!Migrate()){Pause(_reason);return;}
    _mode=Mode::Native;_reason="Native activation and durable realm migration complete";
    LOG_INFO("module.hunts","mod-hunts: currency mode = NATIVE; {}",_reason);
}
bool HuntCurrencyService::Migrate()
{
    if(!ObjectAccessor::GetPlayers().empty()){_reason="Realm migration requires startup before player logins";return false;}
    auto proto=sObjectMgr->GetItemTemplate(_seal);
    std::unique_ptr<Delivery> delivery;
    return HuntCurrencyMigration::Run(_seal,std::uint32_t(proto->Stackable),
        [&](auto const& tx,std::uint32_t guid,std::uint32_t amount,std::string& error) {
            delivery=std::make_unique<Delivery>();return delivery->Prepare(tx,guid,_seal,amount,error);
        },[&]{delivery->Publish(nullptr);delivery.reset();},_reason);
}
std::uint32_t HuntCurrencyService::GetBalance(Player const* player) const
{
    if(!player||!Available())return 0;
    if(IsNative())return player->GetItemCount(_seal,false);
    auto q=CharacterDatabase.Query("SELECT huntmaster_seals FROM hunt_stats WHERE guid="+N(player->GetGUID().GetCounter()));
    return q?q->Fetch()[0].Get<std::uint32_t>():0;
}
bool HuntCurrencyService::SpendLegacy(Player* player,std::uint32_t amount)
{
    if(!IsLegacy()||!player||GetBalance(player)<amount)return false;
    CharacterDatabase.DirectExecute("UPDATE hunt_stats SET huntmaster_seals=huntmaster_seals-"+N(amount)+" WHERE guid="+N(player->GetGUID().GetCounter())+" AND huntmaster_seals>="+N(amount));
    return true;
}
void HuntCurrencyService::RefundLegacy(Player* player,std::uint32_t amount)
{
    if(IsLegacy()&&player)CharacterDatabase.DirectExecute("UPDATE hunt_stats SET huntmaster_seals=huntmaster_seals+"+N(amount)+" WHERE guid="+N(player->GetGUID().GetCounter()));
}
bool HuntCurrencyService::CompleteNativeHunt(Player* player,std::uint32_t amount,std::string const& statsSql,std::string& error)
{
    if(!IsNative()||!player){error="Seal operations are paused";return false;}
    auto serial=CharacterDatabase.Query("SELECT reward_serial FROM hunt_currency_realm WHERE id=1 AND state='NATIVE'");
    if(!serial){Pause("Cannot read native reward receipt");error=_reason;return false;}
    auto before=serial->Fetch()[0].Get<std::uint64_t>();
    if(before==std::numeric_limits<std::uint64_t>::max()){Pause("Native reward receipt exhausted");error=_reason;return false;}
    auto guid=player->GetGUID().GetCounter();auto tx=CharacterDatabase.BeginTransaction();
    tx->Append("UPDATE hunt_currency_realm SET id=id WHERE id=1");
    Guard(tx,"EXISTS(SELECT 1 FROM hunt_currency_realm WHERE id=1 AND state='NATIVE' AND seal_item="+N(_seal)+" AND reward_serial="+N(before)+")");
    Guard(tx,"EXISTS(SELECT 1 FROM hunt_runtime WHERE guid="+N(guid)+")");
    Delivery delivery;
    if(!delivery.Prepare(tx,guid,_seal,amount,error)){Pause(error);return false;}
    tx->Append(statsSql);tx->Append("DELETE FROM hunt_runtime WHERE guid="+N(guid));
    auto firstMail=delivery.mails.empty()?0:delivery.mails.front()->messageID;
    tx->Append("UPDATE hunt_currency_realm SET reward_serial="+N(before+1)+",last_reward_guid="+N(guid)+",last_reward_mail="+N(firstMail)+" WHERE id=1");
    if (!hunts::CommitCharacterTransactionAndWait(tx))
    {
        Pause("Native Seal delivery transaction failed or completion could not be confirmed; review before retrying");
        error = _reason;
        return false;
    }
    auto receipt=CharacterDatabase.Query("SELECT reward_serial FROM hunt_currency_realm WHERE id=1 AND last_reward_guid="+N(guid)+" AND last_reward_mail="+N(firstMail));
    if(!receipt||receipt->Fetch()[0].Get<std::uint64_t>()!=before+1)
    {Pause("Native completion commit unverified; restart before further Seal operations");error=_reason;return false;}
    delivery.Publish(player);return true;
}
