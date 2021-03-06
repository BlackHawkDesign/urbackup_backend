#include <vector>
#include <utility>
#include "Interface/Thread.h"
#include "Interface/Mutex.h"
#include "Interface/Condition.h"
#include "Interface/Pipe.h"
#include "socket_header.h"
#include "Interface/CustomClient.h"

const int MAX_CLIENTS=20;

class IService;
class CStreamPipe;

class CServiceWorker : public IThread
{
public:
	CServiceWorker(IService *pService, std::string pName, IPipe * pExit, int pMaxClientsPerThread);
	~CServiceWorker();

	void operator ()(void);

	int getAvailableSlots(void);
	void AddClient(SOCKET pSocket, const std::string& endpoint);

	void stop(void);

private:
    
	void addNewClients(void);

	std::vector<std::pair<ICustomClient*, CStreamPipe*> > clients;
	std::vector<std::pair<SOCKET, std::string> > new_clients;

	IMutex* mutex;
	IMutex* nc_mutex;
	ICondition* cond;
	IPipe *exit;

	int nClients;
	int max_clients;

	THREAD_ID tid;

	std::string name;

	IService *service;

	volatile bool do_stop;
};
