#include <stdio.h>		// Libreria di base
#include <stdlib.h>		// Libreria di base
#include <unistd.h>		// Libreria di base
#include <string.h>		// Libreria per stringhe
#include <signal.h>		// Libreria per segnali
#include <sys/types.h>		// Libreria per chiave
#include <sys/wait.h>		// Libreria per pipe
#include <fcntl.h>		// Libreria per puntatore con FIFO
#include <sys/shm.h>		// Libreria per memoria condivisa
#include <sys/msg.h>		// Libreria per messaggi
#include <pthread.h>		// Libreria per thread
#include <sys/sem.h>		// Libreria per semafori System-V
#include <sys/ipc.h>		// Libreria per Inter Process Comunication
#include <netinet/in.h>		// Libreria per socket   
#include <sys/socket.h>		// Libreria per socket 
#include <arpa/inet.h>		// Libreria per socket 
#include <netdb.h>		// Libreria per socket 
#include <time.h>		// Libreria per data
#include <sys/stat.h>		// Librerie per creazione e controllo cartelle
#define PORTA 5000		// Porta comunicazione Client-Server
#define MASSIMOCONNESSIONI 10	// Numero massimo connessioni
#define OPERATORI 20		// Numero operatori GarageMoto

// Dichiarazione strutture
// Struttura base del messaggio
struct{
    long tipoMessaggio;
    char testoMessaggio[300];
}messaggio;
// Struttura per la creazione della configurazione della moto
typedef struct{
    /* Colori proposti ---> 1 - Bianco Corsa, 2 - Blu Metallic, 3 - Blu Metallizzato Ducati, 4 - Giallo Ducati, 5 - Grigio Steel Ducati, 6 - Nero Metallic, 7 - Nero Sport, 8 - Rosso Ducati, 9 - Rosso Metallic, 10 - Viola Metallic
       Diametro cerchi proposti ---> 1 - R17, 2 - R18, 3 - R19, 4 - R20, 5 - R21
       Frizioni proposte ---> 1 - Antisaltellamento, 2 - Automatica, 3 - Disco singolo, 4 - Idraulica, 5 - Multidisco
       Opzioni menu' proposte ---> 1 - Creazione configurazione, 2 - Caricamento configurazione, 3 - Chat di assistenza, 4 - Chiusura programma
       Modelli proposti ---> 1 - Hypermotard 950 (937 cc, 114 CV), 2 - Monster (937 cc, 111 CV), 3 - Monster 1200 (1198 cc, 147 CV), 4 - Multistrada V2 (937 cc, 113 CV), 5 - Multistrada V4 (1158 cc, 170 CV), 6 - Panigale V2 (955 cc, 155 CV), 7 - Panigale V4 (1103 cc, 215 CV), 8 - Streetfighter V2 (955 cc, 153 CV), 9 - Streetfighter V4 (1103 cc, 208 CV), 10 - SuperSport 950 (937 cc, 110 CV)
       Scarichi proposti ---> 1 - Akrapovic, 2 - Arrow, 3 - Giannelli, 4 - GPR, 5 - LeoVince, 6 - Malossi, 7 - Mivv, 8 - Polini, 9 - Termignoni, 10 - Yoshimura
       Selle proposte ---> 1 - Carbonio, 2 - Ecopelle, 3 - Pelle, 4 - Sintetico, 5 - Velluto
       Telai proposti ---> 1 - A traliccio, 2 - A tubi saldati, 3 - Misto (In parte a tubi saldati ed in parte stampato), 4 - Monoscocca, 5 - Stampato
       Pneumatici proposti ---> 1 - Da competizione, 2 - Da fuoristrada, 3 - Da strada, 4 - Misti (Da strada e da fuoristrada), 5 - Sportivi 
       Pedane proposte ---> 1 - Acciaio, 2 - Alluminio, 3 - Ergal, 4 - Gomma, 5 - Titanio 
       Cavalletti proposti ---> 1 - Con forcellone monobraccio, 2 - Con forchette, 3 - Con perni orizzontali e verticali, 4 - Con piastre, 5 - Con ruote girevoli */
    unsigned short menu;
    unsigned short modello;
    unsigned short colore;
    unsigned short diametroCerchi;
    unsigned short sella;
    unsigned short scarico;
    unsigned short frizione;
    unsigned short telaio;
    unsigned short pneumatici;
    unsigned short pedane;
    unsigned short cavalletto;
    int prezzo;						// Prezzo finale configurazione
    int grandezza;					// Grandezza nome configurazione
    char nomeConfigurazione[100];			// Nome configurazione
}numeroSeriale;
// Struttura per il caricamento della configurazione della moto
typedef struct{
    char modello[100];
    char colore[100];
    char diametroCerchi[100];
    char sella[100];
    char scarico[100];
    char frizione[100];
    char telaio[100];
    char pneumatici[100];
    char pedane[100];
    char cavalletto[100];
    int prezzo;
}dati;
// Dichiarazione struttura per controllo semafori System-V
union semun{
    int valore;						// Valore per SETVAL
    struct semid_ds *buffer;				// Buffer per IPC_STAT e IPC_SET
    unsigned short *array;				// Array per GETALL e SETALL
    struct seminfo *__buffer;				// Buffer per IPC_INFO (Specifica di Linux)
};

