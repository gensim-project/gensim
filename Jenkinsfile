#!/usr/bin/env groovy

// Avoid parallel execution until it's properly supported in declarative 
// pipelines

pipeline {
	agent any
    
	stages {
        stage ('Build') {
            agent any
            steps {
				sh 'mkdir -p build; cd build; cmake .. -DTESTING_ENABLED=TRUE; make'
            }
        }

        stage ('Run tests') {
            agent any
            steps {
				sh 'cd build; ctest --verbose || true'
            }
        }
	}
}

