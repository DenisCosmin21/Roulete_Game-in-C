#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <assert.h>
#include <poll.h>

typedef enum
  {
    culoare = 5,
    numar = 6,
    paritate = 7
  }TipPariu;

typedef enum
  {
    par = 0,
    impar = 1
  }TipParitate;

typedef enum
  {
    rosu = 2,
    negru = 3,
    verde = 4
  }color;

typedef union
{
  color culoare_aleasa;//va salva culoare pentru pariurile de tip culoare
  TipParitate paritate_aleasa;//va salva par sau impar pentru pariurile bazate pe paritate
  uint8_t numar;//va salva un numar, pentru pariurile bazate pe numere. Este de uint8_t pentru ca maximul este 255, iar la ruleta exista numere pana la 36 doar
}pariu;
  
typedef struct
{
  TipPariu type;//va salva tipul de pariu
  pariu pariu_ales;//va salva atributul pentru un pariu anume
  int bet;//miza
}bets;

typedef struct//aceasta structura va fi folosita pentru citirea pariului si va salva pariurile puse la un joc
{
  bets *lista; //acesta va salva un pariu anume ce contine miza si tipul de pariu si alegerea facuta
  size_t elements;//va salva numarul de pariuri facute
}BetList;

void show_bet(bets pariu)
{
  const char atr_list[5][10] = {"par", "impar", "rosu", "negru", "verde"};
  printf("Pariu : %d\n", pariu.bet);
  switch(pariu.type)
    {
    case numar:
      printf("numarul : %d\n", pariu.pariu_ales.numar);
      break;
    default:
      if(pariu.type == paritate){
	printf("Tip pariu : paritate -> %s\n", atr_list[pariu.pariu_ales.paritate_aleasa]);
      }
      else{
	printf("Tip pariu : culoare -> %s\n", atr_list[pariu.pariu_ales.culoare_aleasa]);
      }
      break;
    }
}

int din_mem(int size)
{
  return (size >= 4) ? (size / 2 * 3) : size + 1;
}

void *read_bets(void *thread_to_kill)
{
  const int fd = STDIN_FILENO;
  struct pollfd pfd = {.fd = fd, .events = POLLIN};

  if(poll(&pfd, 1, 10000) != 1 || !(pfd.revents & POLLIN)){
    pthread_cancel(*(pthread_t *)(thread_to_kill));
    pthread_exit(NULL);
  }
  
  char *text = NULL;
  int size = 0;
  int max_size = 0;
  ssize_t bytes_read;
  char input[256];
  
  do{
    bytes_read = read(fd, input, sizeof(input));//vom citi maxim sizeof(input) bytes in tabloul input, pentru ca apoi sa le putem copia intr-un string alocat dinamic, pentru a permite adaugarea oricator pariuri.
    if(bytes_read <= 0){//verificam daca am citit macar un byte de continut
      pthread_exit(NULL);
    }

    if(bytes_read < sizeof(input)){//vom aloca memorie necesara pentru a adaquga bytes_read bytes din input in text
      if(bytes_read < sizeof(input)){//in acest caz input-ul este gata astfel ca vom aloca strict memoria necesara pentru bytes respectivi inclusiv \\0
	max_size = max_size + bytes_read + 1;
      }
      else{
	while(bytes_read > max_size - size){
	  max_size = din_mem(max_size);
	}
	if(bytes_read == max_size - size){
	  max_size++;//vom adauga inca un spatiu in memorie pentru a nu avea exact spatiile necesare ca sa putem considera cazul in care nu se mai gasesc mai multe caractere si dorim  sa avem loc de \\0
	}
      }
      text = realloc(text, max_size * sizeof(char));
      assert(text != NULL); //vom verifica pentru eroare la alocare
    }
    memcpy(text + size, input, bytes_read);//vom copia in adresa de memorie a lui text de la ultima pozitie scrisa bytes_read bytes din input.
    size += bytes_read;
    }while((size_t)bytes_read == sizeof(input));/*Dam cast la tipul de date size_t din tipul de date folosit ssize_t*/
  text[size - 1] = '\0';//adaugam caracterul de final de string
  pthread_cancel(*(pthread_t *)thread_to_kill); //vom inchide threadul de timer in cazul in care inputul iese mai repede
  pthread_exit(text);
}
  
