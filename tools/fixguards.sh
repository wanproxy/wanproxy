#! /bin/sh

files=`find . -name '*.h' | cut -d / -f 2- | xargs`

for _file in ${files}; do
	_guard=`echo ${_file} | tr a-z/. A-Z__`
	( echo '#ifndef	'${_guard} ; echo '#define	'${_guard} ; ( cat ${_file} | tail -n +3 | sed '$d' ) ; echo '#endif /* !'${_guard}' */' ) > x
	cat x > ${_file}
	rm x
done
