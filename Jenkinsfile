#!/usr/bin/env groovy

// Avoid parallel execution until it's properly supported in declarative 
// pipelines

pipeline {
	agent any
    
	stages {
        stage ('Build') {
            agent any
            steps {
				sh 'make'
            }
        }

        stage ('Run tests') {
            agent any
            steps {
				sh 'make TESTING_ENABLED=TRUE test || true'
            }
        }
	}
}

