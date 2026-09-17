#include "DatabaseEnv.h"
#include "ContentManager.h"
#include "ContentBuildService.h"
#include "ContentBuildHash.h"
#include "ContentPackage.h"
#include "ContentAllocationRegistry.h"
#include "ContentServerBundle.h"
#include "ContentPackageRegistry.h"
#include "ItemExtendedCostDbc.h"
#include "DbcReader.h"
#include "third_party/json/json.hpp"
#include <StormLib.h>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <fstream>

namespace { std::filesystem::path root, hunts, aq; }
// Only config/discovery are injected. Build, staging, allocation registry, parity and MPQ are production code.
ContentManager& ContentManager::Instance(){static ContentManager manager;return manager;}
void ContentManager::LoadConfig()
{
    _enabled=true;_clientBuild=12340;_baselineDbcDirectory=(root/"baseline").string();
    _workDirectory=(root/"staging").string();_outputDirectory=(root/"output").string();
    _itemBaselineSha256.clear(); _currencyTypesBaselineSha256.clear(); // Normal operation needs no per-DBC pins.
}
bool ContentManager::IsEnabled() const{return _enabled;}
std::string const& ContentManager::GetBaselineDbcDirectory() const{return _baselineDbcDirectory;}
std::string const& ContentManager::GetWorkDirectory() const{return _workDirectory;}
std::string const& ContentManager::GetOutputDirectory() const{return _outputDirectory;}
std::string const& ContentManager::GetItemBaselineSha256() const{return _itemBaselineSha256;}
std::uint32_t ContentManager::GetClientBuild() const{return _clientBuild;}
std::vector<ContentPackageCandidate> ContentManager::ScanAvailablePackages() const
{return {{hunts,hunts.filename().string(),ContentPackageSource::Module,"hunts"},{aq,aq.filename().string(),ContentPackageSource::Module,"aq"}};}
static void SQL(std::string const& sql){if(!WorldDatabase.Execute(sql))throw std::runtime_error(WorldDatabase.lastError);}
static std::vector<std::uint8_t> Extract(std::filesystem::path const& path,char const* name)
{
    HANDLE archive=nullptr,file=nullptr;assert(SFileOpenArchive(path.c_str(),0,MPQ_OPEN_READ_ONLY,&archive));
    std::string mpqName(name);std::replace(mpqName.begin(),mpqName.end(),'/','\\');
    if (!SFileOpenFileEx(archive,mpqName.c_str(),SFILE_OPEN_FROM_MPQ,&file))
        throw std::runtime_error("Missing MPQ entry: "+mpqName);
    auto size=SFileGetFileSize(file,nullptr);std::vector<std::uint8_t> bytes(size);DWORD read=0;
    assert(SFileReadFile(file,bytes.data(),size,&read,nullptr)&&read==size);
    SFileCloseFile(file);SFileCloseArchive(archive);return bytes;
}
static unsigned Scalar(std::string const& sql)
{
    auto q=WorldDatabase.Query(sql);assert(q);return q->Fetch()[0].Get<unsigned>();
}
static void Install(std::filesystem::path const& path)
{
    auto v=ContentPackage(path).Validate();if(!v.valid)throw std::runtime_error(v.error);
    auto const& m=v.manifest;
    auto installed=ContentPackageRegistry().Install({m.packageKey,m.name,m.version,"fixture",path.string(),{}});
    if(!installed.success)throw std::runtime_error(installed.error);
}
static std::string Read(std::filesystem::path const& path)
{
    std::ifstream f(path);assert(f.is_open());return std::string(std::istreambuf_iterator<char>(f),{});
}
static std::set<std::string> Entries(std::filesystem::path const& path)
{
    HANDLE archive=nullptr;assert(SFileOpenArchive(path.c_str(),0,MPQ_OPEN_READ_ONLY,&archive));
    SFILE_FIND_DATA data{};auto search=SFileFindFirstFile(archive,"*",&data,nullptr);assert(search!=nullptr);
    std::set<std::string> names;
    do
    {
        std::string name=data.cFileName;
        if(!name.empty()&&name[0]!='('){std::replace(name.begin(),name.end(),'\\','/');names.insert(name);}
    }while(SFileFindNextFile(search,&data));
    SFileFindClose(search);SFileCloseArchive(archive);return names;
}
int main(int argc,char** argv)
{
    // Private test database only. No Apply/Activate/Publish call exists in this consumer fixture.
    assert(argc==6);WorldDatabase.Connect(argv[1]);CharacterDatabase.Connect(argv[1]);
    root=std::filesystem::absolute(argv[2]);hunts=std::filesystem::absolute(argv[3]);
    auto updated=std::filesystem::absolute(argv[4]);aq=std::filesystem::absolute(argv[5]);
    auto oldManifest=ContentPackage(hunts).Validate();auto newManifest=ContentPackage(updated).Validate();
    assert(oldManifest.valid&&newManifest.valid);
    auto const& old=oldManifest.manifest;auto const& next=newManifest.manifest;
    assert(old.version=="4.1.0"&&next.version=="4.2.0"&&old.extendedCosts.empty());
    assert(old.itemRows==next.itemRows&&old.serverItemRows==next.serverItemRows);
    assert(old.currencyRows==next.currencyRows&&old.currencyCategories==next.currencyCategories);
    assert(next.extendedCosts.size()==1);
    auto const& cost=next.extendedCosts[0];assert(cost.symbol=="seal-cost-5"&&cost.requirements.size()==1);
    assert(cost.requirements[0].packageKey=="mod-hunts"&&cost.requirements[0].symbol=="seal"&&cost.requirements[0].count==5);
    auto& manager=ContentManager::Instance();manager.LoadConfig();Install(hunts);Install(aq);
    auto text=ContentServerBundle::SqlIdentityText;std::string error;
    auto item=ContentBaselineRegistry::Inspect(root/"baseline",*FindDbcDescriptor(12340,"Item"));
    auto currency=ContentBaselineRegistry::Inspect(root/"baseline",*FindDbcDescriptor(12340,"CurrencyTypes"));
    auto category=ContentBaselineRegistry::Inspect(root/"baseline",*FindDbcDescriptor(12340,"CurrencyCategory"));
    auto extended=ContentBaselineRegistry::Inspect(root/"baseline",*FindDbcDescriptor(12340,"ItemExtendedCost"));
    // Numeric values below describe retained DATABASE FIXTURES, never EPF authoring.
    SQL("INSERT INTO content_manager_allocation VALUES ('Eitrigg','mod-hunts','seal','item.id',56807,'reserved',1,7,"+text(item.hash)+",1,1),"
        "('Eitrigg','mod-hunts','seal-currency','currency.known-bit',4,'reserved',1,7,"+text(currency.hash)+",1,1),"
        "('Eitrigg','mod-hunts','hunts','currency-category.id',5,'reserved',1,7,"+text(category.hash)+",1,1)");
    // Any vendor write would make the test fail, including an update that leaves row count unchanged.
    for(auto table:{"npc_vendor","game_event_npc_vendor"})
        for(auto action:{"INSERT","UPDATE","DELETE"})
            SQL("CREATE TRIGGER deny_"+std::string(table)+"_"+action+" BEFORE "+action+" ON "+table
                +" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='Consumer must not mutate vendors'");
    auto build=[&]{auto b=ContentBuildService().Build(manager,"Eitrigg");if(!b.success)throw std::runtime_error(b.error);return b;};
    auto previous=build();assert(previous.fileCount==4&&previous.packageCount==2);
    assert(Scalar("SELECT COUNT(*) FROM content_manager_baseline WHERE table_name='ItemExtendedCost'")==0);
    assert(ContentPackageRegistry().Uninstall("mod-hunts").success);
    assert(Scalar("SELECT COUNT(*) FROM content_manager_allocation")==3);
    hunts=updated;Install(hunts); // Deliberately no build between uninstall/install.
    auto packages=ContentPackageRegistry().GetInstalledPackages();assert(packages.success);
    for(auto const& p:packages.packages)if(p.packageKey=="mod-hunts")assert(p.version=="4.2.0");
    auto first=build();assert(first.fileCount==5&&first.packageCount==2);
    auto names=Entries(first.outputPath);
    std::set<std::string> expected={"DBFilesClient/Item.dbc","DBFilesClient/CurrencyTypes.dbc",
        "DBFilesClient/CurrencyCategory.dbc","DBFilesClient/ItemExtendedCost.dbc"};
    auto aqManifest=ContentPackage(aq).Validate().manifest;
    for(auto const& entry:aqManifest.content)expected.insert(entry.target);
    assert(names==expected);
    std::cout<<"Actual cumulative content ("<<names.size()<<" files):\n";
    for(auto const& name:names)std::cout<<"  "<<name<<'\n';
    for(auto const& name:Entries(previous.outputPath))
        assert(Extract(first.outputPath,name.c_str())==Extract(previous.outputPath,name.c_str()));
    std::vector<ItemAllocation> leases;assert(ContentAllocationRegistry().Read("Eitrigg",leases,error)&&leases.size()==4);
    auto lease=[&](char const* symbol,char const* kind)->ItemAllocation const&{
        auto found=std::find_if(leases.begin(),leases.end(),[&](auto const& a){return a.packageKey=="mod-hunts"&&a.symbol==symbol&&a.resourceKind==kind;});
        assert(found!=leases.end());return *found;
    };
    assert(lease("seal","item.id").value==56807);
    assert(lease("seal-currency","currency.known-bit").value==4);
    assert(lease("hunts","currency-category.id").value==5);
    auto allocated=lease("seal-cost-5","item-extended-cost.id").value;assert(allocated>=1&&allocated<=65535);
    auto costBytes=Extract(first.outputPath,"DBFilesClient/ItemExtendedCost.dbc");
    auto parsed=DbcReader::Parse(costBytes,*FindDbcDescriptor(12340,"ItemExtendedCost"));
    assert(parsed.valid&&parsed.document.recordCount==extended.document.recordCount+1);
    std::vector<ResolvedServerItem> rows;std::vector<ResolvedExtendedCost> costs;
    auto server=Read(first.outputPath.string()+".server.json");auto parity=Read(first.outputPath.string()+".parity.json");
    auto object=nlohmann::json::parse(parity);
    assert(ContentServerBundle::ParseServer(server,"Eitrigg",rows,error,&costs));
    assert(costs.size()==1&&costs[0].packageKey=="mod-hunts"&&costs[0].symbol=="seal-cost-5"&&costs[0].id==allocated);
    auto words=ItemExtendedCostDbc::Words(costs[0]);
    for(unsigned i=0;i<16;++i)
        assert(words[i]==(i==0?allocated:i==4?lease("seal","item.id").value:i==9?5:0));
    bool found=false;
    for(std::size_t i=0;i<parsed.document.recordCount;++i)
        if(parsed.document.words[i*16]==allocated){assert(std::equal(words.begin(),words.end(),parsed.document.words.begin()+i*16));found=true;}
    assert(found);
    for(std::size_t i=0;i<extended.document.recordCount;++i)
    {
        auto start=extended.document.words.begin()+i*16;bool present=false;
        for(std::size_t j=0;j<parsed.document.recordCount;++j)
            if(parsed.document.words[j*16]==*start){assert(std::equal(start,start+16,parsed.document.words.begin()+j*16));present=true;break;}
        assert(present);
    }
    assert(parsed.document.strings==extended.document.strings);
    assert(ContentServerBundle::VerifyParity(parity,"Eitrigg",first.buildNumber,item.hash,
        object["clientMpqSha256"],object["serverBundleSha256"],rows,leases,error,costs));
    auto registered=WorldDatabase.Query("SELECT sha256,descriptor_version FROM content_manager_baseline WHERE table_name='ItemExtendedCost' AND client_build=12340");
    assert(registered&&registered->Fetch()[0].Get<std::string>()==extended.hash&&registered->Fetch()[1].Get<unsigned>()==1);
    auto second=build();assert(Extract(second.outputPath,"DBFilesClient/ItemExtendedCost.dbc")==costBytes);
    for(auto const& name:names)assert(Extract(second.outputPath,name.c_str())==Extract(first.outputPath,name.c_str()));
    std::vector<ItemAllocation> after;assert(ContentAllocationRegistry().Read("Eitrigg",after,error)&&after.size()==4);
    for(auto const& a:leases)for(auto const& b:after)
        if(a.packageKey==b.packageKey&&a.symbol==b.symbol&&a.resourceKind==b.resourceKind)
            assert(a.value==b.value&&a.baselineSha256==b.baselineSha256&&b.lastBuild==second.buildNumber);
    assert(Scalar("SELECT COUNT(*) FROM content_manager_build WHERE state<>'STAGED'")==0);
    assert(Scalar("SELECT COUNT(*) FROM content_manager_server_build WHERE server_state<>'STAGED'")==0);
    for(auto table:{"item_template","currencytypes_dbc","itemextendedcost_dbc","content_manager_item_owner","content_manager_currency_owner","content_manager_extended_cost_owner"})
        assert(Scalar("SELECT COUNT(*) FROM "+std::string(table))==0);
    std::cout<<"PASS mod-hunts 4.1.0 -> 4.2.0: production parser; install upgrade without intervening build; four retained allocations; local seal x5 resolves to retained item; automatic cost lease; 973-row local composition; exact stock preservation; deterministic rebuild; three existing DBCs and AQ unchanged; automatic baseline registration; STAGED only; vendor-write prohibition; no Apply/Activate\n";
    std::cout<<"Local fixture automatically allocated cost ID "<<allocated<<" (not an Eitrigg expectation or authored ID).\n";
}
