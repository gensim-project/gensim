#!/usr/bin/env groovy

// Avoid parallel execution until it's properly supported in declarative 
// pipelines

pipeline {
	agent any
    options { skipDefaultCheckout() }
	stages {
        stage('Checkout') {
            agent any
            steps {
                sshagent(['10dcbc88-6161-4c34-8955-d11866eaafab']) {
                    sh '''
                        rm -rf gensim-repo
                        hg clone ssh://hg@bitbucket.org/gensim/gensim gensim-repo 
                    '''
                }
            }
        }

        stage ('Build') {
            agent any
            steps {
                dir('gensim-repo') {
                    sh 'make'
                }
            }
        }

        stage ('Run tests') {
            agent any
            steps {
                dir('gensim-repo') {
                    sh 'make tests'
                }
            }
        }
	}
}

