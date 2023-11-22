# Proiect Shell Sisteme De Operare

---

- Numele echipei: Inner Shell
- Grupa: 252
- Membrii: Buzatu Giulian, Ilie Dumitru, Neculae Andrei-Fabian

---

## Cerinte

- Istoric comenzi
    - [ ] pe disk, pentru a se salva chiar daca inchidem shell-ul
    - [ ] limita de cativa MB

- Piping Operators
    - [ ] '|'
        - Output-ul primei comenzi este input pentru a doua comanda
        - Usage: cat logfile.txt | sort |  uniq -c
    - [ ] '>'
        - Redirectioneaza output-ul catre o fila txt (overwrite) 
        - Usage: echo "Hello World!" > output.txt
    - [ ] '>>'
        - Redirectioneaza output-ul catre o fila txt (append)
        - Usage: echo "Hello World!" > output.txt
    - [ ] '<'
        - Preia input-ul dintr-o fila txt
        - Usage: sort < input.txt

- Expresii logice
    - [ ] '&&'
        - Executa comanda urmatoare doar daca prima comanda a avut succes
        - Usage: gcc test.cpp -o test && ./test
    - [ ] '||'
        - Executa comanda urmatoare doar daca prima comanda a avut esec
        - Usage: gcc test.cpp -o test || echo "Compilation failed"
    - [ ] '!'
        - Inverseaza rezultatul comenzii
        - Usage: if ! [ -e "test.txt" ]; then echo "File does not exist"; fi

- Control Flow
    - [ ] ';'
        - Permite executia mai multor comenzi pe aceeasi linie
        - Usage: sleep 10; echo "Hello World!"
    - [ ] '&'
        - Permite executia mai multor comenzi in background
        - Usage: echo "Hello World 1!" & sleep 10 & echo "Hello World 2!"

- Suspendarea unui program
    - [ ] Ctrl + Z

- Sistemul de foldere
    - [ ] Pwd
    - [ ] Cd

- Clear

- Variabile de mediu

---

## Resurse

- [Ascii Art Tool](https://www.asciiart.eu/image-to-ascii)
