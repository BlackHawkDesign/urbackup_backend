/*************************************************************************
*    UrBackup - Client/Server backup system
*    Copyright (C) 2011-2014 Martin Raiber
*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/
#ifndef NO_SQLITE


#include "vld.h"
#include "Query.h"
#ifndef BDBPLUGIN
#include "Server.h"
#else
#include "Interface/Server.h"
#ifdef LINUX
#include "bdbplugin/config.h"
#include DB_HEADER
#else
#include <db.h>
#endif
#endif
#include "sqlite/sqlite3.h"
#include "Database.h"
#include "DatabaseCursor.h"
#include <memory.h>
#include <algorithm>
#include "stringtools.h"


#ifdef LOG_QUERIES
IMutex* CQuery::active_mutex;
std::vector<std::string> CQuery::active_queries;
#endif

CQuery::CQuery(const std::string &pStmt_str, sqlite3_stmt *prepared_statement, CDatabase *pDB)
	: stmt_str(pStmt_str), cursor(NULL)
{
	ps=prepared_statement;
	curr_idx=1;
	db=pDB;
}

CQuery::~CQuery()
{
	int err=sqlite3_finalize(ps);
	if( err!=SQLITE_OK && err!=SQLITE_BUSY && err!=SQLITE_IOERR_BLOCKED )
		Server->Log("SQL: "+(std::string)sqlite3_errmsg(db->getDatabase())+ " Stmt: ["+stmt_str+"]", LL_ERROR);

	if( err==SQLITE_IOERR )
	{
		Server->setFailBit(IServer::FAIL_DATABASE_IOERR);
	}

	delete cursor;
}

void CQuery::init_mutex(void)
{
#ifdef LOG_QUERIES
	active_mutex=Server->createMutex();
#endif
}

void CQuery::Bind(const std::string &str)
{
	int err=sqlite3_bind_text(ps, curr_idx, str.c_str(), (int)str.size(), SQLITE_TRANSIENT);
	if( err!=SQLITE_OK )
		Server->Log("Error binding text to Query  Stmt: ["+stmt_str+"]", LL_ERROR);
	++curr_idx;
}

void CQuery::Bind(const std::wstring &str)
{
	int err;
	if( sizeof(wchar_t)==2 )
	{
		err=sqlite3_bind_text16(ps, curr_idx, str.c_str(), (int)str.size()*2, SQLITE_TRANSIENT);
	}
	else
	{
		unsigned short *tmp=new unsigned short[str.size()];
		for(size_t i=0,l=str.size();i<l;++i)
		{
			tmp[i]=str[i];
		}
		err=sqlite3_bind_text16(ps, curr_idx, tmp, (int)str.size()*2, SQLITE_TRANSIENT);
		delete []tmp;
	}
	if( err!=SQLITE_OK )
		Server->Log("Error binding text to Query  Stmt: ["+stmt_str+"]", LL_ERROR);
	++curr_idx;
}

void CQuery::Bind(const char* buffer, _u32 bsize)
{
	int err=sqlite3_bind_blob(ps, curr_idx, buffer, bsize, SQLITE_TRANSIENT);
	if( err!=SQLITE_OK )
		Server->Log("Error binding blob to Query  Stmt: ["+stmt_str+"]", LL_ERROR);
	++curr_idx;
}

void CQuery::Bind(int p)
{
	int err=sqlite3_bind_int(ps, curr_idx, p);
	if( err!=SQLITE_OK )
		Server->Log("Error binding int to Query  Stmt: ["+stmt_str+"]", LL_ERROR);
	++curr_idx;
}

void CQuery::Bind(unsigned int p)
{
	Bind((int64)p);
}

void CQuery::Bind(double p)
{
	int err=sqlite3_bind_double(ps, curr_idx, p);
	if( err!=SQLITE_OK )
		Server->Log("Error binding double to Query  Stmt: ["+stmt_str+"]", LL_ERROR);
	++curr_idx;
}

void CQuery::Bind(int64 p)
{
	int err=sqlite3_bind_int64(ps, curr_idx, p);
	if( err!=SQLITE_OK )
		Server->Log("Error binding int64 to Query  Stmt: ["+stmt_str+"]", LL_ERROR);
	++curr_idx;
}

#if defined(_WIN64) || defined(_LP64)
void CQuery::Bind(size_t p)
{
	Bind((int64)p);
}
#endif

void CQuery::Reset(void)
{
	sqlite3_reset(ps);
	//sqlite3_clear_bindings(ps);
	curr_idx=1;
}

bool CQuery::Write(int timeoutms)
{
#ifdef LOG_WRITE_QUERIES
	ScopedAddActiveQuery active_query(this);
#endif
	return Execute(timeoutms);
}

bool CQuery::Execute(int timeoutms)
{
	//Server->Log("Write: "+stmt_str);

	bool transaction_lock=false;
	int tries=60; //10min
	if(timeoutms>=0)
	{
		sqlite3_busy_timeout(db->getDatabase(), timeoutms);
	}
	int err=sqlite3_step(ps);
	while( err==SQLITE_IOERR_BLOCKED || err==SQLITE_BUSY || err==SQLITE_ROW || err==SQLITE_LOCKED )
	{
		if(err==SQLITE_IOERR_BLOCKED)
		{
			Server->Log("SQlite is blocked!", LL_ERROR);
		}
		if(err==SQLITE_BUSY || err==SQLITE_IOERR_BLOCKED)
		{
			if(timeoutms>=0)
			{
				break;
			}
			else if(!db->isInTransaction() && transaction_lock==false)
			{
				sqlite3_reset(ps);
				if(db->LockForTransaction())
				{
					Server->Log("LockForTransaction in CQuery::Execute Stmt: ["+stmt_str+"]", LL_DEBUG);
					transaction_lock=true;
				}
			}
			else
			{
				sqlite3_reset(ps);
				--tries;
				if(tries==-1)
				{
				  Server->Log("SQLITE: Long running query  Stmt: ["+stmt_str+"]", LL_ERROR);
				  showActiveQueries(LL_ERROR);
				}
				else if(tries>=0)
				{
				    Server->Log("SQLITE_BUSY in CQuery::Execute  Stmt: ["+stmt_str+"]", LL_INFO);
					showActiveQueries(LL_INFO);
				}
			}
		}
		else if(err==SQLITE_LOCKED)
		{
			if(!db->isInTransaction() && db->LockForTransaction())
			{
				transaction_lock=true;
			}				
			if(!db->WaitForUnlock())
			{
				Server->Log("DEADLOCK in CQuery::Execute  Stmt: ["+stmt_str+"]", LL_ERROR);
				showActiveQueries(LL_ERROR);
				Server->wait(1000);
				if(timeoutms>=0)
				{
					timeoutms-=1000;

					if(timeoutms<=0)
						break;
				}
			}
		}
		err=sqlite3_step(ps);
	}

	if(transaction_lock)
	{
		db->UnlockForTransaction();
	}

	if(timeoutms>=0)
	{
		sqlite3_busy_timeout(db->getDatabase(), c_sqlite_busy_timeout_default);
	}

	//Server->Log("Write done: "+stmt_str);

	if( err==SQLITE_IOERR )
	{
		Server->setFailBit(IServer::FAIL_DATABASE_IOERR);
	}
	if(err==SQLITE_CORRUPT)
	{
		Server->setFailBit(IServer::FAIL_DATABASE_CORRUPTED);			
	}

	if( err!=SQLITE_DONE )
	{
		if(timeoutms<0)
		{
			Server->Log("Error in CQuery::Execute - "+(std::string)sqlite3_errmsg(db->getDatabase()) +"  Stmt: ["+stmt_str+"]", LL_ERROR);
		}
		return false;
	}

	return true;
}

void CQuery::setupStepping(int *timeoutms)
{
	if(timeoutms!=NULL && *timeoutms>=0)
	{
		sqlite3_busy_timeout(db->getDatabase(), *timeoutms);
	}
}

void CQuery::shutdownStepping(int err, int *timeoutms, bool& transaction_lock)
{
	if(timeoutms!=NULL && *timeoutms>=0)
	{
		sqlite3_busy_timeout(db->getDatabase(), c_sqlite_busy_timeout_default);
	}

	if(timeoutms!=NULL)
	{
		if(err!=SQLITE_DONE)
		{
			*timeoutms=1;
		}
		else
		{
			*timeoutms=0;
		}
	}

	if(transaction_lock)
	{
		db->UnlockForTransaction();
	}

	if(err==SQLITE_IOERR)
	{
		Server->setFailBit(IServer::FAIL_DATABASE_IOERR);
	}
	if(err==SQLITE_CORRUPT)
	{
		Server->setFailBit(IServer::FAIL_DATABASE_CORRUPTED);			
	}
}

db_nresults CQuery::ReadN(int *timeoutms)
{
	int err;
	db_nresults rows;

	bool transaction_lock=false;
	int tries=60; //10min

#ifdef LOG_READ_QUERIES
	ScopedAddActiveQuery active_query(this);
#endif

	setupStepping(timeoutms);
	
	db_nsingle_result res;
	do
	{
		bool reset=false;
		err=stepN(res, timeoutms, tries, transaction_lock, reset);
		if(reset)
		{
			rows.clear();
		}
		if(err==SQLITE_ROW)
		{
			rows.push_back(res);
			res.clear();
		}
	}
	while(resultOkay(err));

	shutdownStepping(err, timeoutms, transaction_lock);

	return rows;
}

db_results CQuery::Read(int *timeoutms)
{
	int err;
	db_results rows;

	bool transaction_lock=false;
	int tries=60; //10min

#ifdef LOG_READ_QUERIES
	ScopedAddActiveQuery active_query(this);
#endif

	setupStepping(timeoutms);

	db_single_result res;
	do
	{
		bool reset=false;
		err=step(res, timeoutms, tries, transaction_lock, reset);
		if(reset)
		{
			rows.clear();
		}
		if(err==SQLITE_ROW)
		{
			rows.push_back(res);
			res.clear();
		}
	}
	while(resultOkay(err));

	shutdownStepping(err, timeoutms, transaction_lock);

	return rows;
}

bool CQuery::resultOkay(int rc)
{
	return  rc==SQLITE_BUSY ||
			rc==SQLITE_ROW ||
			rc==SQLITE_IOERR_BLOCKED;
}

int CQuery::step(db_single_result& res, int *timeoutms, int& tries, bool& transaction_lock, bool& reset)
{
	int err=sqlite3_step(ps);
	if( resultOkay(err) )
	{
		if( err==SQLITE_BUSY || err==SQLITE_IOERR_BLOCKED )
		{
			if(timeoutms!=NULL && *timeoutms>=0)
			{
				return SQLITE_ABORT;
			}
			else if(!db->isInTransaction() && transaction_lock==false)
			{
				sqlite3_reset(ps);
				reset=true;
				if(db->LockForTransaction())
				{
					Server->Log("LockForTransaction in CQuery::Read Stmt: ["+stmt_str+"]", LL_DEBUG);
					transaction_lock=true;
				}
			}
			else
			{
				sqlite3_reset(ps);
				reset=true;
				--tries;
				if(tries==-1)
				{
				  Server->Log("SQLITE: Long runnning query  Stmt: ["+stmt_str+"]", LL_ERROR);
				  showActiveQueries(LL_ERROR);
				}
				else
				{
					Server->Log("SQLITE_BUSY in CQuery::Read  Stmt: ["+stmt_str+"]", LL_INFO);
					showActiveQueries(LL_INFO);
				}
			}
		}
		else if( err==SQLITE_ROW )
		{
			int column=0;
			const unsigned short *c_name;
			while( (c_name=(const unsigned short*)sqlite3_column_name16(ps, column) )!=NULL )
			{
				std::wstring column_name;
				if( sizeof(wchar_t)!=2 )
				{
					size_t len=0;
					while(c_name[len]!=0)
						++len;

					column_name.resize(len);
					for(size_t i=0;i<len;++i)
					{
						column_name[i]=c_name[i];
					}
				}
				else
				{
					column_name=(wchar_t*)c_name;
				}
				std::wstring data;
				int blob_size=sqlite3_column_bytes16(ps, column);
				const void *blob=sqlite3_column_blob(ps, column);
				//int blob_size2=sqlite3_column_bytes(ps, column);
				if(blob_size>0)
				{
					if( sizeof(wchar_t)==2 )
					{
						data.resize(blob_size/2+blob_size%2);
						memcpy(&data[0], blob, blob_size);
					}
					else
					{
						if( SQLITE_BLOB==sqlite3_column_type(ps, column) )
						{
							size_t size=(size_t)(blob_size/sizeof(wchar_t))+((blob_size%sizeof(wchar_t))>0?1:0);
							
							data.resize(size);
							char* ptr=(char*)data.c_str();
							memcpy(ptr, blob, blob_size);

							if( blob_size%sizeof(wchar_t)>0 )
							{
								memset(ptr+blob_size, 0, sizeof(wchar_t)-blob_size%sizeof(wchar_t) );
							}
						}
						else
						{
							data.resize(blob_size/sizeof(unsigned short));
							unsigned short *ip=(unsigned short*)blob;
							for(int i=0,l=blob_size/sizeof(unsigned short);i<l;++i)
							{
						
								data[i]=*ip;
								++ip;
							}
						}
					}
				}
				res.insert( std::pair<std::wstring, std::wstring>(column_name, data) );
				++column;
			}
		}
		else
		{
			Server->wait(1000);
			if(timeoutms!=NULL && *timeoutms>=0)
			{
				*timeoutms-=1000;

				if(*timeoutms<=0)
				{
					return SQLITE_ABORT;
				}
			}
		}
	}
	return err;
}

int CQuery::stepN(db_nsingle_result& res, int *timeoutms, int& tries, bool& transaction_lock, bool& reset)
{
	int err=sqlite3_step(ps);
	if(resultOkay(err))
	{
		if( err==SQLITE_BUSY || err==SQLITE_IOERR_BLOCKED )
		{
			if(timeoutms!=NULL && *timeoutms>=0)
			{
				return SQLITE_ABORT;
			}
			else if(!db->isInTransaction() && transaction_lock==false)
			{
				sqlite3_reset(ps);
				reset=true;
				if(db->LockForTransaction())
				{
					Server->Log("LockForTransaction in CQuery::ReadN Stmt: ["+stmt_str+"]", LL_DEBUG);
					transaction_lock=true;
				}
			}
			else
			{
				sqlite3_reset(ps);
				reset=true;
				--tries;
				if(tries==-1)
				{
				  Server->Log("SQLITE: Long running query  Stmt: ["+stmt_str+"]", LL_ERROR);
				  showActiveQueries(LL_ERROR);
				}
				else
				{
					Server->Log("SQLITE_BUSY in CQuery::ReadN  Stmt: ["+stmt_str+"]", LL_INFO);
					showActiveQueries(LL_INFO);
				}
			}
		}
		else if( err==SQLITE_ROW )
		{
			int column=0;
			const char *column_name;
			while( (column_name=sqlite3_column_name(ps, column) )!=NULL )
			{
				std::string data;
				const void *blob=sqlite3_column_blob(ps, column);
				int blob_size=sqlite3_column_bytes(ps, column);
				if(blob_size>0 && blob!=NULL)
				{
					data.resize(blob_size);
					memcpy(&data[0], blob, blob_size);
				}
				res.insert( std::pair<std::string, std::string>(column_name, data) );
				++column;
			}
		}
		else
		{
			Server->wait(1000);
			if(timeoutms!=NULL && *timeoutms>=0)
			{
				*timeoutms-=1000;

				if(*timeoutms<=0)
				{
					return SQLITE_ABORT;
				}
			}
		}
	}
	return err;
}

IDatabaseCursor* CQuery::Cursor(int *timeoutms)
{
	if(cursor==NULL)
	{
		cursor=new DatabaseCursor(this, timeoutms);
	}

	return cursor;
}

std::string CQuery::getStatement(void)
{
	return stmt_str;
}

std::string CQuery::getErrMsg(void)
{
	return std::string(sqlite3_errmsg(db->getDatabase()));
}

void CQuery::addActiveQuery(const std::string& query_str)
{
#ifdef LOG_QUERIES
	IScopedLock lock(active_mutex);
	active_queries.push_back(query_str);
#endif
}

void CQuery::removeActiveQuery(const std::string& query_str)
{
#ifdef LOG_QUERIES
	IScopedLock lock(active_mutex);
	std::vector<std::string>::iterator iter =
		std::find(active_queries.begin(), active_queries.end(),
				  query_str);
	if(iter!=active_queries.end())
		active_queries.erase(iter);
#endif
}

void CQuery::showActiveQueries(int loglevel)
{
#ifdef LOG_QUERIES
	IScopedLock lock(active_mutex);
	for(size_t i=0;i<active_queries.size();++i)
	{
		Server->Log("Active query("+nconvert(i)+"): "+active_queries[i], loglevel);
	}
#endif
}

#endif