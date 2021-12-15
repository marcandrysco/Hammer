all:

#hammer.cache: hammer.sh
#	./hammer.sh

.PHONY: all
all: hammer.cache
	@./hammer.cache .all

.PHONY: clean
clean: hammer.cache
	@./hammer.cache .clean

.PHONY: run
run: hammer.cache
	@./hammer.cache .run
