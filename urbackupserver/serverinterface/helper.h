#pragma once

#include "../../Interface/Database.h"
#include "../../Interface/Server.h"
#include "../../Interface/SessionMgr.h"
#include "../../Interface/Template.h"
#include "../../Interface/Mutex.h"

class Helper
{
public:
	Helper(THREAD_ID pTID, str_map *pGET, str_nmap *pPARAMS);
	void update(THREAD_ID pTID, str_map *pGET, str_nmap *pPARAMS);
	~Helper(void);
	SUser *getSession(void);
	std::wstring generateSession(std::wstring username);
	void OverwriteLanguage(std::string pLanguage);
	ITemplate *createTemplate(std::string name);
	IDatabase *getDatabase(void);
	std::string getRights(const std::string &domain);

	std::string getTimeFormatString(void);

	std::string getLanguage(void);

	void Write(std::string str);
	void WriteTemplate(ITemplate *tmpl);

	void releaseAll(void);

	std::vector<int> getRightIDs(std::string rights);
	bool hasRights(int clientid, std::string rights, std::vector<int> right_ids);

	bool checkPassword(const std::wstring &username, const std::wstring &password, int *user_id);

	std::vector<int> clientRights(const std::string& right_name, bool& all_client_rights);

	std::string getStrippedServerIdentity(void);

	void sleep(unsigned int ms);
private:
	std::string getRightsInt(const std::string &domain);

	SUser* session;
	std::vector<ITemplate*> templates;
	std::string language;

	bool invalid_session;

	str_map *GET;
	str_nmap *PARAMS;

	THREAD_ID tid;
};

struct SStartupStatus
{
	SStartupStatus(void)
		: upgrading_database(false),
		  creating_filescache(false) {}

	bool upgrading_database;
	int curr_db_version;
	int target_db_version;

	bool creating_filescache;
	size_t processed_file_entries;

	IMutex *mutex;
};