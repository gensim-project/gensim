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
                        hg clone ssh://hg@bitbucket.org/gensim/gensim-repo gensim-repo 
                    '''
                }
            }
        }
        stage('Update Subcomponents') {
            agent any
            steps {
                dir('gensim-repo') {
                    sshagent(['10dcbc88-6161-4c34-8955-d11866eaafab']) {
                        sh '''
                            cd gensim; hg pull; hg up; cd -
                            cd archsim; hg pull; hg up; cd -
                            cd support/libtrace; hg pull; hg up; cd -
                            cd models/armv7; hg pull; hg up; cd -
                            cd models/armv8; hg pull; hg up; cd -
                            cd models/risc-v; hg pull; hg up; cd -
                        '''
                    }
                }
            }
        }

        stage ('Prepare build environment') {
            agent any
            steps {
                dir('gensim-repo') {
                    sh 'rsync --update archsim/scripts/bare-archsim-config archsim/.config'
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
                    sh 'make -C gensim/ tests'
                }
            }
        }

        stage ('Create new commit') {
            agent any
            steps {
                dir('gensim-repo') {
                    sshagent(['10dcbc88-6161-4c34-8955-d11866eaafab']) {
                        // Create and push a new commit if changes are detected (meaning that subrepos have been updated)
                        sh 'sh scripts/make-jenkins-commit.sh'
                    }
                }
            }
        }
	}
}

