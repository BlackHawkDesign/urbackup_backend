#ifndef INTERFACE_DATABASE_H
#define INTERFACE_DATABASE_H

#include <vector>
#include <string>
#include <map>
#include "Query.h"
#include "Object.h"

class IDatabase : public IObject
{
public:
	virtual db_nresults ReadN(std::string pQuery)=0; 
	virtual db_results Read(std::string pQuery)=0; 
	virtual bool Write(std::string pQuery)=0;

	virtual bool BeginWriteTransaction(void)=0;
	virtual bool BeginReadTransaction(void)=0;
	virtual bool EndTransaction(void)=0;

	virtual IQuery* Prepare(std::string pQuery, bool autodestroy=true)=0;
	virtual void destroyQuery(IQuery *q)=0;
	virtual void destroyAllQueries(void)=0;

	virtual _i64 getLastInsertID(void)=0;

	virtual bool Import(const std::string &pFile)=0;
	virtual bool Dump(const std::string &pFile)=0;
	
	virtual std::string getEngineName(void)=0;

	virtual void DetachDBs(void)=0;
	virtual void AttachDBs(void)=0;

	virtual bool Backup(const std::string &pFile)=0;

	virtual void freeMemory()=0;

	virtual int getLastChanges()=0;
};

class DBScopedFreeMemory
{
public:
	DBScopedFreeMemory(IDatabase* db)
		: db(db) {}
	~DBScopedFreeMemory() {
		db->freeMemory();
	}
private:
	IDatabase* db;
};

class DBScopedDetach
{
public:
	DBScopedDetach(IDatabase* db)
		: db(db) {
			if(db!=NULL) db->DetachDBs();
	}
	~DBScopedDetach() {
		if(db!=NULL) db->AttachDBs();
	}
	void attach() {
		if(db!=NULL) db->AttachDBs();
		db=NULL;
	}
private:
	IDatabase* db;
};

class DBScopedAttach
{
public:
	DBScopedAttach(IDatabase* db)
		: db(db) {
			if(db!=NULL) db->AttachDBs();
	}
	~DBScopedAttach() {
		if(db!=NULL) db->DetachDBs();
	}
	void detach() {
		if(db!=NULL) db->DetachDBs();
		db=NULL;
	}
private:
	IDatabase* db;
};

class DBScopedWriteTransaction
{
public:
	DBScopedWriteTransaction(IDatabase* db)
		: db(db) {
			if(db!=NULL) db->BeginWriteTransaction();
	}
	~DBScopedWriteTransaction() {
		if(db!=NULL) db->EndTransaction();
	}

	void restart() {
		if(db!=NULL) {
			db->EndTransaction();
			db->BeginWriteTransaction();
		}
	}

	void end() {
		if(db!=NULL) db->EndTransaction();
		db=NULL;
	}
private:
	IDatabase* db;
};

#endif
