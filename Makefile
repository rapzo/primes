CC = gcc
CFLAGS = -Wall -pthread -lrt -lm
APP = primes
rm = rm -f
DBG = -g

SRCDIR = src
OBJDIR = obj
BINDIR = bin
SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(BINDIR)/$(APP): $(OBJECTS)
	$(CC) -o $@ $^ $(CFLAGS)
	@echo "\nCompiling complete! Use following the example to run the application:"
	@echo "./bin/primes <n>\t\tWhere <n> is the limit for which all primes should be found."

$(OBJDIR)/shared_memory.o: $(SRCDIR)/circular_queue.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/circular_queue.o: $(SRCDIR)/shared_memory.c
	$(CC) -c $< -o $@ $(CFLAGS)

$(OBJDIR)/primes.o: $(SRCDIR)/primes.c
	$(CC) -c $< -o $@ $(CFLAGS)

debug: $(SOURCES)
	$(CC) $(DBG) $^ $(CFLAGS)

clean:
	@$(rm) $(OBJDIR)/*.o $(BINDIR)/$(APP)

.PHONY: clean
