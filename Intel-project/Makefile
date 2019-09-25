## ======================================================================
## partial Makefile provided to students
##

CFLAGS = -std=c11 -Wall -Wpedantic -g

# a bit more checks if you'd like to (uncomment
# CFLAGS += -Wextra -Wfloat-equal -Wshadow                         \
# -Wpointer-arith -Wbad-function-cast -Wcast-align -Wwrite-strings \
# -Wconversion -Wunreachable-code

# uncomment if you want to add DEBUG flag
# CPPFLAGS += -DDEBUG

# ---------------------------------------------------------------------- 
# feel free to update/modifiy this part as you wish

# all those libs are required on Debian, feel free to adapt it to your box

LDLIBS += -lcheck -lm -lrt -pthread -lsubunit

all:: test-cache #change all for each test?????
	gcc -g -std=c11 -MM *.c $(LDLIBS) -DDEBUG

#test-addr: test-addr.o tests.h util.h addr.h addr_mng.o error.o
#test-commands: test-commands.o tests.h util.h commands.h commands.o error.o

test-cache: test-cache.o cache_mng.o memory.o page_walk.o cache_mng.o error.o test-cache.o commands.o addr_mng.o

memory.o: memory.c memory.h addr.h page_walk.h addr_mng.h util.h error.h
page_walk.o: page_walk.c page_walk.h addr.h error.h addr_mng.h memory.h
cache_mng.o: cache_mng.c cache_mng.h mem_access.h addr.h cache.h error.h \
addr_mng.c lru.h
error.o: error.c
test-cache.o:test-cache.c error.h cache_mng.h mem_access.h addr.h \
cache.h commands.h memory.h page_walk.h
commands.o: commands.c commands.h mem_access.h addr.h addr_mng.h error.h
addr_mng.o: addr_mng.c error.h addr.h


# ----------------------------------------------------------------------
# This part is to make your life easier. See handouts how to make use of it.

clean::
	-@/bin/rm -f *.o *~ $(CHECK_TARGETS)

new: clean all

static-check:
	scan-build -analyze-headers --status-bugs -maxloop 64 make CC=clang new

style:
	astyle -n -o -A8 *.[ch]

# all those libs are required on Debian, adapt to your box
$(CHECK_TARGETS): LDLIBS += -lcheck -lm -lrt -pthread -lsubunit

check:: $(CHECK_TARGETS)
$(foreach target,$(CHECK_TARGETS),./$(target);)

# target to run tests
check:: all
	@if ls tests/*.*.sh 1> /dev/null 2>&1; then \
      for file in tests/*.*.sh; do [ -x $$file ] || echo "Launching $$file"; ./$$file || exit 1; done; \
    fi

IMAGE=chappeli/pps19-feedback
feedback:
	@docker run -it --rm -v ${PWD}:/home/tester/done $(IMAGE)

SUBMIT_SCRIPT=../provided/submit.sh
submit1: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 1

submit2: $(SUBMIT_SCRIPT)
	@$(SUBMIT_SCRIPT) 2

submit:
	@printf 'what "make submit"??\nIt'\''s either "make submit1" or "make submit2"...\n'