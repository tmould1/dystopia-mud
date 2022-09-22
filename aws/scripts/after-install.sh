#!/bin/bash
set -xe


# Copy war file from S3 bucket to tomcat webapp folder
aws s3 cp s3://codedeploystack-webappdeploymentbucket-1queegksp9hba/dystopia /usr/local/mudder/bin/dystopia


# Ensure the ownership permissions are correct.
chown -R tomcat:tomcat /usr/local/mudder/bin