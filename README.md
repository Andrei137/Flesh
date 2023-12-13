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
chmod +x build.sh
```

1. Folosind scriptul build.sh (run este optional)
```bash
./build.sh [run]
```

2. Manual
```bash
cmake -B build -S src
cd build
make
cd ../bin
./Flesh
```

---

## Cerinte

- Istoric comenzi
    - [x] limita de cativa MB
    - [x] pe disk, pentru a se salva chiar daca inchidem shell-ul
    - [x] salvare pe disk in caz de Ctrl + C
    - [x] salvare pe disk in caz de kill
    - [x] history         : afiseaza tot istoricul, inclusiv comenzile din sesiunile anterioare
    - [x] history -c      : sterge tot istoricul
    - [x] history -number : afiseaza ultimele [number] comenzi
    - [x] !!              : executa ultima comanda

- Parsing
    - [x] obtinerea comenzii de la utilizator
    - [x] tokenizarea comenzii

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

- Sistemul de foldere
    - [x] pwd
    - [x] cd [path]
    - [ ] cd -
    - [ ] cd ~

- Comenzi custom
    - [x] quit 
    - [x] clear

- Functionalitatea corecta a sagetilor
    - [x] Up, Down -> Istoric comenzi
    - [x] Left, Right -> Navigare prin litere
    - [x] Ctrl + Up, Ctrl + Down -> Navigare prin cuvinte

- Ctrl Signals
    - [x] Ctrl + C (SIGINT)
          - Anuleaza executia comanda curenta
    - [x] Ctrl + Z (SIGTSTP)
          - Suspenda executia comenzii curente
    - [x] Ctrl + \ (SIGQUIT)
          - Inchide Flesh
    - [x] Ctrl + D
          - Inchide Flesh

- Extra
    - [x] Culori (ANSI Escape Sequences)
    - [ ] MultiFlesh

- Variabile de mediu

---

## Resurse

- [Ansi Escape Sequences 1](https://gist.github.com/fnky/458719343aabd01cfb17a3a4f7296797)
- [Ansi Escape Sequences 2](https://en.m.wikipedia.org/wiki/ANSI_escape_code#CSI_sequences)
- [Ascii Art Tool](https://www.asciiart.eu/image-to-ascii)
- [Basic Signal Handling (pentru Ctrl + C)](https://www.gnu.org/software/libc/manual/html_node/Basic-Signal-Handling.html)
- [Execvp (pentru a executa comenzi)](https://linux.die.net/man/3/execvp)
- [Proper handling of SIGINT/SIGQUIT](https://www.cons.org/cracauer/sigint.html)
- [Terminos (pentru functionalitatea sagetilor)](https://man7.org/linux/man-pages/man3/termios.3.html)
- [Terminos syntax configuration with bitwise operators](https://stackoverflow.com/questions/48477989/termios-syntax-configuration-with-bitwise-operators)