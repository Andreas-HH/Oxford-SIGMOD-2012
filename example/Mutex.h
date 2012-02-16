// Author: Lukas M. Maas
// An abstraction layer for mutexes (to make locks easier to use and to make the implementation more independent of the actual threading library)
#ifndef _REFERENCE_MUTEX_H_
#define _REFERENCE_MUTEX_H_

#include <pthread.h>

#define lock(x) if(Lock _lock_=x){}else

class Mutex{
	public:
		Mutex(){
			pthread_mutex_init(&mutex_, 0);
		};
		
		~Mutex(){
			pthread_mutex_destroy(&mutex_);
		};
		
		friend class Lock;
		
	private:
		pthread_mutex_t mutex_;
		
		void Lock(){
			pthread_mutex_lock(&mutex_);
		};
		
		void Unlock(){
			pthread_mutex_unlock(&mutex_);
		};
};

class Lock{
	public:
		Lock(Mutex& mutex):mutex_(mutex){mutex_.Lock();};
		~Lock(){mutex_.Unlock();};
		
		operator bool() const {
			return false;
		}
		
	private:
		Mutex& mutex_;
};

#endif // _REFERENCE_MUTEX_H_