void *timer(void *arg)
{
  printf("  :  \x1b[s"); //aceasta combinatie va salva pozitia cursorului la pozitia 4 pentru a o putea refolosi Caracterul \\r este un caracter care ne duce la inceputul liniei respective
  fflush(stdout);
  for(int i = 0;i < 10;i++){
    printf("\r%d : \x1b[u", 10 - i); //vom avea o afisare de forma secunde_ramase : input. In u se aplica cursorul salvat in s, astfel se va muta la pozitia potrivita pentru a nu scrie peste input
    fflush(stdout);//vom folosi fflush pentru a elibera bufferul de la stdout si a-l forta in afisarwa cotninutului
    sleep(1);
    printf("\x1b[s");
  }
  pthread_exit(NULL);
}

int is_number(char *element)
{
  for(;*element != '\0';element++){//vom verifics daca inputul este un numar. Daca nu este numar se returneaza 0, iar daca este numar se returnewza 1
    if(!isdigit(*element)){
      return 0;
    }
  }
  return 1;
}

bets parse_atribute(char *atribut)
{
  bets element;
  char atr_list[5][10] = {"par", "impar", "rosu", "negru", "verde"};//lista atributelor pentru a putea verifica daca etse unul din acestea
  int poz_atribut = 0;
  for(poz_atribut = 0;poz_atribut < 5;poz_atribut++){
    if(strcasecmp(atr_list[poz_atribut], atribut) == 0){
      break;
    }
  }
  if(poz_atribut < 5){
    if(poz_atribut < 2){
      element.type = paritate;
      element.pariu_ales.paritate_aleasa = poz_atribut;
    }
    else{
      element.type = culoare;
      element.pariu_ales.culoare_aleasa = poz_atribut;
    }
  }
  else if(is_number(atribut)){
    element.type = numar;
    element.pariu_ales.numar = strtol(atribut, NULL, 10);
  }
  else{
    element.type = numar;
    element.pariu_ales.numar = 100;
  }
  return element;
}

int miza(char *element)
{
  if(is_number(element) == 0) {
    return 0;
  }
  else{
    return strtol(element, NULL, 10);
  }
}

BetList parse_string(char *text, int balanta)//aici vom da parse la string, vom valida inputul si il vom adauga in strcutura. 
{
  //inputul va fi sub forma atribut_pariu miza,atribut_pariu miza; de exemplu par 10, impar 20, rosu 10, 10 20
  BetList pariuri = {.lista = NULL, .elements = 0};
  bets element;
  char *end_bet = strtok(text, ",");
  char *element_bet = NULL;
  while(end_bet != NULL){
    element_bet = strchr(end_bet, ' ');
    if(element_bet != NULL){
      *element_bet = '\0';
      element_bet++;
    }
    element = parse_atribute(end_bet);
    element.bet = miza(element_bet);
    if(element.bet <= 0){
      element.type = numar;
      element.pariu_ales.numar = 100;
    }
    else{
      balanta -= element.bet;
      if(balanta < 0){
	balanta += element.bet;
	printf("Bet-ul este prea mare. Incercati un bet mai mic sau egal cu %d $\n", balanta);
	element.type = numar;
	element.pariu_ales.numar = 100;
      }
    }
    if((element.type == numar && element.pariu_ales.numar < 37) || element.type != numar){
      pariuri.lista = realloc(pariuri.lista, (pariuri.elements + 1) * sizeof(bets));
      pariuri.lista[pariuri.elements] = element;
      pariuri.elements++;
    }
    end_bet = strtok(NULL, ",");
  }
  return pariuri;
}
      
BetList generate_bets(int balanta)
{
  void *result = NULL;
  pthread_t timer_thread, read_thread; //Vom folosi o abordare de multi thread rendering pentru a putea avea si acest cronometru cu timpul de pariere
  BetList pariuri;
  pthread_create(&timer_thread, NULL, timer, NULL);
  pthread_create(&read_thread, NULL, read_bets, &timer_thread);
  pthread_join(timer_thread, NULL);
  pthread_join(read_thread, &result);
  printf("\e[1;1H\e[2J");
  if(result == NULL){
    printf("Nu s-a pus nici un pariu\n");
    return (BetList){.lista = NULL, .elements = 0};
  }
  else{
    pariuri = parse_string((char*)result, balanta);
    free(result);
    return pariuri;
  }
}      
    
