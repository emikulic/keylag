CFLAGS?=-O2 -g -Wall

lag: lag.c
	$(CC) $(CFLAGS) $< -lX11 -o $@
