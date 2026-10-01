#include "map17_evidence.hpp"
#include <filesystem>
#include <chrono>
#include <cstdio>

int main() {
    const auto path=std::filesystem::temp_directory_path()/(
        "map17-evidence-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".db");
    sqlite3* db=nullptr;
    if(sqlite3_open(path.string().c_str(),&db)!=SQLITE_OK)return 1;
    const char* schema="CREATE TABLE modern_player_item(player_id INTEGER,db_idx INTEGER,item_idx INTEGER,item_param INTEGER,container INTEGER,slot INTEGER);"
        "INSERT INTO modern_player_item VALUES(7,99,8000,1,0,3);";
    if(sqlite3_exec(db,schema,nullptr,nullptr,nullptr)!=SQLITE_OK)return 2;
    sqlite3_close(db);
    mxh::game::ItemBase item{};item.dwDBIdx=99;item.wIconIdx=8000;item.ItemParam=1;item.Position=3;
    bool passed=probe_sqlite_item(path.string(),7,item);
    passed=passed && !probe_sqlite_item(path.string(),8,item);
    item.dwDBIdx=100;passed=passed && !probe_sqlite_item(path.string(),7,item);item.dwDBIdx=99;
    item.ItemParam=2;passed=passed && !probe_sqlite_item(path.string(),7,item);item.ItemParam=1;
    item.Position=4;passed=passed && !probe_sqlite_item(path.string(),7,item);item.Position=3;
    item.wIconIdx=8007;passed=passed && !probe_sqlite_item(path.string(),7,item);item.wIconIdx=8000;
    sqlite3_open(path.string().c_str(),&db);
    sqlite3_exec(db,"INSERT INTO modern_player_item VALUES(7,99,8000,1,0,3);",nullptr,nullptr,nullptr);
    sqlite3_close(db);
    passed=passed && !probe_sqlite_item(path.string(),7,item);
    std::filesystem::remove(path);
    passed=passed && !probe_sqlite_item(path.string(),7,item) && !std::filesystem::exists(path);
    std::puts(passed?"PASS: SQLite identity/count/slot/duplicate/missing-file evidence gates":"FAIL");
    return passed?0:3;
}
