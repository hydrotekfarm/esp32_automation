def EMBEDDED_DEVOPS_HOME = "/c/hydrotek/work/gitsrc/hydrotek-devops/cicd/Embedded"
def IDF_PATH = "/c/Users/Admin/esp/esp-idf"
def IDF_TOOLS = "/c/Users/Admin/esp/esp-idf/tools"
def IDF_PYTHON="/c/Users/Admin/.espressif/python_env/idf4.0_py3.10_env/Scripts/python.exe"
def IDF_REQUIREMENTS="/c/Users/Admin/esp/esp-idf/requirements.txt"
def WORK_DIR="/c/hydrotek/work/gitsrc/"

def IMAGE_TAG = "pipe_test"
def CLOUD_BUCKET_URL="ota-images-testing-12-11-21/fertigation/${IMAGE_TAG}/app-template.bin"




pipeline{
    agent any
    environment{
        TERM = 'xterm-color'
    }
    stages{

        stage(embedded_build){
            steps{
                script{
                     
                    sh """
                    echo "Inside embedded build and push"
                     
                     echo "${IMAGE_TAG} | tee ./version.txt"
                     "${IDF_PYTHON}" "${IDF_TOOLS}"/idf.py build

                     
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
