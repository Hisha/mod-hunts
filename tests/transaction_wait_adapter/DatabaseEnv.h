#pragma once
#include <future>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <type_traits>
struct Field { bool null=false;std::string value;bool IsNull() const{return null;}
 template<class T>T Get()const{if constexpr(std::is_same_v<T,std::string>)return value;else return T(std::stoull(value));}
};
struct Result { std::vector<Field> fields;Field* Fetch(){return fields.data();} };
using QueryResult=std::shared_ptr<Result>;
struct Transaction {std::vector<std::string> sql;void Append(std::string const& s){sql.push_back(s);} };
using CharacterDatabaseTransaction=std::shared_ptr<Transaction>;
struct TransactionCallback {std::future<bool> m_future;};
struct Database {
 std::function<std::future<bool>(CharacterDatabaseTransaction)> submit;
 std::function<QueryResult(std::string const&)> query;
 CharacterDatabaseTransaction BeginTransaction(){return std::make_shared<Transaction>();}
 TransactionCallback AsyncCommitTransaction(CharacterDatabaseTransaction tx){return {submit(tx)};}
 QueryResult Query(std::string const& s){return query(s);}
};
inline Database CharacterDatabase;
