#pragma once
#include "DatabaseEnv.h"
#include <map>
#include <chrono>
#include <cassert>
using uint8=std::uint8_t;using uint64=std::uint64_t;
constexpr unsigned MAX_MAIL_ITEMS=12,MAX_ITEM_EXTENDED_COST_REQUIREMENTS=5;
constexpr unsigned MAIL_NORMAL=0,MAIL_STATIONERY_GM=61,MAIL_CHECK_MASK_COPIED=16,MAIL_STATE_UNCHANGED=1,BIND_WHEN_PICKED_UP=1;
#define LOG_ERROR(...) ((void)0)
#define LOG_INFO(...) ((void)0)
enum class HighGuid { Player };
struct ObjectGuid { using LowType=uint32;uint32 n=0;template<HighGuid>static ObjectGuid Create(uint32 n){return {n};}uint32 GetCounter()const{return n;} };
struct ItemTemplate { int Stackable=200;unsigned BuyCount=1,BuyPrice=0,Bonding=0; };
struct ObjectManager { uint32 mail=1000;std::map<uint32,ItemTemplate> items;ItemTemplate const* GetItemTemplate(uint32 id)const{auto i=items.find(id);return i==items.end()?nullptr:&i->second;}uint32 GenerateMailID(){return ++mail;} };
inline ObjectManager manager;
#define sObjectMgr (&manager)
struct Item { inline static uint32 next=2000;uint32 id,count;ObjectGuid guid,owner;
 static Item* CreateItem(uint32 id,uint32 count,void*){return new Item{id,count,{++next},{}};}
 void SetOwnerGUID(ObjectGuid g){owner=g;}ObjectGuid GetGUID()const{return guid;}uint32 GetEntry()const{return id;}
 void SaveToDB(CharacterDatabaseTransaction const& tx){tx->Append("INSERT INTO item_instance(guid,owner_guid,itemEntry,count) VALUES("+std::to_string(guid.n)+","+std::to_string(owner.n)+","+std::to_string(id)+","+std::to_string(count)+")");}
};
struct Mail { uint32 messageID=0,messageType=0,stationery=0,mailTemplateId=0,sender=0,receiver=0,money=0,COD=0,checked=0,state=0;std::string subject,body;time_t deliver_time=0,expire_time=0;void AddItem(uint32,uint32){} };
struct Player { ObjectGuid guid;std::map<uint32,uint32> inventory;std::vector<Item*> items;std::vector<Mail*> mails;
 ~Player(){for(auto p:items)delete p;for(auto p:mails)delete p;}
 ObjectGuid GetGUID()const{return guid;}uint32 GetItemCount(uint32 id,bool bank)const{assert(!bank);auto i=inventory.find(id);return i==inventory.end()?0:i->second;}
 void AddMItem(Item* p){items.push_back(p);}void AddMail(Mail* p){mails.push_back(p);}void AddNewMailDeliverTime(time_t){}
};
struct MailManager { void OnMailSent(uint32){} };inline MailManager mailManager;
#define sMailMgr (&mailManager)
namespace GameTime { inline std::chrono::seconds GetGameTime(){return std::chrono::seconds(1800000000);} }
namespace ObjectAccessor { inline std::map<uint32,Player*> players;inline auto const& GetPlayers(){return players;} }
struct WorldScript { virtual ~WorldScript()=default; };
template<class T>struct ScriptRegistry { inline static std::map<uint32,T*> ScriptPointerList; };
struct Cost {uint32 reqitem[5]{},reqitemcount[5]{},reqhonorpoints=0,reqarenapoints=0,reqarenaslot=0,reqpersonalarenarating=0;};
struct Costs {std::map<uint32,Cost> rows;Cost const* LookupEntry(uint32 id){auto i=rows.find(id);return i==rows.end()?nullptr:&i->second;} };inline Costs sItemExtendedCostStore;
