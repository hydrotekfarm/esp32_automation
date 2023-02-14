def CI_WORK_DIR = "~/hydrotek/cicdbuild/"
def CONTAINER_REGISTRY = "gcr.io/hydrotek-286213"
def NAMESPACE = "dev"
def NETWORKTYPE = "LoadBalancer"
def PORTNUM = 4400
def GCLOUD_PATH = "/c/gcloud/google-cloud-sdk/bin"
def DOCKER_PATH = "/usr/local/bin"
def KUBE_PATH = "/usr/local/bin"
def EMBEDDED_DEVOPS_HOME = "/c/hydrotek/work/gitsrc/hydrotek-devops/cicd/Embedded"
def IDF_PATH = "/c/Users/Admin/esp/esp-idf"
def IDF_TOOLS = "/c/Users/Admin/esp/esp-idf/tools"
def IDF_PYTHON="/c/Users/Admin/.espressif/python_env/idf4.0_py3.10_env/Scripts/python.exe"
def IDF_REQUIREMENTS="/c/Users/Admin/esp/esp-idf/requirements.txt"
def WORK_DIR="/c/hydrotek/work/gitsrc/"
#parameters
def project = "project"
def BRANCH = "Nithin-ota-testing"
def IMAGE_TAG = "pipe_test"

switch(project)
{
    case "apiserver":
        def IMAGE_NAME="apiserver"
        def PROJECT_NAME="apiserver"
        def REPOS_NAME="apiserver-app"
        def DEPLOYMENT="hydrotekapp-deployment"
    break;
    case "mqttclient":
        def PROJECT_NAME="mqttclient"
        def IMAGE_NAME="hydromqttclient"
        def REPOS_NAME="raspberry-auto"
        def DEPLOYMENT="hydromqttdeployment"
    break;
    case "fertigationsystem":
        def PROJECT_NAME="fertigation-system"
        def IMAGE_NAME=""
        def REPOS_NAME="esp32_automation"
        def CLOUD_BUCKET_URL="ota-images-testing-12-11-21/fertigation/${IMAGE_TAG}/app-template.bin"
    break;
    case "climatecontroller":
        def PROJECT_NAME="climate-controller"
        def IMAGE_NAME=""
        def REPOS_NAME="climate-controller-bot"
        def CLOUD_BUCKET_URL="ota-images-testing-12-11-21/climate/${IMAGE_TAG}/app-template.bin"
    break;
}


pipeline{
    agent any
    stages{
        stage(embedded_checkout){
            steps{
                script{
                    sh echo "Inside embedded checkout"

                    sh """
                    cd $IDF_PATH
                    $IDF_PYTHON -m pip install -r $IDF_REQUIREMENTS
                    ./install.sh

                    source export.sh

                    cd ${WORK_DIR}
                    # Check if repos is already cloned
                    if [ -d "${REPOS_NAME}" ]; then
                    cd ${REPOS_NAME}
                    git pull
                    else
                    git clone -b ${BRANCH} git@gitlab.com:iot15/${REPOS_NAME}.git
                    cd ${REPOS_NAME}
                    fi
                    """
                }
            }
        }

        stage(embedded_build){
            steps{
                script{
                    sh echo "Inside embedded build and push"
                    sh """
                     #Build
                     echo ${IMAGE_TAG} | tee ./version.txt
                     $IDF_TOOLS/idf.py build

                     #Push
                     git add ./version.txt
                     git commit -m "New release created"
                     echo "git tag -a ${IMAGE_TAG} -m "New Tag: ${IMAGE_TAG}""
                     git tag -a ${IMAGE_TAG} -m "New Tag: ${IMAGE_TAG}"
                     git push origin ${IMAGE_TAG}
                     gsutil.cmd cp ./build/app-template.bin gs://${CLOUD_BUCKET_URL}
                     gsutil.cmd acl ch -u AllUsers:R gs://${CLOUD_BUCKET_URL}
                    """
                }
            }
        }

        stage(embedded_deploy){
            steps{
                script{
                    sh echo "Inside embedded deployment"

                    sh pip install -r ${EMBEDDED_DEVOPS_HOME}/requirements.txt
                	sh python ${EMBEDDED_DEVOPS_HOME}/UpdateFirestore.py $PROJECT_NAME $IMAGE_TAG https://storage.googleapis.com/${CLOUD_BUCKET_URL} ${EMBEDDED_DEVOPS_HOME}/serviceAccountKey.json
                    sh python ${EMBEDDED_DEVOPS_HOME}/mqtt.py ${IMAGE_TAG} https://storage.googleapis.com/${CLOUD_BUCKET_URL}
                }
            }
        }
    }
}
