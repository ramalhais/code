pipeline {
  agent any
  stages {
    stage('PList Files') {
      parallel {
        stage('List Files') {
          steps {
            sh 'ls -la'
            sh 'pwd'
          }
        }
        stage('List Files2') {
          steps {
            sh 'ls -laR'
          }
        }
      }
    }
    stage('Check disk space') {
      steps {
        sh 'df -h'
      }
    }
  }
  environment {
    dbConnectionString = 'mysql://localhost/bla'
  }
}