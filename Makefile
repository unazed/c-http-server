CCFLAGS :=  -g -Wall -Wno-maybe-uninitialized \
			-Wno-unused-function -Wno-unused-variable -Wno-format-extra-args
CCXFLAGS :=  -Os -Wall -Wno-maybe-uninitialized \
			-Wno-unused-function -Wno-unused-variable -Wno-format-extra-args
INCDIR = include
SRCDIR = src
BUILDDIR = build
BUILDFILE = main
CC = gcc

_PHONY: all release

release:
	${CC} ${CCXFLAGS} -o ${BUILDDIR}/${BUILDFILE}-release ${SRCDIR}/*.c
	@if [ -z $? ]; then \
		strip "./${BUILDDIR}/${BUILDFILE}-release"; \
	fi
all:
	$(CC) $(CCFLAGS) -o ${BUILDDIR}/${BUILDFILE} ${SRCDIR}/*.c 
	@if [ -z $? ]; then \
		make release; \
		./${BUILDDIR}/${BUILDFILE} ./routes "127.0.0.1" "8080"; \
	fi
