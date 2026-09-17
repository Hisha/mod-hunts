#include "HuntCurrencyService.h"
#include "FakeCore.h"
#include <iostream>
void SQL(std::string const& s){if(!CharacterDatabase.Execute(s))throw std::runtime_error(CharacterDatabase.lastError);}
uint64 V(std::string const& s){auto q=CharacterDatabase.Query(s);assert(q);return q->Fetch()[0].Get<uint64>();}
struct Provider:WorldScript,ContentCapabilitiesV1::Provider
{
    ContentCapabilitiesV1::Result result=ContentCapabilitiesV1::Result::Ready;
    ContentCapabilitiesV1::Result Resolve(std::string const& package,std::string const& symbol,std::vector<ContentCapabilitiesV1::Resource>& rs,
        ContentCapabilitiesV1::Vendor& v,std::string& reason)const override
    {
        assert(package=="mod-hunts" && symbol=="seal-proof-vendor");
        if(result!=ContentCapabilitiesV1::Result::Ready){reason="Injected unresolved/inactive native content";return result;}
        rs[0].value=60001;rs[1].value=77;v={123456,40717,77};return result;
    }
};
int main(int argc,char** argv)
{
    assert(argc==2);CharacterDatabase.Connect(argv[1]);
    auto& service=sHuntCurrency;service.Initialize();assert(service.IsLegacy());
    Player player;player.guid.n=42;SQL("INSERT INTO characters VALUES(42)");SQL("INSERT INTO hunt_stats VALUES(42,20)");
    assert(service.GetBalance(&player)==20 && service.VirtualAward(2)==2);
    SQL("UPDATE hunt_stats SET huntmaster_seals=huntmaster_seals+"+std::to_string(service.VirtualAward(2))+" WHERE guid=42");
    assert(service.GetBalance(&player)==22);assert(service.SpendLegacy(&player,5));assert(service.GetBalance(&player)==17);
    service.RefundLegacy(&player,5);assert(service.GetBalance(&player)==22);
    Provider provider;ScriptRegistry<WorldScript>::ScriptPointerList[1]=&provider;
    provider.result=ContentCapabilitiesV1::Result::Inactive;service.Initialize();assert(service.IsLegacy());
    provider.result=ContentCapabilitiesV1::Result::Invalid;service.Initialize();assert(!service.Available());
    provider.result=ContentCapabilitiesV1::Result::Ready;
    manager.items[60001]={200,1,0,0};manager.items[40717]={1,1,0,1};
    Cost cost;cost.reqitem[0]=60001;cost.reqitemcount[0]=5;sItemExtendedCostStore.rows[77]=cost;
    service.Initialize();assert(service.IsNative() && service.SealItem()==60001);
    assert(V("SELECT SUM(count) FROM item_instance WHERE itemEntry=60001")==22);
    assert(V("SELECT huntmaster_seals FROM hunt_stats WHERE guid=42")==22);
    assert(V("SELECT COUNT(*) FROM mail_items")==1 && V("SELECT COUNT(*) FROM mail")==1);
    player.inventory[60001]=9;assert(service.GetBalance(&player)==9 && service.VirtualAward(2)==0);
    assert(!service.SpendLegacy(&player,5));
    SQL("INSERT INTO hunt_runtime VALUES(42)");std::string error;
    auto stats="UPDATE hunt_stats SET huntmaster_seals=huntmaster_seals+"+std::to_string(service.VirtualAward(2))+" WHERE guid=42";
    assert(service.CompleteNativeHunt(&player,2,stats,error));assert(player.mails.size()==1 && player.items.size()==1);
    assert(V("SELECT SUM(count) FROM item_instance WHERE itemEntry=60001")==24);
    assert(V("SELECT huntmaster_seals FROM hunt_stats WHERE guid=42")==22);
    assert(V("SELECT COUNT(*) FROM hunt_runtime")==0);
    service.Initialize();assert(service.IsNative());assert(V("SELECT SUM(count) FROM item_instance")==24);
    SQL("INSERT INTO hunt_runtime VALUES(42)");
    SQL("CREATE TRIGGER reject_mail BEFORE INSERT ON mail FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='delivery failure'");
    assert(!service.CompleteNativeHunt(&player,2,stats,error));assert(!service.Available());
    assert(player.mails.size()==1 && V("SELECT SUM(count) FROM item_instance")==24 && V("SELECT COUNT(*) FROM hunt_runtime")==1);
    SQL("DROP TRIGGER reject_mail");service.Initialize();assert(service.IsNative());
    assert(service.CompleteNativeHunt(&player,2,stats,error));assert(V("SELECT SUM(count) FROM item_instance")==26);
    ScriptRegistry<WorldScript>::ScriptPointerList.clear();service.Initialize();assert(!service.Available());
    assert(!service.SpendLegacy(&player,1));assert(V("SELECT huntmaster_seals FROM hunt_stats")==22);
    std::cout<<"PASS production currency service: standalone, inactive/invalid capability, dynamic IDs, realm migration, physical balance/award, preserved virtual balance, atomic mail rollback, retry and no downgrade to virtual\n";
}
