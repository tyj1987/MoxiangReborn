#pragma once
#include "mxh/game/item_types.hpp"
#include <sqlite3.h>
#include <string>

inline bool probe_sqlite_item(const std::string& path,std::uint32_t player,const mxh::game::ItemBase& item) {
    sqlite3* db=nullptr;
    if(sqlite3_open_v2(path.c_str(),&db,SQLITE_OPEN_READONLY,nullptr)!=SQLITE_OK){if(db)sqlite3_close(db);return false;}
    sqlite3_busy_timeout(db,500);
    sqlite3_stmt* statement=nullptr;
    const char* sql="SELECT item_idx,item_param,container,slot FROM modern_player_item WHERE player_id=? AND db_idx=?";
    bool result=false;
    if(sqlite3_prepare_v2(db,sql,-1,&statement,nullptr)==SQLITE_OK){
        sqlite3_bind_int64(statement,1,player);sqlite3_bind_int64(statement,2,item.dwDBIdx);
        if(sqlite3_step(statement)==SQLITE_ROW){
            result=sqlite3_column_int64(statement,0)==item.wIconIdx && sqlite3_column_int64(statement,1)==item.ItemParam &&
                sqlite3_column_int(statement,2)==0 && sqlite3_column_int(statement,3)==item.Position;
            result=result && sqlite3_step(statement)==SQLITE_DONE;
        }
    }
    sqlite3_finalize(statement);sqlite3_close(db);return result;
}
