target = cproxy

cc = gcc

compile: $(target)

$(target): $(target).c
	$(cc) $(target).c -o $(target)

clean:
	rm $(target)
