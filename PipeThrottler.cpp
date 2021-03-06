#include "PipeThrottler.h"
#include "Server.h"
#include "Interface/Mutex.h"
#include "stringtools.h"

#define DLOG(x) //x

PipeThrottler::PipeThrottler(size_t bps)
	: throttle_bps(bps), curr_bytes(0), lastresettime(0)
{
	mutex=Server->createMutex();
}

PipeThrottler::~PipeThrottler(void)
{
	Server->destroy(mutex);
}

bool PipeThrottler::addBytes(size_t new_bytes, bool wait)
{
	IScopedLock lock(mutex);

	if(throttle_bps==0) return true;

	int64 ctime=Server->getTimeMS();

	if(ctime-lastresettime>1000)
	{
		lastresettime=ctime;
		curr_bytes=0;
	}

	curr_bytes+=new_bytes;

	int64 passed_time=ctime-lastresettime;

	size_t maxRateTime=(size_t)(((_i64)curr_bytes*1000)/throttle_bps);
	if(passed_time>0)
	{
		size_t bps=(size_t)(((_i64)curr_bytes*1000)/passed_time);
		if(bps>throttle_bps)
		{		
			unsigned int sleepTime=(unsigned int)(maxRateTime-passed_time);

			if(sleepTime>0)
			{
				if(wait)
				{
					DLOG(Server->Log("Throttler: Sleeping for " + nconvert(sleepTime)+ "ms", LL_DEBUG));
					Server->wait(sleepTime);

					if(Server->getTimeMS()-lastresettime>1000)
					{
						curr_bytes=0;
						lastresettime=Server->getTimeMS();
					}
				}

				return false;
			}
		}
	}
	else if(curr_bytes>=throttle_bps)
	{
		if(wait)
		{
			DLOG(Server->Log("Throttler: Sleeping for " + nconvert(maxRateTime)+ "ms", LL_DEBUG));
			Server->wait(static_cast<unsigned int>(maxRateTime));
		}
		curr_bytes=0;
		lastresettime=Server->getTimeMS();
		return false;
	}

	return true;
}

void PipeThrottler::changeThrottleLimit(size_t bps)
{
	IScopedLock lock(mutex);

	throttle_bps=bps;
}