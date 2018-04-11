/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   ThreadManager.h
 * Author: harry
 *
 * Created on 10 April 2018, 16:14
 */

#ifndef THREADMANAGER_H
#define THREADMANAGER_H

namespace archsim {
	
		
		/**
		 * This class manages the execution of threads for a single guest
		 * architecture. These threads might be from the same core or different
		 * cores, or from cores in different configurations.
		 * 
		 * This class mainly exists to bridge the gap between guest thread
		 * instances and execution engines.
		 */
		class ThreadManager {
		public:
			
			
			
		private:
			std::vector<ThreadInstance*> thread_instances_;
			ArchDescriptor &arch_descriptor_;
		};

}

#endif /* THREADMANAGER_H */

