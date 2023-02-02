pipeline {
    agent any

    stages {
        stage('Build') {
            agent {
                docker {
                    image 'espressif/idf:v4.2.2'
                    args '--rm -v $PWD:/project -w /project'
                    reuseNode true
                }
            }
            steps{
                sh '''
                    #source /opt/esp/idf/export.sh
                    . $IDF_PATH/export.sh
                    idf.py build
                '''
            }
        }
        /* stage('Test') {
            steps {
                echo 'Testing..'
            }
        }
        stage('Deploy') {
            steps {
                echo 'Deploying....'
            }
        } */
    }
}
