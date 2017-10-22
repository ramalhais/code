pipeline {
  agent any
  stages {
    stage('PList Files') {
      parallel {
        stage('List Files') {
          steps {
            sh 'date; ls -laR; date'
          }
        }
        stage('List Files2') {
          steps {
            sh 'date; ls -laR; date'
          }
        }
        stage('Print Environment') {
          steps {
            sh 'date; echo $dbConnectionString; date; sleep 5; date'
          }
        }
      }
    }
    stage('Check disk space') {
      steps {
        sh 'date; df -h; date'
      }
    }
  }
  environment {
    dbConnectionString = 'mysql://localhost/bla'
  }
}