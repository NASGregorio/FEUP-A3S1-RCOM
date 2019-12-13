src = $(wildcard src/*.c)
inc := include
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
prog = download

CFLAGS = -I$(inc) -Wall -Werror

$(prog): $(obj)
	$(CC) -o $@ $^ $(CFLAGS)

-include $(dep)

%.d: %.c
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(prog)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: reset
reset: clean cleandep

.PHONY: fresh
fresh: reset $(prog)
