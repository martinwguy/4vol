4vol: 4vol.c
	$(CC) $(CFLAGS) -o 4vol 4vol.c -ljack

clean:
	rm -f 4vol
