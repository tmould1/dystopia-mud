#!/bin/bash
set -xe


# Copy war file from S3 bucket to tomcat webapp folder
aws s3 cp s3://dystopiadeploystack-webappdeploymentbucket-1cb8cgtryq6p9/dystopia /usr/local/mudder/bin/dystopia


# Ensure the ownership permissions are correct.
chown -R mudder:mudder /usr/local/mudder/bin