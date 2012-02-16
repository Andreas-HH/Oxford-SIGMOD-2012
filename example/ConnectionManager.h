#ifndef _CONNECTION_MANAGER_H_
#define _CONNECTION_MANAGER_H_

#include <common/macros.h>

class DbEnv;

/**
 * Defines a simple connection manager for Berkeley DB.
 * 
 * ConnectionManager is used to build and manage an open Berkeley DB environment.
 * 
 * ConnectionManager implements the Singleton Pattern. 
 */
class ConnectionManager{
 	public:
    // Return the singleton instance of ConnectionManager
		static ConnectionManager& getInstance();
		
    // Return the Berkeley DB environment that will be used
		DbEnv *env(){ return env_; }
    
	private:
		// Private constructor (don't allow instanciation from outside)
		ConnectionManager();

		// Destructor
		~ConnectionManager();
		
		// The Berkeley DB environment that will be used
		DbEnv *env_;

    DISALLOW_COPY_AND_ASSIGN(ConnectionManager);
};

#endif // _CONNECTION_MANAGER_H_
