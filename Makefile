CCFLAGS :=  -g -Wall -Wno-maybe-uninitialized \
			-Wno-unused-function -Wno-unused-variable -Wno-format-extra-args \
			-Wno-incompatible-pointer-types
CCXFLAGS := -Os -Wall -Wno-maybe-uninitialized \
			-Wno-unused-function -Wno-unused-variable -Wno-format-extra-args \
			-Wno-incompatible-pointer-types
INCDIR = include
SRCDIR = src
TESTDIR = tests
BUILDDIR = build
BUILDFILE = main
TESTFILE = run_tests
CC = gcc

_PHONY: all release test

test:
	${CC} -g -o ${BUILDDIR}/${TESTFILE} ${TESTDIR}/*.c \
					 ${SRCDIR}/hashmap.c ${SRCDIR}/thunks.c ${SRCDIR}/list.c

release:
	${CC} ${CCXFLAGS} -o ${BUILDDIR}/${BUILDFILE}-release ${SRCDIR}/*.c
	@if [ -z $? ]; then \
		strip "./${BUILDDIR}/${BUILDFILE}-release"; \
	fi

all:
	$(CC) $(CCFLAGS) -o ${BUILDDIR}/${BUILDFILE} ${SRCDIR}/*.c 
	@if [ -z $? ]; then \
		make release > /dev/null 2>&1; \
		make test; \
		./${BUILDDIR}/${TESTFILE}; \
		if [ -z $? ]; then \
			./${BUILDDIR}/${BUILDFILE} ./routes "127.0.0.1" "8080"; \
		fi; \
	fi