// Dichiarazione variabili globali
numeroSeriale seriale;					// Struttura con scelte utente
int IDCoda;						// ID coda di messaggi
char nomeUtente[100];					// Nome utente ricevuto dal client
int flag1 = 1;						// Flag da mandare al client
int contatore,IDNuovoSocket,IDSocket;			// Contatore, ID nuovo socket, ID socket
int IDSemaforo;						// ID semaforo
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;	// Inizializzazione statica Mutex
pthread_cond_t flagger;					// Inizializzazione variabile condizione
pid_t controlloSeriale;					// PID per analisi seriale
dati *memoriaCondivisa;					// Puntatore a memoria condivisa

// Dichiarazione prototipi funzioni
int deallocazioneSemaforo(int IDSemaforo);
int inizializzazioneSemaforo(int IDSemaforo);
int bloccaSemaforo(int IDSemaforo);
int sbloccaSemaforo(int IDSemaforo);
void handlerExit(int segnale);
void inizializzatoreIndirizzo(struct sockaddr_in *indirizzo,int porta,long indirizzoIP);
void *stampaThread1();
void *stampaThread2();

// Sviluppo main thread
int main(){
    // Fase 1 ---> Creazione socket e inizializzazione connessione
    signal(SIGQUIT,handlerExit);
    char nomeOperatore[20];				// Nome operatore GarageMoto
    char uscita[] = "Chiudi\n";				// Generazione stringa comparazione per uscita coda
    const char* cartella;				// Controllo esistenza cartella
    cartella = "Ducati GarageMoto";
    struct stat struttura;
    int IDMemoriaCondivisa,risultato,thread1,thread2;	// Creazione ID memoria condivisa ed interi da associare a system call (risultato per fork,thread1,thread2)
    struct sockaddr_in indirizzoServer;			// Creazione struttura indirizzo server
    struct sockaddr_in indirizzoClient;			// Creazione struttura indirizzo client
    struct tm * informazioniTempo;			// Struttura per ottenere data
    int flagCoda = 0;					// Creazione flag per coda messaggi client-server
    int lunghezzaClient = sizeof(indirizzoClient);	// Creazione lunghezza client ed associazione indirizzo client a lunghezza client
    int fd[2];						// Creazione pipe
    int operatoreScelto;				// Numero per posizione operatore in array con nomi operatori
    key_t chiave = ftok(".",'x');			// Generazione chiave con ftok
    pthread_t thread_1,thread_2;			// ID thread
    pid_t esecuzione,flag,suddivisione;			// Creazione variabili per processi
    FILE *puntatoreFile;				// Puntatore a file
    time_t dataCorrente;				// Variabile contenente data corrente
    pthread_cond_init(&flagger,NULL);			// Inizializzazione variabile condizione per thread
    // Fase 1.1 ---> Inizializzazione server e tempo
    time(&dataCorrente);
    informazioniTempo = localtime(&dataCorrente);
    printf("[SERVER][GarageDucati] In esecuzione...\n");
    inizializzatoreIndirizzo(&indirizzoServer,PORTA,INADDR_ANY);				// Inizializzazione indirizzo relativo con funzione
    IDSocket = socket(PF_INET,SOCK_STREAM,0);							// Inizializzazione socket
    if(IDSocket == -1){										// Situazione errore creazione socket
	    perror("\nErrore in fase di creazione del socket!\n");
	    exit(EXIT_FAILURE);
    }
    risultato = bind(IDSocket,(struct sockaddr *)&indirizzoServer,sizeof(indirizzoServer));	// Bind server su porta
    if(risultato == -1){									// Situazione errore fallimento bind
	    perror("\nErrore in fase di bind!\n");
	    exit(EXIT_FAILURE);
    }
    if(listen(IDSocket,MASSIMOCONNESSIONI) < 0){						// Listen per ascolto client su porta
	    perror("\nErrore in fase di listen!\n");
	    exit(EXIT_FAILURE);
    }
    // Fase 1.2 ---> Connessione client a server
    printf("\n[SERVER][GarageDucati] In attesa del client sulla porta: %d\nTerminazione server: 'Ctrl + \\'\n",ntohs(indirizzoServer.sin_port));
    if(pipe(fd) == -1){					// Generazione pipe
	    perror("\nErrore in fase di creazione della pipe!\n");
	    exit(EXIT_FAILURE);
    }
    while(1){
	    IDNuovoSocket = accept(IDSocket,(struct sockaddr *)&indirizzoClient,(socklen_t*)&lunghezzaClient);				// Accettazione connessione client
	    if(IDNuovoSocket == -1){			// Situazione errore accettazione connessione
	        perror("\nErrore in fase di creazione del socket\n");
	        exit(EXIT_FAILURE);
	    }
	    sleep(1);
	    // Fase 2 ---> Gestione client
	    suddivisione = fork();			// Generazione processo figlio
	    contatore++;				// Incremento contatore
	    switch(suddivisione){
	        case -1:				// Situazione errore fork
		        perror("\nErrore in fase di creazione del processo figlio tramite fork!\n");
	        break;
            case 0:					// Gestione thread e connessione client 1
                printf("\n[SERVER][GarageDucati] Connessione n°%d stabilita!\n[SERVER][FIGLIO %d - PID: %d - PPID: %d][GarageDucati]\nPorta server: %d\nPorta client: %d\n",contatore,contatore,getpid(),getppid(),PORTA,ntohs(indirizzoClient.sin_port));
                close(IDSocket);
                // Fase 3 ---> Creazione thread e ricezione menu dal client
		        if((thread1 = pthread_create(&thread_1,NULL,stampaThread1,NULL)) == -1){
		            perror("\nErrore in fase di creazione del thread!\n");
		            exit(EXIT_FAILURE);
		        }
		        printf("[SERVER][GarageDucati] Thread avviato con successo...\n");
		        recv(IDNuovoSocket,&seriale.menu,sizeof(numeroSeriale),0);	// Ricezione scelta menu' client
		        IDSemaforo = semget(IPC_PRIVATE,1,IPC_CREAT|0666);		// Allocazione semaforo System-V
		        switch(seriale.menu){
		            // Fase 3.1 ---> Stampa su file, creazione cartella e spostamento file in cartella
                    case 1:{
                        printf("\n[SERVER][FIGLIO %d - PID: %d - PPID: %d][GarageDucati] Scelta ricevuta dal menu': %d.\nConfigurazione ricevuta dal client!\n",contatore,getpid(),getppid(),seriale.menu);
			            recv(IDNuovoSocket,&seriale,sizeof(seriale),0);
			            inizializzazioneSemaforo(IDSemaforo);
			            bloccaSemaforo(IDSemaforo);				// Blocco semaforo System-V
			            char testo[seriale.grandezza];			// Generazione stringa di dimensione nome utente
			            strcpy(testo,seriale.nomeConfigurazione);		// Copiatura nome in stringa
			            puntatoreFile = fopen(testo,"a+");			// Apertura file con successiva scrittura
			            if(puntatoreFile == NULL){				// Situazione errore apertura file
			                perror("\nErrore in fase di apertura del file!\n");
			                exit(EXIT_FAILURE);
			            }
			            fprintf(puntatoreFile,"Data creazione configurazione: %s",asctime(informazioniTempo));
			            fprintf(puntatoreFile,"Codice Seriale: %hd-%hd-%hd-%hd-%hd-%hd-%hd-%hd-%hd-%hd\n",seriale.modello,seriale.colore,seriale.diametroCerchi,seriale.sella,seriale.scarico,seriale.frizione,seriale.telaio,seriale.pneumatici,seriale.pedane,seriale.cavalletto);
			            fprintf(puntatoreFile,"Prezzo totale IVA inclusa: € %d,00\n",seriale.prezzo);
			            fprintf(puntatoreFile,"Inserire nel programma il seriale per visualizzare l'intera configurazione");
			            fclose(puntatoreFile);				// Chiusura file
			            sbloccaSemaforo(IDSemaforo);			// Sblocco semaforo System-V
			            if(deallocazioneSemaforo(IDSemaforo) >= 0){		// Deallocazione semaforo System-V
			                printf("\n[SERVER][GarageDucati] Semaforo terminato e file di testo generato!\n");
			            }
			            flag = fork();					// Creazione processo nipote
			            switch(flag){
			                case -1:
                                perror("\nErrore in fase di creazione del processo nipote tramite fork!\n");
			                break;
			                case 0:
				                for(int i = 1;i < 3;i++){		// Creazione due processi pronipoti per creazione cartella e spostamento file in cartella
                                    esecuzione = fork();
                                    switch(esecuzione){
                                        case -1: 
                                            perror("\nErrore creazione fork!\n");
                                        case 0: 
                                            if(i == 1){
                                                if(stat(cartella,&struttura) == 0 && S_ISDIR(struttura.st_mode)){
                                                    printf("[SERVER][GarageDucati] La cartella gia' esiste\n");
                                                    exit(EXIT_SUCCESS);
                                                }
                                                else{
                                                    printf("[SERVER][GarageDucati] Creazione della cartella...\n");
                                                    execl("/bin/mkdir","mkdir","Ducati GarageMoto",NULL);
                                                    exit(EXIT_SUCCESS);
                                                }
                                            }
                                            if(i == 2){
                                                printf("[SERVER][GarageDucati] Spostamento del file...\n");
                                                execl("/bin/mv","mv",testo,"Ducati GarageMoto",NULL);
                                                exit(EXIT_SUCCESS);
                                            }
                                        default:
                                            wait(NULL);
                                    }
				                }
				                unsigned short i = 1;
				                close(fd[0]);					// Chiusura lettura su pipe
				                write(fd[1],&i,sizeof(unsigned short));		// Scrittura su pipe
			               	    sleep(1);
			                break;
			                default:
				                wait(NULL);
				                close(fd[1]);					// Chiusura scrittura su pipe
				                unsigned short controllo;			// Lettura da pipe
				                read(fd[0],&controllo,sizeof(unsigned short));
				                if(controllo == 1){
				                    printf("[SERVER][GarageDucati] Procedura di creazione configurazione eseguita correttamente!\n");
				                    send(IDNuovoSocket,&flag1,sizeof(flag1),0);		// Invio flag a client
				                    fflush(stdout);
				                    sleep(1);
				                    puts("");
				                    printf("\n[SERVER][GarageDucati] Ritorno in background...\n");
				                    exit(EXIT_SUCCESS);
				                }
				                else{
				                    perror("\nErrore in fase di sviluppo della procedura!\n");
				                    exit(EXIT_FAILURE);
				                }
			                break;
			            }
		                break;
		            }
                    // Fase 3.2 ---> Controllo seriale e restituzione configurazione
		            case 2:
			            // Fase 3.2.1 ---> Inizializzazione memoria condivisa
			            
			            IDMemoriaCondivisa = shmget(IPC_PRIVATE,sizeof(dati),IPC_CREAT|0666);		// Generazione memoria condivisa
			            if(IDMemoriaCondivisa < 0){
			                perror("\nErrore in fase di generazione della memoria condivisa tramite shmget!\n");
			                exit(EXIT_FAILURE);
			            }
			            memoriaCondivisa = (dati*)shmat(IDMemoriaCondivisa,NULL,0);		// Attach a memoria condivisa
			            if(memoriaCondivisa == NULL){
			                perror("\nErrore in fase di attach alla memoria condivisa tramite shmat!\n");
			                exit(EXIT_FAILURE);
			            }
			            printf("\n[SERVER][FIGLIO %d - PID: %d - PPID: %d][GarageDucati] Scelta ricevuta dal menu': %d.\nSegnale ricevuto dal client!\n",contatore,getpid(),getppid(),seriale.menu);
			            recv(IDNuovoSocket,&flag1,sizeof(flag1),0);		// Ricezione flag da client per proseguire
			            sleep(1);
			            pthread_cond_signal(&flagger);			// Invio segnale con condizione a thread
			            pthread_join(thread_1,NULL);			// Attesa terminazione thread
			            sleep(1);
			            if((thread2 = pthread_create(&thread_2,NULL,stampaThread2,NULL)) != 0){		// Creazione thread 2 per invio configurazione a client
			                perror("\nErrore in fase di creazione del thread 2!\n");
			                exit(EXIT_FAILURE);
			            }
			            pthread_join(thread_2,NULL);			// Attesa terminazione thread 2
			            printf("[SERVER][GarageDucati] Procedura di caricamento configurazione eseguita correttamente!\n");
			            sleep(1);
			            puts("");
				    printf("\n[SERVER][GarageDucati] Ritorno in background...\n");
			            fflush(stdout);
			            exit(EXIT_SUCCESS);
		            break;
		            // Fase 3.3 ---> Chat con operatore 
		            case 3:
		                recv(IDNuovoSocket,&nomeUtente,sizeof(nomeUtente),0);	// Ricezione nome utente da client
		                srand(time(NULL));
		                operatoreScelto = 1+rand()%OPERATORI;
		                switch(operatoreScelto){
		                    case 1:
		                        strcpy(nomeOperatore,"Alberto");
		                    break;
		                    case 2:
		                        strcpy(nomeOperatore,"Bernardo");
		                    break;
		                    case 3:
		                        strcpy(nomeOperatore,"Cesare");
		                    break;
		                    case 4:
		                        strcpy(nomeOperatore,"Davide");
		                    break;
		                    case 5:
		                        strcpy(nomeOperatore,"Edoardo");
		                    break;
		                    case 6:
		                        strcpy(nomeOperatore,"Francesca");
		                    break;
		                    case 7:
		                        strcpy(nomeOperatore,"Guido");
		                    break;
		                    case 8:
		                        strcpy(nomeOperatore,"Ilaria");
		                    break;
		                    case 9:
		                        strcpy(nomeOperatore,"Jennifer");
		                    break;
		                    case 10:
		                        strcpy(nomeOperatore,"Katia");
		                    break;
		                    case 11:
		                        strcpy(nomeOperatore,"Luisa");
		                    break;
		                    case 12:
		                        strcpy(nomeOperatore,"Manuele");
		                    break;
		                    case 13:
		                        strcpy(nomeOperatore,"Nicola");
		                    break;
		                    case 14:
		                        strcpy(nomeOperatore,"Olga");
		                    break;
		                    case 15:
		                        strcpy(nomeOperatore,"Paola");
		                    break;
		                    case 16:
		                        strcpy(nomeOperatore,"Raimondo");
		                    break;
		                    case 17:
		                        strcpy(nomeOperatore,"Susanna");
		                    break;
		                    case 18:
		                        strcpy(nomeOperatore,"Teresa");
		                    break;
		                    case 19:
		                        strcpy(nomeOperatore,"Umberta");
		                    break;
		                    case 20:
		                        strcpy(nomeOperatore,"Virginio");
		                    break;
		                    default:
		                        printf("Errore in fase di ricerca di un operatore GarageMoto!\n");
		                    break;
		                }
		                send(IDNuovoSocket,&operatoreScelto,sizeof(operatoreScelto),0);
		                send(IDNuovoSocket,&nomeOperatore,sizeof(nomeOperatore),0);
		                // Fase 3.3.1 ---> Inizializzazione coda di messaggi
			            if((IDCoda = msgget(chiave,IPC_CREAT|0666)) == -1){		// Generazione coda di messaggi
		                    perror("\nErrore in fase di generazione della coda di messaggi tramite msgget!\n");
			                exit(EXIT_FAILURE);
			            }
			            messaggio.tipoMessaggio = 1;
			            // Fase 3.3.2 ---> Conversazione client-server
			            printf("\n[SERVER][FIGLIO %d - PID: %d - PPID: %d][GarageDucati] Scelta ricevuta dal menu': %d.\nIn attesa di un messaggio dall'utente %s...\n\n",contatore,getpid(),getppid(),seriale.menu,nomeUtente);
			            while(flagCoda == 0){
			                msgrcv(IDCoda,&messaggio,sizeof(messaggio),1,0);	// Ricezione messaggio da client
			                sleep(1);
			                printf("[Utente %s]: %s",nomeUtente,messaggio.testoMessaggio);
			                if(strcmp(messaggio.testoMessaggio,uscita) == 0){	// Controllo coincidenza tra messaggio ricevuto e stringa creata in precedenza
			                    flagCoda = 1;
			                    printf("\n[SERVER][GarageDucati] Conversazione terminata dall'utente!\n");
			                    msgctl(IDCoda,IPC_RMID,NULL);			// Chiusura coda di messaggi
			                }
			                else{
			                    printf("[%s]: ",nomeOperatore);
			                    fgets(messaggio.testoMessaggio,sizeof(messaggio.testoMessaggio),stdin);
			                    msgsnd(IDCoda,&messaggio,sizeof(messaggio),0);	// Invio messaggio a client
			                    if(strcmp(messaggio.testoMessaggio,uscita) == 0){	// Controllo coincidenza tra messaggio inviato e stringa creata in precedenza
			                        flagCoda = 1;
				                    printf("\n[SERVER][GarageDucati] Conversazione terminata!\n");
				                    msgctl(IDCoda,IPC_RMID,NULL);		// Chiusura coda di messaggi
				                }
			                }
			            }
			            sleep(1);
			            puts("");
				    printf("\n[SERVER][GarageDucati] Ritorno in background...\n");
			            fflush(stdout);
			            exit(EXIT_SUCCESS);
		            break;
		            // Fase 3.4 ---> Chiusura programma
		            case 4:
		                printf("\n[SERVER][FIGLIO %d - PID: %d - PPID: %d][GarageDucati] Scelta ricevuta dal menu': %d.\nChiusura del client in corso...\n",contatore,getpid(),getppid(),seriale.menu);
		                sleep(1);
		                puts("");
				printf("\n[SERVER][GarageDucati] Ritorno in background...\n");
		                fflush(stdout);
		                exit(EXIT_SUCCESS);
		            break;
		            // Fase 3.5 ---> Situazione di selezione non valida
		            default:
		                printf("\n[SERVER][FIGLIO %d - PID: %d - PPID: %d] Client arrestato in modo anomalo!\n",contatore,getpid(),getppid());
		                puts("");
				printf("\n[SERVER][GarageDucati] Ritorno in background...\n");
		                fflush(stdout);
		                exit(EXIT_FAILURE);
		            break;
		        }
		        // Fase 4 ---> Connessione client
		        if(close(IDNuovoSocket) == 0){
		            printf("[SERVER][FIGLIO %d - PID: %d - PPID: %d][GarageDucati] Terminazione client del sistema con:\nPorta server: %d\nPorta client: %d\n",contatore,getpid(),getppid(),PORTA,ntohs(indirizzoClient.sin_port));
		            exit(EXIT_SUCCESS);
		        }
	        default:
		        puts("");
	    }
    }
    // Fase 5 ---> Terminazione connessione e chiusura programma
    close(IDNuovoSocket);		// Chiusura socket di client
    close(IDSocket);			// Chiusura socket di server
    shmdt(memoriaCondivisa);		// Detach memoria condivisa
    printf("[SERVER][GarageDucati] Connessione terminata\n");
    exit(EXIT_SUCCESS);
}

