/*
	MySQL format specification:
	
	world data is stored in table <world_name>_blocks
	if table do not exists it will be created
	
	Structure of world table:
		x int NOT NULL,
		y int NOT NULL,
		z int NOT NULL,
		ver tinyint unsigned NOT NULL,
		underground bit(1) NOT NULL,
		daynight bit(1) NOT NULL,
		lighting bit(1) NOT NULL,
		generated bit(1) NOT NULL,
		bulk mediumblob NOT NULL,
		meta mediumblob NOT NULL,
		static mediumblob NOT NULL,
		timers mediumblob NOT NULL,
		nimap mediumblob NOT NULL,

	Currently only x,y,z and bulk columns are used.

*/


#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "main.h"
#include "filesys.h"
#include "voxel.h"
#include "porting.h"
#include "mapgen.h"
#include "nodemetadata.h"
#include "settings.h"
#include "log.h"
#include "profiler.h"
#include "nodedef.h"
#include "gamedef.h"
#include "util/directiontables.h"
#include "rollback_interface.h"

#include "database.h"

#define READ_QUERY "SELECT bulk FROM `%s_blocks` WHERE x=? AND y=? AND z=? LIMIT 1"
#define WRITE_QUERY "REPLACE INTO `%s_blocks` (x,y,z,bulk) VALUES(?, ?, ?, ?)"
#define LIST_QUERY "SELECT x,y,z FROM `%s_blocks`"

Database_MySQL::Database_MySQL(ServerMap *map, std::string host, std::string user, std::string pass, std::string dbasename, std::string worldname)
{
	m_database = NULL;
	m_database_read = NULL;
	m_database_write = NULL;
	m_database_list = NULL;
	db_host = host;
	db_user = user;
	db_pass = pass;
	db_base = dbasename;
	m_worldname = worldname.substr(worldname.rfind('/')+1);
	srvmap = map;
}

int Database_MySQL::Initialized(void)
{
	return m_database ? 1 : 0 ;
}

void Database_MySQL::beginSave() {
	verifyDatabase();
	if(mysql_query(m_database, "BEGIN;"))
		infostream<<"WARNING: beginSave() failed, saving might be slow.";
}

void Database_MySQL::endSave() {
	verifyDatabase();
	if(mysql_query(m_database, "COMMIT;"))
		infostream<<"WARNING: endSave() failed, map might not have saved.";
}

void Database_MySQL::verifyDatabase() {
	if(m_database)
		return;

	{
		
		/*
			Open the database connection
		*/
	
                m_database = mysql_init(NULL);
		if (!mysql_real_connect(m_database,db_host.data(),db_user.data(),db_pass.data(),db_base.data(),0,NULL,0)) {
			infostream<<"WARNING: MySQL database failed to open: "<<std::endl;
			throw FileNotGoodException("Cannot open database");
		}

		if (mysql_select_db(m_database,"minetest")) {
			infostream<<"WARNING: MySQL database failed to open: "<<std::endl;
			throw FileNotGoodException(mysql_error(m_database));
		}
		
		char q[1024];
		sprintf(q,"select count(*) from `%s_blocks`",m_worldname.c_str());
		int e = mysql_query(m_database,q);
		if (e) {
			createDatabase();
			e = mysql_query(m_database,q);
			if (e) {
				infostream<<"WARNING: MySQL database failed to initialize!!: "<<std::endl;
                        throw FileNotGoodException(mysql_error(m_database));
			}
		} 
		mysql_free_result(mysql_store_result(m_database));
		if (!(m_database_read = mysql_stmt_init(m_database))) {
			infostream<<"WARNING: MySQL database read statment failed to init: "<<std::endl;
			throw FileNotGoodException("Cannot prepare1 read statement");
		}

		sprintf(q,READ_QUERY,m_worldname.c_str());
		if (mysql_stmt_prepare(m_database_read, q, strlen(q))) {
			printf("q: %s, %s\n",q,mysql_error(m_database));
			infostream<<"WARNING: MySQL database read statment failed to prepare: "<<std::endl;
			throw FileNotGoodException("Cannot prepare2 read statement");
		}

		if (!(m_database_write = mysql_stmt_init(m_database))) {
			infostream<<"WARNING: MySQL database write statment failed to init: "<<std::endl;
			throw FileNotGoodException("Cannot prepare1 write statement");
		}
		sprintf(q,WRITE_QUERY,m_worldname.c_str());
		if (mysql_stmt_prepare(m_database_write, q, strlen(q))) {
			printf("q: %s, %s\n",q,mysql_error(m_database));
			infostream<<"WARNING: MySQL database write statment failed to prepare: "<<std::endl;
			throw FileNotGoodException("Cannot prepare2 write statement");
		}

		if (!(m_database_list = mysql_stmt_init(m_database))) {
			infostream<<"WARNING: MySQL database list statment failed to init: "<<std::endl;
			throw FileNotGoodException("Cannot prepare1 list statement");
		}
		sprintf(q,LIST_QUERY,m_worldname.c_str());
		if (mysql_stmt_prepare(m_database_list, q, strlen(q))) {
			printf("q: %s, %s\n",q,mysql_error(m_database));
			infostream<<"WARNING: MySQL database list statment failed to prepare: "<<std::endl;
			throw FileNotGoodException("Cannot prepare2 list statement");
		}

		infostream<<"Server: MySQL database opened"<<std::endl;
	}
}

