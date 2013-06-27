#! /bin/sh

files=`find . -name '*.h' | grep -v uinet | cut -d / -f 2- | xargs`

for _file in ${files}; do
	_guard=`echo ${_file} | tr a-z/. A-Z__`
	( head -25 ${_file} ; echo '#ifndef	'${_guard} ; echo '#define	'${_guard} ; ( cat ${_file} | tail -n +28 | sed '$d' ) ; echo '#endif /* !'${_guard}' */' ) > x
	cat x > ${_file}
	rm x
done
