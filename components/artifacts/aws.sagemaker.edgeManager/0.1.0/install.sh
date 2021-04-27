#!/bin/bash

read -p "Enter directory path to install [press ENTER use default: /opt/aws/sagemaker_edge]: " WORKING_DIRECTORY
if [[ -z "${WORKING_DIRECTORY}" ]]; then
  WORKING_DIRECTORY=/opt/aws/sagemaker_edge
  echo "Use default directory: ${WORKING_DIRECTORY}"
fi

SYSTEMD_DIR=/etc/systemd/system

if [ -f ${SYSTEMD_DIR}/sagemaker_edge_agent.service ]; then
    systemctl stop sagemaker_edge_agent.service
fi

echo "Installing artifacts ..."
mkdir -p ${WORKING_DIRECTORY}
cp -r bin ${WORKING_DIRECTORY}
cp -r docs ${WORKING_DIRECTORY}
cp *.txt ${WORKING_DIRECTORY}
cp *.md ${WORKING_DIRECTORY}
cp agent.env ${WORKING_DIRECTORY}/agent.env
cp sagemaker_edge_agent.service ${SYSTEMD_DIR}

echo "Set sagemaker_edge_agent.service working directory to ${WORKING_DIRECTORY}"
sed -i "s@REPLACE_SAGEMAKER_EDGE_DIRECTORY@${WORKING_DIRECTORY}@" ${SYSTEMD_DIR}/sagemaker_edge_agent.service

LOG_USER=$(logname)
echo "Run sagemaker_edge_agent.service as user: ${LOG_USER}"
sed -i "s@REPLACE_USER@${LOG_USER}@" ${SYSTEMD_DIR}/sagemaker_edge_agent.service

systemctl enable sagemaker_edge_agent.service
systemctl --no-block start sagemaker_edge_agent.service
systemctl daemon-reload

echo "Install successful!"
