CC = gcc

OBJDIR = build
TARGET = $(OBJDIR)/out
SRCS = waitforwakeup.c parse_config.c main.c
OBJS = $(addprefix $(OBJDIR)/, $(SRCS:.c=.o))
CFLAGS=$(shell pkg-config --cflags libelogind) -Wextra -Wall
LDFLAGS=$(shell pkg-config --libs libelogind)

all: debug

debug: CFLAGS += -g -O0 -DDEBUG_BUILD
debug: $(OBJDIR) $(TARGET)

release: CFLAGS += -O3
release: $(OBJDIR) $(TARGET)
	strip $(TARGET)
	rm -v $(OBJDIR)/*.o
	
clean:
	rm -frv $(OBJDIR)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

$(OBJDIR)/%.o: %.c
	@mkdir -pv $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -pv $(OBJDIR)

