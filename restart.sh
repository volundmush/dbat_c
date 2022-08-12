#!/bin/sh 
#restart.sh is ment to be run via cron as often as you want. 
#1 minute or 5 minutes is good. It checks to see if your user name 
#has an autorun.sh running, and if not, start one. You'll need to change 
#things for your system etc. 
#1)USER = your local system user name 
#2)AUTORUN = what you start the game with (autorun or autorun.sh) 
#3)MUDPATH = the path to your MUD 
USER=m053car2
AUTORUN=autorun
MUDPATH=~/dbat

ps aux|grep USER|grep AUTORUN|grep -v grep > /dev/null 

if [ $? -eq 1 ] 
  then 
#  echo "MUD is not running!" 
 cd ~/MUDPATH 
   ulimit -c unlimited 
   ./AUTORUN &
fi
