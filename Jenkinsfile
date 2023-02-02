pipeline {
    agent any

    stages {
        stage('Build') {
            
            steps{
                sh '''
                cd /c/Users/krish/Documents/esp32
                    docker run --rm -v --privileged "$PWD:/esp32_automation" -w /project espressif/idf 
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
