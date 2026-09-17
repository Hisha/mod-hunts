#include "HuntCharacterTransaction.h"
#include "HuntCurrencyMigration.h"
#include <cassert>
#include <deque>
#include <iostream>
#include <stdexcept>
std::future<bool> ready(bool b){std::promise<bool> p;auto f=p.get_future();p.set_value(b);return f;}
QueryResult row(std::initializer_list<Field> f){return std::make_shared<Result>(Result{f});}
int main()
{
    auto tx=CharacterDatabase.BeginTransaction();tx->Append("preserved transaction");
    bool workerFinished=false;
    CharacterDatabase.submit=[&](auto submitted){assert(submitted==tx);return std::async(std::launch::async,[&]{workerFinished=true;return true;});};
    assert(hunts::CommitCharacterTransactionAndWait(tx) && workerFinished);
    CharacterDatabase.submit=[](auto){return ready(false);};assert(!hunts::CommitCharacterTransactionAndWait(tx));
    CharacterDatabase.submit=[](auto){return std::future<bool>{};};assert(!hunts::CommitCharacterTransactionAndWait(tx));
    CharacterDatabase.submit=[](auto){std::promise<bool> p;auto f=p.get_future();p.set_exception(std::make_exception_ptr(std::runtime_error("worker failure")));return f;};
    assert(!hunts::CommitCharacterTransactionAndWait(tx));
    CharacterDatabase.submit=[](auto)->std::future<bool>{throw std::runtime_error("submission failure");};
    assert(!hunts::CommitCharacterTransactionAndWait(tx));
    std::cout<<"PASS helper: worker completion, exact transaction identity, true/false, invalid future, future/submission exceptions\n";

    for (int scenario=0;scenario<5;++scenario)
    {
        std::deque<QueryResult> replies;
        if(scenario==0)replies.push_back(row({{true,""}})); // snapshot failure
        else replies.push_back(row({{false,"MIGRATING"}}));
        if(scenario==1)replies.push_back(row({{true,""}})); // final transition failure
        if(scenario>=2)replies.push_back(row({{false,"7"},{false,"20"},{false,"0"},{false,"7"}}));
        if(scenario==3)replies.push_back(row({{false,"19"}})); // successful commit, bad receipt
        if(scenario==4)
        {
            replies.push_back(row({{false,"20"}}));replies.push_back(row({{true,""}}));replies.push_back(row({{false,"NATIVE"}}));
        }
        bool futureCompleted=false;unsigned commits=0,published=0,queries=0;
        CharacterDatabase.query=[&](auto const& sql){
            assert(!replies.empty());
            if(sql.find("SELECT delivered_amount")!=std::string::npos || sql=="SELECT state FROM hunt_currency_realm WHERE id=1")assert(futureCompleted);
            ++queries;auto r=replies.front();replies.pop_front();return r;
        };
        CharacterDatabase.submit=[&](auto batch){
            ++commits;
            if(scenario>=2 && commits==1)
            {
                bool item=false,mail=false,attachment=false,receipt=false;
                for(auto const& sql:batch->sql){item|=sql=="item_instance";mail|=sql=="mail";attachment|=sql=="mail_items";receipt|=sql.find("UPDATE hunt_currency_delivery SET delivered_amount=20")!=std::string::npos;}
                assert(item&&mail&&attachment&&receipt); // same submitted transaction
            }
            return std::async(std::launch::async,[&]{futureCompleted=true;return scenario>=3;});
        };
        std::string error;
        auto prepare=[](auto const& batch,unsigned,unsigned,std::string&){batch->Append("item_instance");batch->Append("mail");batch->Append("mail_items");return true;};
        bool ok=HuntCurrencyMigration::Run(60001,200,prepare,[&]{assert(futureCompleted);++published;},error);
        assert(replies.empty());assert(ok==(scenario==4));assert(published==(scenario==4?1u:0u));
        assert(commits==(scenario==4?2u:1u));assert(ok || !error.empty());
    }
    std::cout<<"PASS production migration: failed snapshot/delivery/final commit skip verification and publication; bad receipt skips publication; successful delivery verifies then publishes, final transition verifies\n";
}
