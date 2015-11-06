#!/bin/bash
#script to back up ifes site

echo "script in progress"

NOW=$(date +"%m%d%Y-%H%M%S")
echo $NOW

echo "Working directory:"
pwd

DAY_OF_WEEK=`date '+%a'`
echo "Today is ".$DAY_OF_WEEK

FILE="ifes-db-$DAY_OF_WEEK.tar.gz"
echo "Today's file will be ".$FILE

cd /var/www/vhosts/ifes/public
echo "New working directory"
pwd

BACKUP_DIR=/var/www/vhosts/ifes/backups

echo "starting backup"
usr/bin/drush sql-dump | gzip -9 > $BACKUP_DIR./.$FILE

ll $BACKUP_DIR

DONE=$(date +"%m%d%Y-%H%M%S")
echo "script finished successfully at ".$DONE