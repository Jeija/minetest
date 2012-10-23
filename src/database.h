#ifndef DATABASE_HEADER
#define DATABASE_HEADER

#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "main.h"
#include "filesys.h"

extern "C" {
	#include "sqlite3.h"
}

extern "C" {
	#include "mysql.h"
}

#include "leveldb/db.h"

class Database;
class Database_SQLite3;
class ServerMap;

class Database
{
public:
        virtual void beginSave()=0;
        virtual void endSave()=0;

        virtual void saveBlock(MapBlock *block)=0;
        virtual MapBlock* loadBlock(v3s16 blockpos)=0;
	long long getBlockAsInteger(const v3s16 pos);
	v3s16 getIntegerAsBlock(long long i);
	virtual void listAllLoadableBlocks(core::list<v3s16> &dst)=0;
	virtual int Initialized(void)=0;
	virtual ~Database() {};
};

class Database_SQLite3 : public Database
{
public:
	Database_SQLite3(ServerMap *map, std::string savedir);
        virtual void beginSave();
        virtual void endSave();

        virtual void saveBlock(MapBlock *block);
        virtual MapBlock* loadBlock(v3s16 blockpos);
        virtual void listAllLoadableBlocks(core::list<v3s16> &dst);
        virtual int Initialized(void);
	~Database_SQLite3();
private:
	ServerMap *srvmap;
	std::string m_savedir;
	sqlite3 *m_database;
	sqlite3_stmt *m_database_read;
	sqlite3_stmt *m_database_write;
	sqlite3_stmt *m_database_list;

	// Create the database structure
	void createDatabase();
        // Verify we can read/write to the database
        void verifyDatabase();
        void createDirs(std::string path);
};

class Database_Dummy : public Database
{
public:
	Database_Dummy(ServerMap *map);
	virtual void beginSave();
	virtual void endSave();
        virtual void saveBlock(MapBlock *block);
        virtual MapBlock* loadBlock(v3s16 blockpos);
        virtual void listAllLoadableBlocks(core::list<v3s16> &dst);
        virtual int Initialized(void);
	~Database_Dummy();
private:
	ServerMap *srvmap;
	std::map<unsigned long long, std::string> m_database;
};

class Database_LevelDB : public Database
{
public:
	Database_LevelDB(ServerMap *map, std::string savedir);
	virtual void beginSave();
	virtual void endSave();
        virtual void saveBlock(MapBlock *block);
        virtual MapBlock* loadBlock(v3s16 blockpos);
        virtual void listAllLoadableBlocks(core::list<v3s16> &dst);
        virtual int Initialized(void);
	~Database_LevelDB();
private:
	ServerMap *srvmap;
	leveldb::DB* m_database;
};

class Database_MySQL : public Database
{
public:
	Database_MySQL(ServerMap *map, std::string host, std::string user, std::string pass, std::string dbasename, std::string worldname);
        virtual void beginSave();
        virtual void endSave();

        virtual void saveBlock(MapBlock *block);
        virtual MapBlock* loadBlock(v3s16 blockpos);
        virtual void listAllLoadableBlocks(core::list<v3s16> &dst);
        virtual int Initialized(void);
	~Database_MySQL();
private:
	ServerMap *srvmap;
	std::string m_worldname;
	std::string db_host, db_user, db_pass, db_base;
	MYSQL *m_database;
	MYSQL_STMT *m_database_read;
	MYSQL_STMT *m_database_write;
	MYSQL_STMT *m_database_list;

	// Create the database structure
	void createDatabase();
        // Verify we can read/write to the database
        void verifyDatabase();
        void createDirs(std::string path);
};

#endif
