COMPONENT_NAME=$1
NAMESPACE=$2

if [ $# -ne 2 ]
then
    echo "generate_component.sh COMPONENT_NAME NAMESPACE"
    exit 1
fi

if [ ! -d "../../$NAMESPACE" ]
then
    echo "namespace $NAMESPACE does not exist"
    exit 1
fi

OutputPath="../../$NAMESPACE/$COMPONENT_NAME"
Aixml="${OutputPath}/${COMPONENT_NAME}ComponentAi.xml"

mkdir -p $OutputPath
cp ./CubeRoverComponentTemplate/CubeRoverComponentTemplateAi.xml $Aixml
cp ./CubeRoverComponentTemplate/Makefile $OutputPath
cp ./CubeRoverComponentTemplate/mod.mk $OutputPath
sed -i "s/{{ COMPONENT_NAME }}/$COMPONENT_NAME/g" $Aixml
sed -i "s/{{ COMPONENT_NAME }}/$COMPONENT_NAME/g" ${OutputPath}/Makefile
sed -i "s/{{ COMPONENT_NAME }}/$COMPONENT_NAME/g" ${OutputPath}/mod.mk
sed -i "s/{{ NAMESPACE }}/$NAMESPACE/g" $Aixml
sed -i "s/{{ NAMESPACE }}/$NAMESPACE/g" ${OutputPath}/Makefile
echo "Don't forget to add the component to fprime/mk/configs/modules/modules.mk under CubeRover_MODULES"
