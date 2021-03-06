#pragma once
#define NO_INTERFACE

#ifdef THREAD_BOOST
#	include <boost/thread/thread.hpp>
#else
#ifdef _WIN32
#else
#	include <pthread.h>
#endif
#endif

#include "Interface/Server.h"
#include "Interface/Action.h"
#include <vector>
#include <fstream>

typedef void(*LOADACTIONS)(IServer*);
typedef void(*UNLOADACTIONS)(void);

#ifdef _WIN32
#include <windows.h>
#else
typedef void *HMODULE;
#endif

class FCGIRequest;
class IDatabaseInt;
class IDatabaseFactory;
class CSessionMgr;
class CServiceAcceptor;
class CThreadPool;
class IOutputStream;

struct SDatabase
{
	SDatabase(IDatabaseFactory *factory, const std::string &file) : factory(factory), file(file) {}
	IDatabaseFactory *factory;
	std::string file;
	std::map<THREAD_ID, IDatabaseInt*> tmap;
	std::vector<std::pair<std::string,std::string> > attach;
};


class CServer : public IServer
{
public:
	CServer();
	~CServer();
	void setup(void);
	void setServerParameters(const str_nmap &pServerParams);

	virtual std::string getServerParameter(const std::string &key);
	virtual std::string getServerParameter(const std::string &key, const std::string &def);
	virtual void setServerParameter(const std::string &key, const std::string &value);
	virtual void setLogLevel(int LogLevel);
	virtual void setLogFile(const std::string &plf, std::string chown_user="");
	virtual void setLogCircularBufferSize(size_t size);
	virtual std::vector<SCircularLogEntry> getCicularLogBuffer(size_t minid);
	virtual void Log(const std::string &pStr, int LogLevel=LL_INFO);
	virtual void Log(const std::wstring &pStr, int LogLevel=LL_INFO);
	virtual bool Write(THREAD_ID tid, const std::string &str, bool cached=true);
	virtual bool WriteRaw(THREAD_ID tid, const char *buf, size_t bsize, bool cached=true);

	virtual void setContentType(THREAD_ID tid, const std::string &str);
	virtual void addHeader(THREAD_ID tid, const std::string &str);

	THREAD_ID Execute(const std::wstring &action, const std::wstring &context, str_map &GET, str_map &POST, str_nmap &PARAMS, IOutputStream *req);
	std::string Execute(const std::wstring &action, const std::wstring &context, str_map &GET, str_map &POST, str_nmap &PARAMS);

	virtual void AddAction(IAction *action);
	virtual bool RemoveAction(IAction *action);
	virtual void setActionContext(std::wstring context);
	virtual void resetActionContext(void);

	virtual int64 getTimeSeconds(void);
	virtual int64 getTimeMS(void);

	virtual bool LoadDLL(const std::string &name);
	virtual bool UnloadDLL(const std::string &name);

	virtual void destroy(IObject *obj);

	virtual void wait(unsigned int ms);

	virtual ITemplate* createTemplate(std::string pFile);
	virtual IMutex* createMutex(void);
	virtual ICondition* createCondition(void);
	virtual IPipe *createMemoryPipe(void);
	virtual void createThread(IThread *thread);
	virtual IThreadPool *getThreadPool(void);
	virtual ISettingsReader* createFileSettingsReader(std::string pFile);
	virtual ISettingsReader* createDBSettingsReader(THREAD_ID tid, DATABASE_ID pIdentifier, const std::string &pTable, const std::string &pSQL="");
	virtual ISettingsReader* createDBSettingsReader(IDatabase *db, const std::string &pTable, const std::string &pSQL="");
	virtual ISettingsReader* createMemorySettingsReader(const std::string &pData);
	virtual IPipeThrottler* createPipeThrottler(size_t bps);

	virtual bool openDatabase(std::string pFile, DATABASE_ID pIdentifier, std::string pEngine="sqlite");
	virtual IDatabase* getDatabase(THREAD_ID tid, DATABASE_ID pIdentifier);
	virtual void destroyAllDatabases(void);
	virtual void destroyDatabases(THREAD_ID tid);

	virtual ISessionMgr *getSessionMgr(void);
	virtual IPlugin* getPlugin(THREAD_ID tid, PLUGIN_ID pIdentifier);

	virtual THREAD_ID getThreadID(void);
	
	virtual std::string ConvertToUTF8(const std::wstring &input);
	virtual std::wstring ConvertToUnicode(const std::string &input);
	virtual std::string ConvertToUTF16(const std::wstring &input);
	virtual std::string ConvertToUTF32(const std::wstring &input);
	virtual std::wstring ConvertFromUTF16(const std::string &input);
	virtual std::wstring ConvertFromUTF32(const std::string &input);

	virtual std::string GenerateHexMD5(const std::string &input);
	virtual std::string GenerateBinaryMD5(const std::string &input);
	virtual std::string GenerateHexMD5(const std::wstring &input);
	virtual std::string GenerateBinaryMD5(const std::wstring &input);

	virtual void StartCustomStreamService(IService *pService, std::string pServiceName, unsigned short pPort, int pMaxClientsPerThread=-1, IServer::BindTarget bindTarget=IServer::BindTarget_All);
	virtual IPipe* ConnectStream(std::string pServer, unsigned short pPort, unsigned int pTimeoutms);
	virtual IPipe *PipeFromSocket(SOCKET pSocket);
	virtual void DisconnectStream(IPipe *pipe);

