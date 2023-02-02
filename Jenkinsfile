pipeline {
    agent any

    stages {
        stage('Build') {
            
            steps{
                sh '''
                    docker run --rm -v $PWD:/project -w /project espressif/idf idf.py build
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
