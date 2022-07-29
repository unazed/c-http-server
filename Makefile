CCFLAGS :=  -g -DMALLOC_DEBUG -Wall -Wno-maybe-uninitialized \
			-Wno-unused-function -Wno-unused-variable -Wno-format-extra-args \
			-Wno-incompatible-pointer-types
CCXFLAGS :=  -Os -Wall -Wno-maybe-uninitialized \
			-Wno-unused-function -Wno-unused-variable -Wno-format-extra-args \
			-Wno-incompatible-pointer-types
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
		make release > /dev/null 2>&1; \
		./${BUILDDIR}/${BUILDFILE} ./routes "127.0.0.1" "8080"; \
	fi
