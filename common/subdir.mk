all install regress clean:
	@for _dir in ${SUBDIR}; do					\
		echo "==> ${SUBMAKE_DIR}$$_dir ($@)" ;			\
		(cd $$_dir &&						\
		 SUBMAKE_DIR=${SUBMAKE_DIR}$$_dir/ ${MAKE} $@) ||	\
		exit $$? ; \
	done
