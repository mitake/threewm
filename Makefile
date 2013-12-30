CFLAGS = -Os -Wall -std=c++11
LDFLAGS = -L/usr/X11R6/lib -lX11
OBJS = threewm.o screen.o keys.o events.o client.o
HDRS = threewm.h screen.h keys.h events.h client.h

threewm: $(OBJS) $(HDRS) config_key.def
	g++ -o threewm $(OBJS) $(LDFLAGS) $(CFLAGS)

.cc.o:
	g++ -c $(CFLAGS) $<

clean:
	$(RM) -f $(OBJS) threewm

cscope:
	find . -name "*\.cc" > cscope.files
	find . -name "*\.h" >> cscope.files
	find . -name "*\.def" >> cscope.files
	cscope -b -q