void Database_MySQL::saveBlock(MapBlock *block)
{
	DSTACK(__FUNCTION_NAME);
	/*
		Dummy blocks are not written
	*/
	MYSQL_BIND bind[4];
	if(block->isDummy())
	{
		/*v3s16 p = block->getPos();
		infostream<<"Database_MySQL::saveBlock(): WARNING: Not writing dummy block "
				<<"("<<p.X<<","<<p.Y<<","<<p.Z<<")"<<std::endl;*/
		return;
	}

	if(!block->getModified())
		return;
	// Format used for writing
	u8 version = SER_FMT_VER_HIGHEST;
	// Get destination
	v3s16 p3d = block->getPos();

	/*
		[0] u8 serialization version
		[1] data
	*/
	
	verifyDatabase();
	
	std::ostringstream o(std::ios_base::binary);
	
	o.write((char*)&version, 1);
	
	// Write basic data
	block->serialize(o, version, true);
	
	// Write block to database
	
	std::string tmp = o.str();
	const char *bytes = tmp.c_str();
	unsigned long bytes_cnt = o.tellp(); // TODO this mught not be the right length 
	
	memset(bind, 0, sizeof(bind));
	bind[0].buffer_type= MYSQL_TYPE_SHORT;
	bind[0].buffer= (char *)&p3d.X;
	bind[0].is_null= 0;
	bind[0].length= 0;

	bind[1].buffer_type= MYSQL_TYPE_SHORT;
	bind[1].buffer= (char *)&p3d.Y;
	bind[1].is_null= 0;
	bind[1].length= 0;

	bind[2].buffer_type= MYSQL_TYPE_SHORT;
	bind[2].buffer= (char *)&p3d.Z;
	bind[2].is_null= 0;
	bind[2].length= 0;

	bind[3].buffer_type= MYSQL_TYPE_BLOB;
	bind[3].buffer= (char *)bytes;
	bind[3].buffer_length= bytes_cnt;
	bind[3].is_null= 0;
	bind[3].length= &bytes_cnt;

	if (mysql_stmt_bind_param(m_database_write, bind))
		infostream<<"WARNING: Block failed to bind:"<<mysql_stmt_error(m_database_write)<<std::endl;

	if (mysql_stmt_execute(m_database_write))
		infostream<<"WARNING: Block failed to save ("<<p3d.X<<", "<<p3d.Y<<", "<<p3d.Z<<") "
		<<mysql_stmt_error(m_database_write)<<std::endl;
	// We just wrote it to the disk so clear modified flag
	mysql_stmt_reset(m_database_write);
	block->resetModified();
}

