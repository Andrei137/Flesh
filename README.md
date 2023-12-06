# Proiect Shell Sisteme De Operare

---

- Numele echipei: Inner Shell
- Grupa: 252
- Membrii: Buzatu Giulian, Ilie Dumitru, Neculae Andrei-Fabian

---

## Utilizare
- Inainte de a rula, trebuie sa ne asiguram ca avem instalat cmake
```bash
sudo apt install cmake
```

- Daca se foloseste scriptul, este necesar sa ii dam drepturi de executie
```bash
chmod +x run.sh
```

1. Folosind scriptul run.sh
```bash
./run.sh
```

2. Manual
```bash
cmake .
make
./Flesh
```

---

## Cerinte

- Istoric comenzi
    - [x] limita de cativa MB
    - [x] pe disk, pentru a se salva chiar daca inchidem shell-ul
    - [x] salvare pe disk in caz de Ctrl + C
    - [ ] salvare pe disk in caz de kill
    - Utilizare
        - history        : afiseaza tot istoricul, inclusiv comenzile din sesiunile anterioare
        - history -c     : sterge tot istoricul
        - history -number: afiseaza ultimele [number] comenzi

- Piping
    - [ ] Operatorul |
        - Output-ul primei comenzi este input pentru a doua comanda
        - Utilizare: cat logfile.txt | sort | uniq -c

- Redirectionare
    - [ ] Operatorul >
        - Redirectioneaza output-ul catre o fila txt (overwrite) 
        - Utilizare: echo "Hello World!" > output.txt
    - [ ] Operatorul >>
        - Redirectioneaza output-ul catre o fila txt (append)
        - Utilizare: echo "Hello World!" > output.txt
    - [ ] Operatorul <
        - Preia input-ul dintr-o fila txt
        - Utilizare: sort < input.txt

- Operatori Logici
    - [ ] Operatorul &&
        - Executa comanda urmatoare doar daca prima comanda a avut succes
        - Utilizare: gcc test.cpp -o test && ./test
    - [ ] Operatorul ||
        - Executa comanda urmatoare doar daca prima comanda a avut esec
        - Utilizare: gcc test.cpp -o test || echo "Compilation failed"

- Control Flow
    - [ ] Separatorul ;
        - Permite executia mai multor comenzi secvential
        - Utilizare: sleep 10; echo "Hello World!"
    - [ ] Separatorul '&'
        - Permite executia mai multor comenzi in background
        - Utilizare: echo "Hello World 1!" & sleep 10 & echo "Hello World 2!"
    - [ ] Ctrl + Z
        - Suspenda executia comenzii curente

- Sistemul de foldere
    - [x] pwd
    - [x] cd

- Comenzi custom
    - [x] quit 
    - [x] clear

- Functionalitatea corecta a sagetilor
    - [x] Up, Down -> Istoric comenzi
    - [x] Left, Right -> Navigare cu cursorul

- Variabile de mediu

---

## Resurse

- [Ascii Art Tool](https://www.asciiart.eu/image-to-ascii)
- [Basic Signal Handling (pentru Ctrl + C)](https://www.gnu.org/software/libc/manual/html_node/Basic-Signal-Handling.html)
- [CSI Sequences](https://en.m.wikipedia.org/wiki/ANSI_escape_code#CSI_sequences)
- [Execvp (pentru a executa comenzi)](https://linux.die.net/man/3/execvp)
- [Terminos (pentru functionalitatea sagetilor)](https://man7.org/linux/man-pages/man3/termios.3.html)
- [Terminos syntax configuration with bitwise operators](https://stackoverflow.com/questions/48477989/termios-syntax-configuration-with-bitwise-operators)