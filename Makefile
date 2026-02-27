SUBDIRS := utility client broker cli

all:
	@for dir in $(SUBDIRS); do \
	  echo "=== Building $$dir ==="; \
	  $(MAKE) -C $$dir || exit 1; \
	done

clean:
	@for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir clean; \
	done

.PHONY: all clean
