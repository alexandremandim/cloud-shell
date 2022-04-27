all: servidor cloudShell clean

servidor: readLine
	gcc ./servidorContabilizacao.c ./readln -o servidor
cloudShell: readLine
	gcc ./cloudShell.c ./readln -o cloud
readLine:
	gcc -c ./libs/readln.c -o readln
clean:
	rm readln

cleanAll: clean
	rm servidor clouds


iniciarServidor:
	gnome-terminal -e "bash -c './servidor'"
	
iniciarServidorECliente: iniciarServidor
	./cloud