// Sviluppo funzioni
// Inizializzazione indirizzo
void inizializzatoreIndirizzo(struct sockaddr_in *indirizzo,int porta,long indirizzoIP){
    indirizzo->sin_family = AF_INET;
    indirizzo->sin_port = htons(PORTA);
    indirizzo->sin_addr.s_addr = htonl(indirizzoIP);
};
// Deallocazione semaforo System-V
int deallocazioneSemaforo(int IDSemaforo){
    union semun semaforoDeallocato;
    return semctl(IDSemaforo,0,IPC_RMID,semaforoDeallocato);
};
// Inizializzazione semaforo System-V
int inizializzazioneSemaforo(int IDSemaforo){
    union semun semaforoInizializzato;
    unsigned short valori[1];
    valori[0] = 1;
    semaforoInizializzato.array = valori;
    return semctl(IDSemaforo,0,SETALL,semaforoInizializzato);
};
// Blocco semaforo System-V
int bloccaSemaforo(int IDSemaforo){
    struct sembuf operazioniBlocco[1];
    operazioniBlocco[0].sem_num = 0;
    operazioniBlocco[0].sem_op = -1;
    operazioniBlocco[0].sem_flg = SEM_UNDO;
    return semop(IDSemaforo,operazioniBlocco,1);
};
// Sblocco semaforo System-V
int sbloccaSemaforo(int IDSemaforo){
    struct sembuf operazioniSblocco[1];
    operazioniSblocco[0].sem_num = 0;
    operazioniSblocco[0].sem_op = 1;
    operazioniSblocco[0].sem_flg = SEM_UNDO;
    return semop(IDSemaforo,operazioniSblocco,1);
};
// Handler segnale ---> Chiusura server
void handlerExit(int segnale){
    printf("\n[SERVER][GarageDucati] Chiusura server...\n");
    sleep(1);
    fflush(stdout);
    close(IDNuovoSocket);		// Chiusura socket di client
    close(IDSocket);			// Chiusura socket di server
    shmdt(memoriaCondivisa);		// Detach memoria condivisa
    printf("[SERVER][GarageDucati] Connessione terminata con successo!\n");
    exit(EXIT_SUCCESS);
};
// Thread ---> Switch e invio dati a memoria condivisa
void *stampaThread1(){
    dati configurazione;						// Struttura per scrittura scelte utente su file
    pthread_mutex_lock(&mutex);						// Blocco Mutex
    pthread_cond_wait(&flagger,&mutex);					// Attesa variabile condizione
    recv(IDNuovoSocket,&seriale,sizeof(seriale),0);			// Ricezione seriale da client
    sleep(3);
    printf("\n[SERVER][GarageDucati] Seriale dati ricevuto!\n");
    pthread_mutex_unlock(&mutex);					// Sblocco Mutex
    pthread_cond_destroy(&flagger);					// Eliminazione variabile condizione
    printf("[SERVER][GarageDucati] Controllo del seriale...\n\n");	// Controllo seriale con switch
    sleep(1);
    switch(seriale.modello){
    	case 1:
	    printf("[SERVER][GarageDucati] Modello scelto: Hypermotard 950\n");
            strcpy(configurazione.modello,"Modello: Ducati Hypermotard 950");
       	    configurazione.prezzo += 13290;
	    break;
	    case 2:
            printf("[SERVER][GarageDucati] Modello scelto: Monster\n");
            strcpy(configurazione.modello,"Modello: Ducati Monster");
            configurazione.prezzo += 11690;
	    break;
	    case 3:
            printf("[SERVER][GarageDucati] Modello scelto: Monster 1200\n");
            strcpy(configurazione.modello,"Modello: Ducati Monster 1200");
            configurazione.prezzo += 14390;
        break;
        case 4:
            printf("[SERVER][GarageDucati] Modello scelto: Multistrada V2\n");
            strcpy(configurazione.modello,"Modello: Ducati Multistrada V2");
            configurazione.prezzo += 14990;
	    break;
	    case 5:
            printf("[SERVER][GarageDucati] Modello scelto: Multistrada V4\n");
            strcpy(configurazione.modello,"Modello: Ducati Multistrada V4");
            configurazione.prezzo += 18990;
	    break;
	    case 6:
            printf("[SERVER][GarageDucati] Modello scelto: Panigale V2\n");
            strcpy(configurazione.modello,"Modello: Ducati Panigale V2");
            configurazione.prezzo += 18490;
        break;
        case 7:
            printf("[SERVER][GarageDucati] Modello scelto: Panigale V4\n");
            strcpy(configurazione.modello,"Modello: Ducati Panigale V4");
            configurazione.prezzo += 24590;
	    break;
	    case 8:
            printf("[SERVER][GarageDucati] Modello scelto: Streetfighter V2\n");
            strcpy(configurazione.modello,"Modello: Ducati Streetfighter V2");
            configurazione.prezzo += 16990;
	    break;
	    case 9:
            printf("[SERVER][GarageDucati] Modello scelto: Streetfighter V4\n");
            strcpy(configurazione.modello,"Modello: Ducati Streetfighter V4");
            configurazione.prezzo += 20690;
        break;
        case 10:
            printf("[SERVER][GarageDucati] Modello scelto: SuperSport 950\n");
            strcpy(configurazione.modello,"Modello: Ducati SuperSport 950");
            configurazione.prezzo += 14290;
        break;
    }
    switch(seriale.colore){
	    case 1:
	        printf("[SERVER][GarageDucati] Colore scelto: Bianco Corsa\n");
	        strcpy(configurazione.colore,"Colore: Bianco Corsa");
	        configurazione.prezzo += 750;
	    break;
	    case 2:
	        printf("[SERVER][GarageDucati] Colore scelto: Blu Metallic\n");
	        strcpy(configurazione.colore,"Colore: Blu Metallic");
	        configurazione.prezzo += 1050;
	    break;
	    case 3:
	        printf("[SERVER][GarageDucati] Colore scelto: Blu Metallizzato Ducati\n");
	        strcpy(configurazione.colore,"Colore: Blu Metallizzato Ducati");
	        configurazione.prezzo += 1000;
	    break;
	    case 4:
	        printf("[SERVER][GarageDucati] Colore scelto: Giallo Ducati\n");
	        strcpy(configurazione.colore,"Colore: Giallo Ducati");
	        configurazione.prezzo += 800;
	    break;
	    case 5:
	        printf("[SERVER][GarageDucati] Colore scelto: Grigio Steel Ducati\n");
	        strcpy(configurazione.colore,"Colore: Grigio Steel Ducati");
	        configurazione.prezzo += 850;
	    break;
	    case 6:
	        printf("[SERVER][GarageDucati] Colore scelto: Nero Metallic\n");
	        strcpy(configurazione.colore,"Colore: Nero Metallic");
	        configurazione.prezzo += 1150;
	    break;
	    case 7:
	        printf("[SERVER][GarageDucati] Colore scelto: Nero Sport\n");
	        strcpy(configurazione.colore,"Colore: Nero Sport");
	        configurazione.prezzo += 950;
	    break;
	    case 8:
	        printf("[SERVER][GarageDucati] Colore scelto: Rosso Ducati\n");
	        strcpy(configurazione.colore,"Colore: Rosso Ducati");
	        configurazione.prezzo += 0;
	    break;
	    case 9:
	        printf("[SERVER][GarageDucati] Colore scelto: Rosso Metallic\n");
	        strcpy(configurazione.colore,"Colore: Rosso Metallic");
	        configurazione.prezzo += 1100;
	    break;
	    case 10:
	        printf("[SERVER][GarageDucati] Colore scelto: Viola Metallic\n");
	        strcpy(configurazione.colore,"Colore: Viola Metallic");
	        configurazione.prezzo += 1200;
	    break;
    }
    switch(seriale.diametroCerchi){
	    case 1:
	        printf("[SERVER][GarageDucati] Diametro cerchi scelto: R17\n");
	        strcpy(configurazione.diametroCerchi,"Diametro cerchi: R17");
	        configurazione.prezzo += 0;
	    break;
	    case 2:
	        printf("[SERVER][GarageDucati] Diametro cerchi scelto: R18\n");
	        strcpy(configurazione.diametroCerchi,"Diametro cerchi: R18");
	        configurazione.prezzo += 700;
	    break;
	    case 3:
	        printf("[SERVER][GarageDucati] Diametro cerchi scelto: R19\n");
	        strcpy(configurazione.diametroCerchi,"Diametro cerchi: R19");
	        configurazione.prezzo += 800;
	    break;
	    case 4:
	        printf("[SERVER][GarageDucati] Diametro cerchi scelto: R20\n");
	        strcpy(configurazione.diametroCerchi,"Diametro cerchi: R20");
	        configurazione.prezzo += 900;
	    break;
	    case 5:
	        printf("[SERVER][GarageDucati] Diametro cerchi scelto: R21\n");
	        strcpy(configurazione.diametroCerchi,"Diametro cerchi: R21");
	        configurazione.prezzo += 1000;
	    break;
    }
    switch(seriale.sella){
	    case 1:
	        printf("[SERVER][GarageDucati] Sella scelta: Carbonio\n");
	        strcpy(configurazione.sella,"Sella: Carbonio");
	        configurazione.prezzo += 0;
	    break;
	    case 2:
	        printf("[SERVER][GarageDucati] Sella scelta: Ecopelle\n");
	        strcpy(configurazione.sella,"Sella: Ecopelle");
	        configurazione.prezzo += 250;
	    break;
	    case 3:
	        printf("[SERVER][GarageDucati] Sella scelta: Pelle\n");
	        strcpy(configurazione.sella,"Sella: Pelle");
	        configurazione.prezzo += 300;
	    break;
	    case 4:
	        printf("[SERVER][GarageDucati] Sella scelta: Sintetico\n");
	        strcpy(configurazione.sella,"Sella: Sintetico");
	        configurazione.prezzo += 0;
	    break;
	    case 5:
	        printf("[SERVER][GarageDucati] Sella scelta: Velluto\n");
	        strcpy(configurazione.sella,"Sella: Velluto");
	        configurazione.prezzo += 200;
	    break;
    }
    switch(seriale.scarico){
	    case 1:
	        printf("[SERVER][GarageDucati] Scarico scelto: Akrapovic\n");
	        strcpy(configurazione.scarico,"Scarico: Akrapovic");
	        configurazione.prezzo += 1200;
	    break;
	    case 2:
	        printf("[SERVER][GarageDucati] Scarico scelto: Arrow\n");
	        strcpy(configurazione.scarico,"Scarico: Arrow");
	        configurazione.prezzo += 750;
	    break;
	    case 3:
	        printf("[SERVER][GarageDucati] Scarico scelto: Giannelli\n");
	        strcpy(configurazione.scarico,"Scarico: Giannelli");
	        configurazione.prezzo += 900;
	    break;
	    case 4:
	        printf("[SERVER][GarageDucati] Scarico scelto: GPR\n");
	        strcpy(configurazione.scarico,"Scarico: GPR");
	        configurazione.prezzo += 1100;
	    break;
	    case 5:
	        printf("[SERVER][GarageDucati] Scarico scelto: LeoVince\n");
	        strcpy(configurazione.scarico,"Scarico: LeoVince");
	        configurazione.prezzo += 850;
	    break;
	    case 6:
	        printf("[SERVER][GarageDucati] Scarico scelto: Malossi\n");
	        strcpy(configurazione.scarico,"Scarico: Malossi");
	        configurazione.prezzo += 800;
	    break;
	    case 7:
	        printf("[SERVER][GarageDucati] Scarico scelto: Mivv\n");
	        strcpy(configurazione.scarico,"Scarico: Mivv");
	        configurazione.prezzo += 950;
	    break;
	    case 8:
	        printf("[SERVER][GarageDucati] Scarico scelto: Polini\n");
	        strcpy(configurazione.scarico,"Scarico: Polini");
	        configurazione.prezzo += 700;
	    break;
	    case 9:
	        printf("[SERVER][GarageDucati] Scarico scelto: Termignoni\n");
	        strcpy(configurazione.scarico,"Scarico: Termignoni");
	        configurazione.prezzo += 1000;
	    break;
	    case 10:
	        printf("[SERVER][GarageDucati] Scarico scelto: Yoshimura\n");
	        strcpy(configurazione.scarico,"Scarico: Yoshimura");
	        configurazione.prezzo += 0;
	    break;
    }
    switch(seriale.frizione){
	    case 1:
	        printf("[SERVER][GarageDucati] Frizione scelta: Antisaltellamento\n");
	        strcpy(configurazione.frizione,"Frizione: Antisaltellamento");
	        configurazione.prezzo += 0;
	    break;
	    case 2:
	        printf("[SERVER][GarageDucati] Frizione scelta: Automatica\n");
	        strcpy(configurazione.frizione,"Frizione: Automatica");
	        configurazione.prezzo += 600;
	    break;
	    case 3:
	        printf("[SERVER][GarageDucati] Frizione scelta: Disco singolo\n");
	        strcpy(configurazione.frizione,"Frizione: Disco singolo");
	        configurazione.prezzo += 350;
	    break;
	    case 4:
	        printf("[SERVER][GarageDucati] Frizione scelta: Idraulica\n");
	        strcpy(configurazione.frizione,"Frizione: Idraulica");
	        configurazione.prezzo += 700;
	    break;
	    case 5:
	        printf("[SERVER][GarageDucati] Frizione scelta: Multidisco\n");
	        strcpy(configurazione.frizione,"Frizione: Multidisco");
	        configurazione.prezzo += 450;
	    break;
    }
    switch(seriale.telaio){
	    case 1:
	        printf("[SERVER][GarageDucati] Telaio scelto: A traliccio\n");
	        strcpy(configurazione.telaio,"Telaio: A traliccio");
	        configurazione.prezzo += 1000;
	    break;
	    case 2:
	        printf("[SERVER][GarageDucati] Telaio scelto: A tubi saldati\n");
	        strcpy(configurazione.telaio,"Telaio: A tubi saldati");
	        configurazione.prezzo += 1250;
	    break;
	    case 3:
	        printf("[SERVER][GarageDucati] Telaio scelto: Misto\n");
	        strcpy(configurazione.telaio,"Telaio: Misto");
	        configurazione.prezzo += 1500;
	    break;
	    case 4:
	        printf("[SERVER][GarageDucati] Telaio scelto: Monoscocca\n");
	        strcpy(configurazione.telaio,"Telaio: Monoscocca");
	        configurazione.prezzo += 750;
	    break;
	    case 5:
	        printf("[SERVER][GarageDucati] Telaio scelto: Stampato\n");
	        strcpy(configurazione.telaio,"Telaio: Stampato");
	        configurazione.prezzo += 0;
	    break;
    }
    switch(seriale.pneumatici){
	    case 1:
	        printf("[SERVER][GarageDucati] Pneumatici scelti: Da competizione\n");
	        strcpy(configurazione.pneumatici,"Pneumatici: Da competizione");
	        configurazione.prezzo += 500;
	    break;
	    case 2:
	        printf("[SERVER][GarageDucati] Pneumatici scelti: Da fuoristrada\n");
	        strcpy(configurazione.pneumatici,"Pneumatici: Da fuoristrada");
	        configurazione.prezzo += 200;
	    break;
	    case 3:
	        printf("[SERVER][GarageDucati] Pneumatici scelti: Da strada\n");
	        strcpy(configurazione.pneumatici,"Pneumatici: Da strada");
	        configurazione.prezzo += 0;
	    break;
	    case 4:
	        printf("[SERVER][GarageDucati] Pneumatici scelti: Misti\n");
	        strcpy(configurazione.pneumatici,"Pneumatici: Misti");
	        configurazione.prezzo += 300;
	    break;
	    case 5:
	        printf("[SERVER][GarageDucati] Pneumatici scelti: Sportivi\n");
	        strcpy(configurazione.pneumatici,"Pneumatici: Sportivi");
	        configurazione.prezzo += 400;
	    break;
    }
    switch(seriale.pedane){
	    case 1:
	        printf("[SERVER][GarageDucati] Pedane scelte: Acciaio\n");
	        strcpy(configurazione.pedane,"Pedane: Acciaio");
	        configurazione.prezzo += 20;
	    break;
	    case 2:
	        printf("[SERVER][GarageDucati] Pedane scelte: Alluminio\n");
	        strcpy(configurazione.pedane,"Pedane: Alluminio");
	        configurazione.prezzo += 25;
	    break;
	    case 3:
	        printf("[SERVER][GarageDucati] Pedane scelte: Ergal\n");
	        strcpy(configurazione.pedane,"Pedane: Ergal");
	        configurazione.prezzo += 35;
	    break;
	    case 4:
	        printf("[SERVER][GarageDucati] Pedane scelte: Gomma\n");
	        strcpy(configurazione.pedane,"Pedane: Gomma");
	        configurazione.prezzo += 30;
	    break;
	    case 5:
	        printf("[SERVER][GarageDucati] Pedane scelte: Titanio\n");
	        strcpy(configurazione.pedane,"Pedane: Titanio");
	        configurazione.prezzo += 0;
	    break;
    }
    switch(seriale.cavalletto){
	    case 1:
	        printf("[SERVER][GarageDucati] Cavalletto scelto: Con forcellone monobraccio\n");
	        strcpy(configurazione.cavalletto,"Cavalletto: Con forcellone monobraccio");
	        configurazione.prezzo += 0;
	    break;
	    case 2:
	        printf("[SERVER][GarageDucati] Cavalletto scelto: Con forchette\n");
	        strcpy(configurazione.cavalletto,"Cavalletto: Con forchette");
	        configurazione.prezzo += 30;
	    break;
	    case 3:
	        printf("[SERVER][GarageDucati] Cavalletto scelto: Con perni orizzontali e verticali\n");
	        strcpy(configurazione.cavalletto,"Cavalletto: Con perni orizzontali e verticali");
	        configurazione.prezzo += 25;
	    break;
	    case 4:
	        printf("[SERVER][GarageDucati] Cavalletto scelto: Con piastre\n");
	        strcpy(configurazione.cavalletto,"Cavalletto: Con piastre");
	        configurazione.prezzo += 20;
	    break;
	    case 5:
	        printf("[SERVER][GarageDucati] Cavalletto scelto: Con ruote girevoli\n");
	        strcpy(configurazione.cavalletto,"Cavalletto: Con ruote girevoli");
	        configurazione.prezzo += 35;
	    break;
    }
    sleep(1);
    memcpy(memoriaCondivisa,&configurazione,sizeof(configurazione));	// Copiatura struttura su memoria condivisa
    pthread_exit(NULL);							// Uscita da thread
};
// Thread ---> Lettura da memoria condivisa e invio configurazione a client
void *stampaThread2(){
    dati configurazione;						// Struttura per scrittura scelte utente su file
    memcpy(&configurazione,memoriaCondivisa,sizeof(configurazione));	// Prelevamento struttura da memoria condivisa
    shmdt(memoriaCondivisa);						// Detach memoria condivisa
    send(IDNuovoSocket,&configurazione,sizeof(configurazione),0);	// Invio struttura a client
    printf("\n[SERVER][GarageDucati] Configurazione inviata con successo!\n");
    pthread_exit(NULL);							// Uscita da thread
};