int play_game(BetList lista)
{
  static color numbers[37] = {verde, verde}; //Vom folosi o idee de vector in care vom salva tipul de date, iar pe fiecare pozitie vom salva tipul culorii pentru a nu fi nevoiti sa parcurgem acest tablou non stop. astfel pe pozitia 0 specifica numarului 0 vom salva verde, pe poitia 1 specifica numarului 1 vom salva negru, etc... Vom folosi static pentru a ramane in memorie la fiecare iteratie pentru a nu fi nevoiti de fieacare data sa rulam algoritmul de atribuire al numerelor. Se putea pune si global dar avem nevoie doar in aceasta functie. O verificare de culoare se va face sub forma culoare = numbers[nr_generat]
  if(numbers[1] == verde){//vom genera culorile doar o singura data
    color tip_to_set = rosu;
    for(int i = 1;i <= 36;i++){
      numbers[i] = tip_to_set;
      if(tip_to_set == rosu){
	tip_to_set = negru;
      }
      else{
	tip_to_set = rosu;
      }
      if(i == 10){
	tip_to_set = negru;
      }
      if(i == 28){
	tip_to_set = negru;
      }
    }
  }
  uint8_t nr = rand() % 37;
  color culoare_numar = numbers[nr];
  bets start;
  TipParitate paritate = nr % 2; //1 daca este impar, 0 daca este par
  int balanta_returnata = 0; //daca este negativ balanta va scadea banii pariati, iar daca este pozitiv va incrementa banii.
  char atr_list[5][10] = {"par", "impar", "rosu", "negru", "verde"};
  printf("numarul : %d, culoare :  %s, paritate : %s\n", nr, atr_list[numbers[nr]], atr_list[nr % 2]); //color si paritate va fi in functie de cum s-a atribuit valoarea in enum
  for(int i = 0;i < lista.elements;i++){
    start = lista.lista[i];
    //vom parcurge fiecare element al listei pentru a verifica proprietatile sale
    show_bet(start);
      switch(start.type){
      case culoare:
	if(start.pariu_ales.culoare_aleasa != culoare_numar){
	  balanta_returnata -= start.bet;
	}
	else{
	  switch(start.pariu_ales.culoare_aleasa){
	  case verde:
	    balanta_returnata += start.bet * 14;
	    break;
	  default :
	    balanta_returnata += start.bet;
	    break;
	  }
	}
	break;
      case numar:
	if(nr != start.pariu_ales.numar){
	  balanta_returnata -= start.bet;
	}
	else{
	  balanta_returnata += start.bet * 36;
	}
	break;
      default:
	if(paritate != start.pariu_ales.paritate_aleasa){
	  balanta_returnata -= start.bet;
	}
	else{
	  balanta_returnata += start.bet;
	}
	break;
      }
  }
  return balanta_returnata;
}

void validate_input_balance(int balanta, BetList *lista)
{
  int validare = 0;
  for(int i = 0;i < lista->elements;i++){
    validare += lista->lista[i].bet;
    if(validare > balanta){
      validare -= lista->lista[i].bet;
      lista->elements = i;
      break;
    }
  }
}

int game(int balance)
{
  srand(time(NULL));
  uint8_t optiune = 0;
  BetList lista = {NULL, 0};
  while(balance){
    printf("Alegeti o optiune : \n 1. Play\n2.withdraw_balance\n");
    scanf("%hhu", &optiune);
    getchar();//preluam caracterul de \n de dupa citirea optiunii 
    switch(optiune)
      {
      case 1:
	lista = generate_bets(balance);
	validate_input_balance(balance, &lista);
	balance += play_game(lista);
	printf("balanta dupa joc: %d\n", balance);
	free(lista.lista);
	lista.lista = NULL;
	lista.elements = 0;
	break;
      default:
	return balance;
      }
  }
  return balance;
}

int main(void)
{
  uint8_t op = 0;
  int balanta_final = 100;
  printf("Acesta este un joc de ruleta. Pentru a juca apasati tasta 1, iar pentru a iesi tasta 2. Balanta initiala va fii 100. Inputul trebuie sa fie exact cum este cerut pentru a pune un pariu, si trebuie apasata tasta enter.\n ");
  while(scanf("%hhu", &op)){
    if(op == 2){
      break;
    }
    else if(op == 1){
      printf("\e[1;1H\e[2J");
      balanta_final = game(balanta_final);
      printf("%d ", balanta_final);
    }
    else{
      printf("Optiune gresita\n");
    }
    printf("Acesta este un joc de ruleta. Pentru a juca apasati tasta 1, iar pentru a iesi tasta 2. Balanta initiala va fii 100. Inputul trebuie sa fie exact cum este cerut pentru a pune un pariu, si trebuie apasata tasta enter.\n ");
  }
  return 0;
}
