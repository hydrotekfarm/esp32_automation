def EMBEDDED_DEVOPS_HOME = "/c/Users/krish/Documents/hydrotek/devops/hydrotek-devops/cicd/Embedded"
def IDF_PATH = "/c/Users/krish/esp/esp-idf"
def IDF_TOOLS = "/c/Users/krish/esp/esp-idf/tools"
def IDF_PYTHON="/c/Python310/python.exe"
def IDF_REQUIREMENTS="/c/Users/krish/esp/esp-idf/requirements.txt"
def WORK_DIR="/c/Users/krish/Documents/esp32"

def IMAGE_TAG = "pipe_test"
def CLOUD_BUCKET_URL="ota-images-testing-12-11-21/fertigation/${IMAGE_TAG}/app-template.bin"




pipeline{
    agent any
    stages{

        stage(embedded_build){
            steps{
                script{
                     
                    sh """
                    echo "Inside embedded build and push"
                     
                     echo "${IMAGE_TAG} | tee ./version.txt"
                     "${IDF_TOOLS}"/idf.py build

                     
                     git add ./version.txt
                     git commit -m "New release created"
                     echo "git tag -a ${IMAGE_TAG} -m "New Tag: ${IMAGE_TAG}""
                     git tag -a "${IMAGE_TAG}" -m "New Tag: ${IMAGE_TAG}"
                     git push origin "${IMAGE_TAG}"
                     gsutil.cmd cp ./build/app-template.bin gs://"${CLOUD_BUCKET_URL}"
                     gsutil.cmd acl ch -u AllUsers:R gs://"${CLOUD_BUCKET_URL}"
                    """
                }
            }
        }

        stage(embedded_deploy){
            steps{
                script{
                    sh
                    """
                     echo "Inside embedded deployment"

                    pip install -r "${EMBEDDED_DEVOPS_HOME}"/requirements.txt
                	 python "${EMBEDDED_DEVOPS_HOME}/UpdateFirestore.py $PROJECT_NAME ${IMAGE_TAG} https://storage.googleapis.com/${CLOUD_BUCKET_URL} ${EMBEDDED_DEVOPS_HOME}/serviceAccountKey.json"
                     python "${EMBEDDED_DEVOPS_HOME}/mqtt.py ${IMAGE_TAG} https://storage.googleapis.com/${CLOUD_BUCKET_URL}"
                     """
                }
            }
        }
    }
}