MapBlock* Database_MySQL::loadBlock(v3s16 blockpos)
{
	v2s16 p2d(blockpos.X, blockpos.Z);
        verifyDatabase();
        
	MYSQL_BIND bind[3];
	MYSQL_BIND bindr[1];
	unsigned long real_length;
	char buf[65535];

	memset(bind, 0, sizeof(bind));
        bind[0].buffer_type= MYSQL_TYPE_SHORT;
        bind[0].buffer= (char *)&blockpos.X;
        bind[0].is_null= 0;
        bind[0].length= 0;
        bind[1].buffer_type= MYSQL_TYPE_SHORT;
        bind[1].buffer= (char *)&blockpos.Y;
        bind[1].is_null= 0;
        bind[1].length= 0;
        bind[2].buffer_type= MYSQL_TYPE_SHORT;
        bind[2].buffer= (char *)&blockpos.Z;
        bind[2].is_null= 0;
        bind[2].length= 0;

	if (mysql_stmt_bind_param(m_database_read, bind))
                infostream<<"WARNING: Could not bind block position for load: "
                        <<mysql_stmt_error(m_database_read)<<std::endl;

	if (mysql_stmt_execute(m_database_read))
		infostream<<"WARNING: Block failed to load "
		<<mysql_stmt_error(m_database_read)<<std::endl;


	memset(bindr, 0, sizeof(bindr));

	bindr[0].buffer_type= MYSQL_TYPE_BLOB;
	bindr[0].buffer= (char *)buf;
	bindr[0].buffer_length= 65535;
	bindr[0].length= &real_length;

	if (mysql_stmt_bind_result(m_database_read, bindr))
                infostream<<"WARNING: Could not bind result block position for load: "
                        <<mysql_stmt_error(m_database_read)<<std::endl;
	
	if (!mysql_stmt_fetch(m_database_read)) {
                /*
                        Make sure sector is loaded
                */
                MapSector *sector = srvmap->createSector(p2d);
                
                /*
                        Load block
                */
                const char * data = buf;
                size_t len = real_length;
                
                std::string datastr(data, len);
                
//                srvmap->loadBlock(&datastr, blockpos, sector, false);
		try {
                	std::istringstream is(datastr, std::ios_base::binary);
                     
                   	u8 version = SER_FMT_VER_INVALID;
                     	is.read((char*)&version, 1);

                     	if(is.fail())
                             	throw SerializationError("ServerMap::loadBlock(): Failed"
                                	             " to read MapBlock version");

                     	MapBlock *block = NULL;
                     	bool created_new = false;
                     	block = sector->getBlockNoCreateNoEx(blockpos.Y);
                     	if(block == NULL)
                     	{
                             	block = sector->createBlankBlockNoInsert(blockpos.Y);
                             	created_new = true;
                     	}
                     
                     	// Read basic data
                     	block->deSerialize(is, version, true);
                     
                     	// If it's a new block, insert it to the map
                     	if(created_new)
                             	sector->insertBlock(block);
                     
                     	/*
                             	Save blocks loaded in old format in new format
                     	*/

                     	//if(version < SER_FMT_VER_HIGHEST || save_after_load)
                     	// Only save if asked to; no need to update version
                     	//if(save_after_load)
                        //     	saveBlock(block);
                     
                     	// We just loaded it from, so it's up-to-date.
                     	block->resetModified();

             	}
             	catch(SerializationError &e)
             	{
                     	errorstream<<"Invalid block data in database"
                                     <<" ("<<blockpos.X<<","<<blockpos.Y<<","<<blockpos.Z<<")"
                                     <<" (SerializationError): "<<e.what()<<std::endl;
                     
                     // TODO: Block should be marked as invalid in memory so that it is
                     // not touched but the game can run

                     	if(g_settings->getBool("ignore_world_load_errors")){
                             errorstream<<"Ignoring block load error. Duck and cover! "
                                             <<"(ignore_world_load_errors)"<<std::endl;
                     	} else {
                             throw SerializationError("Invalid block data in database");
                             //assert(0);
                     	}
             	}
		mysql_stmt_reset(m_database_read);
                return srvmap->getBlockNoCreateNoEx(blockpos);
        }
	mysql_stmt_reset(m_database_read);
	return(NULL);
}

void Database_MySQL::createDatabase()
{
	int e;
	char q[512]; 
	assert(m_database);
	sprintf (q,"CREATE TABLE IF NOT EXISTS `%s_blocks` ( \
			x int NOT NULL, \
			y int NOT NULL, \
			z int NOT NULL, \
			ver tinyint unsigned NOT NULL, \
			underground bit(1) NOT NULL, \
			daynight bit(1) NOT NULL, \
			lighting bit(1) NOT NULL, \
			generated bit(1) NOT NULL, \
			bulk mediumblob NOT NULL, \
			meta mediumblob NOT NULL, \
			static mediumblob NOT NULL, \
			timers mediumblob NOT NULL, \
			nimap mediumblob NOT NULL, \
  			PRIMARY KEY (x,y,z) \
		);", m_worldname.c_str());
	e = mysql_query(m_database, q);
	if(e) {
		printf("%d: %s / %s\n",e,mysql_error(m_database),q);
		throw FileNotGoodException("Could not create MySQL database structure");

	} else
		infostream<<"ServerMap: MySQL database structure was created";
	
}

void Database_MySQL::listAllLoadableBlocks(core::list<v3s16> &dst)
{
	verifyDatabase();
	printf("LIST CALLED\n");	
	if (mysql_stmt_execute(m_database_list))
		infostream<<"WARNING: Block list failed "
		<<mysql_stmt_error(m_database_list)<<std::endl;
	
	long long block_i;
	MYSQL_BIND bind[1];
	memset(bind, 0, sizeof(bind));

	bind[0].buffer_type= MYSQL_TYPE_LONGLONG;
	bind[0].buffer= (char *)&block_i;

	if (mysql_stmt_bind_result(m_database_list, bind))
                infostream<<"WARNING: Could not bind result block position for load: "
                        <<mysql_stmt_error(m_database_list)<<std::endl;

	while(!mysql_stmt_fetch(m_database_read))
	{
		v3s16 p = getIntegerAsBlock(block_i);
		//dstream<<"block_i="<<block_i<<" p="<<PP(p)<<std::endl;
		dst.push_back(p);
	}
}

Database_MySQL::~Database_MySQL()
{
	if(m_database)
		mysql_close(m_database);
}