	virtual bool RegisterPluginPerThreadModel(IPluginMgr *pPluginMgr, std::string pName);
	virtual bool RegisterPluginThreadsafeModel(IPluginMgr *pPluginMgr, std::string pName);

	virtual PLUGIN_ID StartPlugin(std::string pName, str_map &params);

	virtual bool RestartPlugin(PLUGIN_ID pIdentifier);

	virtual unsigned int getNumRequests(void);
	virtual void addRequest(void);

	virtual IFile* openFile(std::string pFilename, int pMode=0);
	virtual IFile* openFile(std::wstring pFilename, int pMode=0);
	virtual IFile* openFileFromHandle(void *handle);
	virtual IFile* openTemporaryFile(void);
	virtual IFile* openMemoryFile(void);
	virtual bool deleteFile(std::string pFilename);
	virtual bool deleteFile(std::wstring pFilename);

	virtual POSTFILE_KEY getPostFileKey();
	virtual void addPostFile(POSTFILE_KEY pfkey, const std::string &name, const SPostfile &pf);
	virtual SPostfile getPostFile(POSTFILE_KEY pfkey, const std::string &name);
	virtual void clearPostFiles(POSTFILE_KEY pfkey);

	virtual std::wstring getServerWorkingDir(void);
	void setServerWorkingDir(const std::wstring &wdir);

	void ShutdownPlugins(void);

	void setTemporaryDirectory(const std::wstring &dir);

	virtual void registerDatabaseFactory(const std::string &pEngineName, IDatabaseFactory *factory);
	virtual bool hasDatabaseFactory(const std::string &pEngineName);

	virtual bool attachToDatabase(const std::string &pFile, const std::string &pName, DATABASE_ID pIdentifier);

	static int WriteDump(void* pExceptionPointers);

	virtual void waitForStartupComplete(void);
	void startupComplete(void);

	void shutdown(void);

	void initRandom(unsigned int seed);
	virtual unsigned int getRandomNumber(void);
	virtual std::vector<unsigned int> getRandomNumbers(size_t n);
	virtual void randomFill(char *buf, size_t blen);

	virtual unsigned int getSecureRandomNumber(void);
	virtual std::vector<unsigned int> getSecureRandomNumbers(size_t n);
	virtual void secureRandomFill(char *buf, size_t blen);

	virtual void setFailBit(size_t failbit);
	virtual void clearFailBit(size_t failbit);
	virtual size_t getFailBits(void);

	void clearDatabases(THREAD_ID tid);

	void setLogRotationFilesize(size_t filesize);

	void setLogRotationNumFiles(size_t numfiles);

private:

	void logToCircularBuffer(const std::string& msg, int loglevel);

	bool UnloadDLLs(void);
	void UnloadDLLs2(void);

	void rotateLogfile();


	int loglevel;
	bool logfile_a;
	std::fstream logfile;

	IMutex* log_mutex;
	IMutex* action_mutex;
	IMutex* requests_mutex;
	IMutex* outputs_mutex;
	IMutex* db_mutex;
	IMutex* thread_mutex;
	IMutex* plugin_mutex;
	IMutex* rps_mutex;
	IMutex* postfiles_mutex;
	IMutex* param_mutex;
	IMutex* startup_complete_mutex;
	IMutex* rnd_mutex;

	ICondition *startup_complete_cond;
	bool startup_complete;

	std::map< std::wstring, std::map<std::wstring, IAction*> > actions;

	std::map<std::string, UNLOADACTIONS> unload_functs;
	std::vector<HMODULE> unload_handles;

	std::map<THREAD_ID, IOutputStream*> current_requests;
	std::map<THREAD_ID, std::pair<bool, std::string> > current_outputs;

	THREAD_ID curr_thread_id;
#ifdef THREAD_BOOST
	std::map<boost::thread::id, THREAD_ID> threads;
#else
#ifdef _WIN32
#else
	std::map<pthread_t, THREAD_ID> threads;
#endif
#endif

	std::map<DATABASE_ID, SDatabase > databases;

	CSessionMgr *sessmgr;

	std::vector<CServiceAcceptor*> stream_services;

	std::map<PLUGIN_ID, std::map<THREAD_ID, IPlugin*> > perthread_plugins;
	std::map<std::string, IPluginMgr*> perthread_pluginmgrs;
	std::map<PLUGIN_ID, std::pair<IPluginMgr*,str_map> > perthread_pluginparams;

	std::map<std::string, IPluginMgr*> threadsafe_pluginmgrs;
	std::map<PLUGIN_ID, IPlugin*> threadsafe_plugins;

	std::map<POSTFILE_KEY, std::map<std::string, SPostfile > > postfiles;
	POSTFILE_KEY curr_postfilekey;

	str_nmap server_params;

	PLUGIN_ID curr_pluginid;

	unsigned int num_requests;

	CThreadPool* threadpool;

	std::wstring action_context;

	std::wstring workingdir;

	std::wstring tmpdir;

	std::map<std::string, IDatabaseFactory*> database_factories;

	size_t circular_log_buffer_id;

	std::vector<SCircularLogEntry> circular_log_buffer;
	size_t circular_log_buffer_idx;

	bool has_circular_log_buffer;

	size_t failbits;

	std::string logfile_fn;

	std::string logfile_chown_user;

	size_t log_rotation_size;

	size_t log_rotation_files;
};

#ifndef DEF_SERVER
extern CServer *Server;
#endif
