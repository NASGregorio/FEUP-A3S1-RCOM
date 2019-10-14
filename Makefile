src = $(wildcard *.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)
prog = rcom

CFLAGS = -Wall -Werror

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
