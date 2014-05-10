#! /bin/sh
# vim: si ai

if ! svn log -q $1 > /tmp/_.svn; then
	exit $?
fi

grep -v ^- /tmp/_.svn > /tmp/_.svn.1
mv /tmp/_.svn.1 /tmp/_.svn

getyear() {
	line=''
	read line
	if echo "$line" | grep -q '^r1 '; then
		echo 2008
	else
		echo "$line" | cut -d ' ' -f 5 | cut -d - -f 1
	fi
}

range() {
	if [ "$1" = "$2" ]; then
		echo $1
	else
		echo "$1"-"$2"
	fi
}

lastyear=`head -1 /tmp/_.svn | getyear`
firstyear=`tail -1 /tmp/_.svn | getyear`

years=`range $firstyear $lastyear`

sed -i '' "s/Copyright (c) [0-9-]* Juli Mallett/Copyright (c) $years Juli Mallett/" $1
