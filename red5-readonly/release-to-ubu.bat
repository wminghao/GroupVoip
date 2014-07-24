./deploy-to-mac.bat
scp -i ~/Develop/GroupVoip/videochat.pem target/red5-server-1.0.2-RC4/red5.jar ubuntu@ec2-54-186-122-59.us-west-2.compute.amazonaws.com:~/howard_files/red5-server.jar
