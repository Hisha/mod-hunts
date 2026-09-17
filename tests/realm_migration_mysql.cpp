#include "HuntCurrencyMigration.h"
#include <cassert>
#include <iostream>
#include <stdexcept>
static void SQL(std::string const& s){if(!CharacterDatabase.Execute(s))throw std::runtime_error(CharacterDatabase.lastError);}
static unsigned long long Value(std::string const& s){auto q=CharacterDatabase.Query(s);assert(q);return q->Fetch()[0].Get<unsigned long long>();}
int main(int argc,char** argv)
{
    assert(argc==2);CharacterDatabase.Connect(argv[1]);
    auto reset=[] {
        SQL("DELETE FROM hunt_currency_delivery");SQL("DELETE FROM hunt_currency_realm");SQL("DELETE FROM hunt_stats");SQL("DELETE FROM characters");SQL("DELETE FROM delivery_items");
    };
    unsigned notifications=0,attempts=0;
    // Delivery seam only: production migration owns all snapshot/receipt/guard SQL.
    // Production Prepare uses core Item::SaveToDB plus mail/mail_items in this tx.
    auto prepare=[&](auto const& tx,unsigned guid,unsigned amount,std::string&) {
        ++attempts;
        tx->Append("INSERT INTO delivery_items(guid,itemEntry,amount) VALUES("+std::to_string(guid)+",60001,"+std::to_string(amount)+")");return true;
    };
    auto run=[&](auto const& delivery){std::string e;bool ok=HuntCurrencyMigration::Run(60001,200,delivery,[&]{++notifications;},e);if(!ok)std::cout<<"Expected stopped migration: "<<e<<'\n';return ok;};
    reset();SQL("INSERT INTO characters VALUES(1),(2),(3)");SQL("INSERT INTO hunt_stats VALUES(1,0),(2,20),(3,25001)");
    assert(run(prepare));assert(Value("SELECT snapshot_count FROM hunt_currency_realm")==3);
    assert(Value("SELECT snapshot_total FROM hunt_currency_realm")==25021);
    assert(Value("SELECT SUM(amount) FROM delivery_items")==25021);assert(notifications==3);
    assert(Value("SELECT SUM(huntmaster_seals) FROM hunt_stats")==25021);
    assert(Value("SELECT COUNT(*) FROM hunt_currency_delivery WHERE legacy_amount=delivered_amount")==3);
    assert(run(prepare));assert(Value("SELECT SUM(amount) FROM delivery_items")==25021); // restart does not reissue
    reset();assert(run(prepare));assert(Value("SELECT snapshot_total FROM hunt_currency_realm")==0); // fresh installation
    reset();SQL("INSERT INTO characters VALUES(9)");SQL("INSERT INTO hunt_stats VALUES(9,50000)");
    unsigned calls=0;
    auto failSecond=[&](auto const& tx,unsigned guid,unsigned amount,std::string& error) {
        if(++calls==2){error="Injected item creation failure";return false;}return prepare(tx,guid,amount,error);
    };
    assert(!run(failSecond));assert(Value("SELECT SUM(amount) FROM delivery_items")==24000);
    assert(Value("SELECT delivered_amount FROM hunt_currency_delivery")==24000);
    assert(Value("SELECT huntmaster_seals FROM hunt_stats")==50000);
    assert(run(prepare));assert(Value("SELECT SUM(amount) FROM delivery_items")==50000); // resumes remainder only
    reset();SQL("INSERT INTO characters VALUES(10)");SQL("INSERT INTO hunt_stats VALUES(10,33)");
    auto failTx=[&](auto const& tx,unsigned guid,unsigned amount,std::string& error) {
        prepare(tx,guid,amount,error);tx->Append("INSERT INTO hunt_currency_realm(id,migration_version,state,seal_item) VALUES(1,1,'BAD',1)");return true;
    };
    auto publishedBefore=notifications;
    assert(!run(failTx));assert(Value("SELECT COUNT(*) FROM delivery_items")==0);
    assert(Value("SELECT delivered_amount FROM hunt_currency_delivery")==0);assert(notifications==publishedBefore);
    assert(run(prepare));assert(Value("SELECT SUM(amount) FROM delivery_items")==33);
    // Offline/nonexistent character is never silently skipped or marked delivered.
    reset();SQL("INSERT INTO hunt_stats VALUES(99,7)");assert(!run(prepare));
    assert(Value("SELECT delivered_amount FROM hunt_currency_delivery")==0);
    SQL("INSERT INTO characters VALUES(99)");assert(run(prepare));assert(Value("SELECT SUM(amount) FROM delivery_items")==7);
    std::cout<<"Production realm migration SQL: zero/fresh, offline balances, snapshot preservation, chunking, restart, partial preparation and transaction rollback PASS\n";
}
