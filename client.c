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
#include <sys/ipc.h>		// Libreria per Inter Process Comunication
#include <semaphore.h>		// Libreria per semafori Posix
#include <netinet/in.h>		// Libreria per socket
#include <sys/socket.h>		// Libreria per socket
#include <arpa/inet.h>		// Libreria per socket
#include <netdb.h>		// Libreria per socket
#include <time.h>		// Libreria per data
#define PORTA 5000		// Porta comunicazione Client-Server

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

// Dichiarazione variabili globali
struct sockaddr_in indirizzoClient;			// Creazione struct indirizzo client
sem_t *semaforo;					// Semaforo Posix
int IDCoda;						// ID coda di messaggi
int pidMainThread;					// PID del main thread
int flag1;						// Flag 1
int flag2 = 0;						// Flag 2
int risultato,IDSocket;					// ID risultato per fork e ID socket
numeroSeriale seriale;					// Struttura con scelte utente per creazione configurazione
dati datiOttenuti;					// Struttura con dati ricevuti dal server per caricamento configurazione
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;	// Inizializzazione statica Mutex

// Dichiarazione prototipi funzioni
void *funzioneThread1();
void *funzioneThread2_1();
void *funzioneThread2_2();
void handlerExit(int segnale);
void handlerMenu(int segnale);
void handlerQuit(int segnale);
void handlerConnessione(int segnale);
void handlerSemaforo(int segnale);
void inizializzatoreIndirizzo(struct sockaddr_in *indirizzo,int porta);

// Sviluppo main thread
int main(){
    // Fase 1 ---> Creazione socket e inizializzazione connessione
    // Assegnazione handler ---> Segnale SIGQUIT per uscita programma, segnale SIGINT per uscita menu', segnale SIGALARM per controllo connessione e segnale SIGUSR1 per controllo semaforo
    signal(SIGQUIT,handlerQuit);
    signal(SIGINT,handlerExit);
    signal(SIGALRM,handlerConnessione);
    signal(SIGUSR1,handlerSemaforo);
    signal(SIGUSR2,handlerMenu);
    char nomeOperatore[20];				// Stringa nome operatore GarageMoto
    char uscita[] = "Chiudi\n";				// Stringa nome utente e comparazione per uscita coda
    int thread,sigmask,cambiaSigmask,memoriaCondivisa;	// ID thread, ID sigmask, ID per cambio sigmask, ID memoria condivisa
    int flagCoda = 0;					// Flag per coda messaggi
    int pidMainThread = getpid();			// Associazione getpid a pidMainThread
    int operatoreScelto;				// Numero operatore selezionato
    pthread_t threadScelte;				// Dichiarazione threadScelte per gestione scelte
    pid_t suddivisione;					// Creazione pid_t
    key_t chiave = ftok(".",'x');			// Generazione chiave con ftok
    struct sockaddr_in indirizzoServer;			// Creazione struttura indirizzo server
    int lunghezzaClient = sizeof(indirizzoClient);	// Associazione indirizzo client a lunghezza client
    // Fase 1.1 ---> Impostazione maschera
    sigset_t maschera;						// Riempimento campi vuoti maschera
    sigemptyset(&maschera);					// Inserimento segnale SIGQUIT nella maschera
    sigaddset(&maschera,SIGQUIT);
    sigmask = pthread_sigmask(SIG_SETMASK,&maschera,NULL);	// Impostazione maschera come SIGMASK thread
    // Fase 1.2 ---> Inizializzazzione indirizzo
    printf("[CLIENT] In esecuzione...\n");
    inizializzatoreIndirizzo(&indirizzoServer,PORTA);		// Inizializzazione indirizzo relativo con funzione
    IDSocket = socket(PF_INET,SOCK_STREAM,0);			// Inizializzazione socket ---> Socket di rete con connessione locale o TCP
    if(IDSocket == -1){                                         // Situazione errore creazione socket
        perror("\nErrore in fase di creazione del socket!\n");
        exit(EXIT_FAILURE);
    }
    printf("\n[SERVER][GarageDucati] In attesa della connessione del client con il server sulla porta: %d\nTerminazione client: 'Ctrl + C'\n\n",PORTA);
    // Fase 1.3 ---> Inizializzazione socket, thread, semaforo
    
    risultato = connect(IDSocket,(struct sockaddr *)&indirizzoServer,sizeof(indirizzoServer));	// Connessione
    alarm(1);											// Segnale SIGALRM richiama handler di controllo connessione
    getsockname(IDSocket,(struct sockaddr *)&indirizzoClient,(socklen_t*)&lunghezzaClient);	// Ottenimento porta locale client
    sleep(2);
    semaforo = sem_open("PRICEHOLD",O_CREAT|O_EXCL,0666,1);					// Apertura semaforo
    raise(SIGUSR1);										// Segnale SIGUSR1 richiama hanler che aspetta liberazione semaforo
    sleep(2);
    // Fase 2 ---> Inizio programma
    raise(SIGUSR2);										// Segnale SIGUSR2 richiama hanler che avvia il menu'
    send(IDSocket,&seriale.menu,sizeof(seriale),0);						// Invio scelta menu' a server
    // Fase 3 ---> Sdoppiamento opzioni, tra un thread e l'altro
    // Fase 3.1 ---> Creazione configurazione
    if(seriale.menu == 1){
        thread = pthread_create(&threadScelte,NULL,funzioneThread1,NULL);			// Generazione thread per configurazione moto
        if(thread == -1){									// Caso di fallimento creazione thread
            perror("\nErrore in fase di creazione del thread!\n");
            exit(EXIT_FAILURE);
        }
        pthread_join(threadScelte,NULL);
        printf("\nUltimazione della configurazione scelta...\n");
        sleep(3);
        suddivisione = fork();	// Generazione processo figlio per scelta effettuata
        switch(suddivisione){
            case -1:		// Caso fallimento fork
            	perror("\nErrore in fase di generazione del thread figlio tramite fork!\n");
                exit(EXIT_FAILURE);
            break;
            case 0:
                send(IDSocket,&seriale,sizeof(seriale),0);		// Invio seriale a server
                recv(IDSocket,&flag1,sizeof(flag1),0);			// Ricezione flag da server
                printf("Le scelte effettuate sono state salvate correttamente!\n\n");
                fflush(stdout);
                close(IDSocket);					// Chiusura socket
            default:
            	waitpid(suddivisione,NULL,WUNTRACED);
        }
        cambiaSigmask = pthread_sigmask(SIG_UNBLOCK,&maschera,NULL);	// Sblocco segnale SIGQUIT da maschera
        kill(pidMainThread,SIGQUIT);					// Invio segnale SIGQUIT e attivazione handler
    }
    // Fase 3.2 ---> Caricamento configurazione
    if(seriale.menu == 2){
        pthread_mutex_lock(&mutex);						// Blocco Mutex
        thread = pthread_create(&threadScelte,NULL,funzioneThread2_1,NULL);	// Creazione thread per ricezione seriale e salvataggio su struttura
        if(thread == -1){							// Caso fallimento creazione thread
            printf("\nErrore in fase di creazione del thread!\n");
            exit(EXIT_FAILURE);
        }
        pthread_join(threadScelte,NULL);					// Attesa terminazione thread
        send(IDSocket,&flag1,sizeof(flag1),0);					// Invio flag a server
        sleep(1);
        send(IDSocket,&seriale,sizeof(numeroSeriale),0);			// Invio seriale a server
        printf("Recupero della configurazione scelta...\n");
        thread = pthread_create(&threadScelte,NULL,funzioneThread2_2,NULL);	// Creazione thread per attesa risultato da server e stampa
        if(thread == -1){							// Caso fallimento creazione thread
            printf("\nErrore in fase di creazione del thread!\n");
            exit(EXIT_FAILURE);
        }
        pthread_join(threadScelte,NULL);			// Attesa terminazione thread
        pthread_mutex_unlock(&mutex);				// Sblocco Mutex
        puts("");
        close(IDSocket);					// Chiusura socket
        sleep(1);
        sigmask = pthread_sigmask(SIG_UNBLOCK,&maschera,NULL);	// Sblocco segnale SIGQUIT da maschera
        kill(pidMainThread,SIGQUIT);				// Invio segnale SIGQUIT e attivazione handler
    }
    // Fase 3.3 ---> Chat di assistenza
    if(seriale.menu == 3){
        printf("Questa e' la chat di assistenza!\n");
        fflush(stdout);
        sleep(1);
        char nomeUtente[100];
        getchar();
        printf("Inserire il proprio nome: ");
        scanf("%s",nomeUtente);
        send(IDSocket,&nomeUtente,sizeof(nomeUtente),0);	// Invio nome a server
        recv(IDSocket,&operatoreScelto,sizeof(operatoreScelto),0);
        recv(IDSocket,&nomeOperatore,sizeof(nomeOperatore),0);
        sleep(1);
        while(getchar() != '\n');
        // Fase 3.3.1 ---> Inizializzazione coda di messaggi
        if((IDCoda = msgget(chiave,IPC_CREAT|0666)) == -1){	// Creazione coda di messaggi con controllo errore
            perror("\nErrore in fase di creazione della coda di messaggi!\n");
            exit(EXIT_FAILURE);
        }
        messaggio.tipoMessaggio = 1;
        printf("\nDescrivere il proprio problema. Sara' inviato ad un operatore GarageDucati che rispondera' a breve. Per terminare la conversazione, digitare 'Chiudi'\n\n");	// Creazione iterazioni tra client-server
        printf("[%s]: ",nomeUtente);
        fgets(messaggio.testoMessaggio,sizeof(messaggio.testoMessaggio),stdin);
        msgsnd(IDCoda,&messaggio,sizeof(messaggio),0);				// Invio messaggio a server
        for(;;){
            msgrcv(IDCoda,&messaggio,sizeof(messaggio),1,0);			// Ricezione messaggio da server
            sleep(1);
            if((operatoreScelto >= 1 && operatoreScelto <= 5) || operatoreScelto == 7 || operatoreScelto == 12 || operatoreScelto == 13 || operatoreScelto == 16 || operatoreScelto == 20){
            	printf("[Operatore %s]: %s",nomeOperatore,messaggio.testoMessaggio);
            }
            else{
            	printf("[Operatrice %s]: %s",nomeOperatore,messaggio.testoMessaggio);
            }
            if(strcmp(messaggio.testoMessaggio,uscita) == 0){			// Controllo se messaggio coincide con stringa flag per terminare
                printf("\nFine chat!\n");
                msgctl(IDCoda,IPC_RMID,NULL);					// Chiusura coda di messaggi
                break;
            }
            else{
                printf("[%s]: ",nomeUtente);
 		        fgets(messaggio.testoMessaggio,sizeof(messaggio.testoMessaggio),stdin);
                msgsnd(IDCoda,&messaggio,sizeof(messaggio),0);			// Invio messaggio a server
            }
        }
        puts("");
        close(IDSocket);							// Chiusura socket
        sleep(1);
        sigmask = pthread_sigmask(SIG_UNBLOCK,&maschera,NULL);			// Sblocco segnale SIGQUIT da maschera
        kill(pidMainThread,SIGQUIT);						// Invio segnale SIGQUIT e attivazione handler
    }
    // Fase 3.4 ---> Chiusura programma
    if(seriale.menu == 4){
        close(IDSocket);							// Chiusura socket
        sleep(1);
        cambiaSigmask = pthread_sigmask(SIG_UNBLOCK,&maschera,NULL);		// Sblocco segnale SIGQUIT da maschera
        kill(pidMainThread,SIGQUIT);						// Invio segnale SIGQUIT e attivazione handler
    }
}

// Sviluppo funzioni
// Inizializzazione indirizzo
void inizializzatoreIndirizzo(struct sockaddr_in *indirizzo,int porta){
    indirizzo->sin_family = AF_INET;
    indirizzo->sin_port = htons(PORTA);
    inet_aton("127.0.0.1",&indirizzo->sin_addr);
}
// Handler segnale ---> Controllore e allarme connessione
void handlerConnessione(int segnale){
    if(risultato == -1){
        printf("Errore in fase di connessione al server! Il server non e' attivo!\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("[CLIENT] Connessione al server effettuata correttamente!\nPorta server: %d\nPorta client: %d",PORTA,ntohs(indirizzoClient.sin_port));
    }
}
// Handler segnale ---> Flag creazione semaforo Posix
void handlerSemaforo(int segnale){
    if(semaforo == SEM_FAILED){
    	puts("");
        printf("\nErrore! Attualmente il menu' non e' disponibile!\n");
        while(semaforo == SEM_FAILED){
            printf("\n[CLIENT] Attendere 10 secondi e riprovare\n");
            for(int i = 10;i >= 0;i--){
    	        if(i != 0){
            	    printf("%d ---> ",i);
        	    }
        	    else{
                    printf("%d",i);
                }
                fflush(stdout);
                sleep(1);
            }
            semaforo = sem_open("PRICEHOLD",O_CREAT|O_EXCL,0666,1);		// Creazione semaforo
        }
    }
    puts("");
}
// Handler segnale ---> Uscita da menu'
void handlerExit(int segnale){
    puts("");
    printf("\nGrazie e arrivederci.\n#################### GarageDucati ####################\n");
    if(sem_close(semaforo) == -1){
        perror("\nErrore in fase di chiusura del semaforo!\n");
        exit(EXIT_FAILURE);
    }
    if(sem_unlink("PRICEHOLD") == -1){
        perror("\nErrore in fase di disconnesione del semaforo!\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_mutex_destroy(&mutex) == -1){
        perror("\nErrore in fase di distruzione del Mutex!\n");
        exit(EXIT_FAILURE);
    }
    sleep(2);
    exit(EXIT_SUCCESS);
}
// Handler segnale ---> Chiusura client
void handlerQuit(int segnale){
    printf("Grazie e arrivederci.\n#################### GarageDucati ####################\n");
    sleep(3);
    puts("");
    printf("\n[CLIENT] Chiusura client...\n");
    sleep(3);
    close(IDSocket);
    if(sem_close(semaforo) == -1){
        perror("\nErrore in fase di chiusura del semaforo!\n");
        exit(EXIT_FAILURE);
    }
    if(sem_unlink("PRICEHOLD") == -1){
        perror("\nErrore in fase di disconnessione del semaforo!\n");
        exit(EXIT_FAILURE);
    }
    if(pthread_mutex_destroy(&mutex) == -1){
        perror("\nErrore in fase di distruzione del Mutex!\n");
        exit(EXIT_FAILURE);
    }
    sleep(2);
    printf("\n[SERVER][GarageDucati] Disconnessione effettuata con successo!\n");
    sleep(2);
    exit(EXIT_SUCCESS);
};
// Funzione gestione stampa e prima scelta menu'
void handlerMenu(int segnale){
    printf("\n[CLIENT] Avvio del menu' in corso...\n\n");
    sleep(3);
    printf("\n#################### GarageDucati ####################\nQuesta e' GarageDucati, l'applicazione che permette di configurare la propria moto!\n");
    sleep(3);
    fflush(stdout);
    printf("Opzioni menu':\n");
    fflush(stdout);
    printf("1 - Creazione della configurazione della propria moto\n2 - Caricamento della configurazione della propria moto\n3 - Contatto di un operatore in chat per assistenza\n4 - Chiusura del programma\n");
    fflush(stdout);
    printf("Selezionare dal menu' il numero in base alla propria esigenza: ");
    scanf("%hd",&seriale.menu);
    fflush(stdout);
    puts("");
    while(seriale.menu != 1 && seriale.menu != 2 && seriale.menu != 3 && seriale.menu != 4){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di scelta! Riprovare: ");
        scanf("%hd",&seriale.menu);
        puts("");
    }
    sleep(1);
    flag2 = 1;							// Modifica variabile globale per terminazione thread menu'
};
// Funzione per gestione scelta primo menu' (Creazione configurazione)
void *funzioneThread1(){
    printf("Modelli Ducati proposti:\n1 - Hypermotard 950 (937 cc, 114 CV) [€13290,00]\n2 - Monster (937 cc, 111 CV) [€11690,00]\n3 - Monster 1200 (1198 cc, 147 CV) [€14390,00]\n4 - Multistrada V2 (937 cc, 113 CV) [€14990,00]\n5 - Multistrada V4 (1158 cc, 170 CV) [€18990,00]\n6 - Panigale V2 (955 cc, 155 CV) [€18490,00]\n7 - Panigale V4 (1103 cc, 215 CV) [€24590,00]\n8 - Streetfighter V2 (955 cc, 153 CV) [€16990,00]\n9 - Streetfighter V4 (1103 cc, 208 CV) [€20690,00]\n10 - SuperSport 950 (937 cc, 110 CV) [€14290,00]\nSelezionare il modello della Ducati: ");
    scanf("%hd",&seriale.modello);
    puts("");
    while(seriale.modello < 1 || seriale.modello > 10){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di scelta! Riprovare: ");
        scanf("%hd",&seriale.modello);
        puts("");
    }
    sem_wait(semaforo);
    switch(seriale.modello){
        case 1:
            seriale.prezzo += 13290;
        break;
        case 2:
            seriale.prezzo += 11690;
        break;
        case 3:
            seriale.prezzo += 14390;
        break;
        case 4:
            seriale.prezzo += 14990;
        break;
        case 5:
            seriale.prezzo += 18990;
        break;
        case 6:
            seriale.prezzo += 18490;
        break;
        case 7:
            seriale.prezzo += 24590;
        break;
        case 8:
            seriale.prezzo += 16990;
        break;
        case 9:
            seriale.prezzo += 20690;
        break;
        case 10:
            seriale.prezzo += 14290;
        break;
    }
    sem_post(semaforo);
    sleep(1);
    printf("Colori verniciatura Ducati proposti:\n1 - Bianco Corsa [€750,00]\n2 - Blu Metallic [€1050,00]\n3 - Blu Metallizzato Ducati [€1000,00]\n4 - Giallo Ducati [€800,00]\n5 - Grigio Steel Ducati [€850,00]\n6 - Nero Metallic [€1150,00]\n7 - Nero Sport [€950,00]\n8 - Rosso Ducati [€0,00 - Incluso nel set base]\n9 - Rosso Metallic [€1100,00]\n10 - Viola Metallic [€1200,00]\nSelezionare il colore della verniciatura della Ducati: ");
    scanf("%hd",&seriale.colore);
    puts("");
    while(seriale.colore < 1 || seriale.colore > 10){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di scelta! Riprovare: ");
        scanf("%hd",&seriale.colore);
        puts("");
    }
    sem_wait(semaforo);
    switch(seriale.colore){
        case 1:
            seriale.prezzo += 750;
        break;
        case 2:
            seriale.prezzo += 1050;
        break;
        case 3:
            seriale.prezzo += 1000;
        break;
        case 4:
            seriale.prezzo += 800;
        break;
        case 5:
            seriale.prezzo += 850;
        break;
        case 6:
            seriale.prezzo += 1150;
        break;
        case 7:
            seriale.prezzo += 950;
        break;
        case 8:
            seriale.prezzo += 0;
        break;
        case 9:
            seriale.prezzo += 1100;
        break;
        case 10:
            seriale.prezzo += 1200;
        break;
    }
    sem_post(semaforo);
    sleep(1);
    printf("Diametri cerchi Ducati proposti:\n1 - R17 (Standard) [€0,00 - Incluso nel set base]\n2 - R18 (Cross) [€700,00]\n3 - R19 (Motard) [€800,00]\n4 - R20 (Corsa) [€900,00]\n5 - R21 (Sport) [€1000,00]\nSelezionare il diametro dei cerchi della Ducati: ");
    scanf("%hd",&seriale.diametroCerchi);
    puts("");
    while(seriale.diametroCerchi < 1 || seriale.diametroCerchi > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di scelta! Riprovare: ");
        scanf("%hd",&seriale.diametroCerchi);
        puts("");
    }
    sem_wait(semaforo);
    switch(seriale.diametroCerchi){
        case 1:
            seriale.prezzo += 0;
        break;
        case 2:
            seriale.prezzo += 700;
        break;
        case 3:
            seriale.prezzo += 800;
        break;
        case 4:
            seriale.prezzo += 900;
        break;
        case 5:
            seriale.prezzo += 1000;
        break;
    }
    sem_post(semaforo);
    sleep(1);
    printf("Materiali sella Ducati proposti:\n1 - Carbonio [€150,00]\n2 - Ecopelle [€250,00]\n3 - Pelle [€300,00]\n4 - Sintetico [€0,00 - Incluso nel set base]\n5 - Velluto [€200,00]\nSelezionare il materiale della sella della Ducati: ");
    scanf("%hd",&seriale.sella);
    puts("");
    while(seriale.sella < 1 || seriale.sella > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di scelta! Riprovare: ");
        scanf("%hd",&seriale.sella);
        puts("");
    }
    sem_wait(semaforo);
    switch(seriale.sella){
        case 1:
            seriale.prezzo += 150;
        break;
        case 2:
            seriale.prezzo += 250;
        break;
        case 3:
            seriale.prezzo += 300;
        break;
        case 4:
            seriale.prezzo += 0;
        break;
        case 5:
            seriale.prezzo += 200;
        break;
    }
    sem_post(semaforo);
    sleep(1);
    printf("Tipi scarico Ducati proposti:\n1 - Akrapovic [€1200,00]\n2 - Arrow [€750,00]\n3 - Giannelli [€900,00]\n4 - GPR [€1100,00]\n5 - LeoVince [€850,00]\n6 - Malossi [€800,00]\n7 - Mivv [€950,00]\n8 - Polini [€700,00]\n9 - Termignoni [€1000,00]\n10 - Yoshimura [€0,00 - Incluso nel set base]\nSelezionare il tipo di scarico della Ducati: ");
    scanf("%hd",&seriale.scarico);
    puts("");
    while(seriale.scarico < 1 || seriale.scarico > 10){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di scelta! Riprovare: ");
        scanf("%hd",&seriale.scarico);
        puts("");
    }
    sem_wait(semaforo);
    switch(seriale.scarico){
        case 1:
            seriale.prezzo += 1200;
        break;
        case 2:
            seriale.prezzo += 750;
        break;
        case 3:
            seriale.prezzo += 900;
        break;
        case 4:
            seriale.prezzo += 1100;
        break;
        case 5:
            seriale.prezzo += 850;
        break;
        case 6:
            seriale.prezzo += 800;
        break;
        case 7:
            seriale.prezzo += 950;
        break;
        case 8:
            seriale.prezzo += 700;
        break;
        case 9:
            seriale.prezzo += 1000;
        break;
        case 10:
            seriale.prezzo += 0;
        break;
    }
    sem_post(semaforo);
    sleep(1);
    printf("Tipi frizione Ducati proposti:\n1 - Antisaltellamento [€0,00 - Inclusa nel set base]\n2 - Automatica [€600,00]\n3 - Disco singolo [€350,00]\n4 - Idraulica [€700,00]\n5 - Multidisco [€450,00]\nSelezionare il tipo di frizione della Ducati: ");
    scanf("%hd",&seriale.frizione);
    puts("");
    while(seriale.frizione < 1 || seriale.frizione > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di scelta! Riprovare: ");
        scanf("%hd",&seriale.frizione);
        puts("");
    }
    sem_wait(semaforo);
    switch(seriale.frizione){
        case 1:
            seriale.prezzo += 0;
        break;
        case 2:
            seriale.prezzo += 600;
        break;
        case 3:
            seriale.prezzo += 350;
        break;
        case 4:
            seriale.prezzo += 700;
        break;
        case 5:
            seriale.prezzo += 450;
        break;
    }
    sem_post(semaforo);
    sleep(1);
    printf("Tipi telaio Ducati proposti:\n1 - A traliccio [€1000,00]\n2 - A tubi saldati [€1250,00]\n3 - Misto (In parte a tubi saldati ed in parte stampato) [€1500,00]\n4 - Monoscocca [€750,00]\n5 - Stampato [€0,00 - Incluso nel set base]\nSelezionare il tipo di telaio della Ducati: ");
    scanf("%hd",&seriale.telaio);
    puts("");
    while(seriale.telaio < 1 || seriale.telaio > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di scelta! Riprovare: ");
        scanf("%hd",&seriale.telaio);
        puts("");
    }
    sem_wait(semaforo);
    switch(seriale.telaio){
    	case 1:
            seriale.prezzo += 1000;
        break;
        case 2:
            seriale.prezzo += 1250;
        break;
        case 3:
            seriale.prezzo += 1500;
        break;
        case 4:
            seriale.prezzo += 750;
        break;
        case 5:
            seriale.prezzo += 0;
        break;
    }
    sem_post(semaforo);
    sleep(1);
    printf("Tipi pneumatici Ducati proposti:\n1 - Da competizione [€500,00]\n2 - Da fuoristrada [€200,00]\n3 - Da strada [€0,00 - Inclusi nel set base]\n4 - Misti (Da strada e da fuoristrada) [€300,00]\n5 - Sportivi [€400,00]\nSelezionare il tipo di pneumatici della Ducati: ");
    scanf("%hd",&seriale.pneumatici);
    puts("");
    while(seriale.pneumatici < 1 || seriale.pneumatici > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di scelta! Riprovare: ");
        scanf("%hd",&seriale.pneumatici);
        puts("");
    }
    sem_wait(semaforo);
    switch(seriale.pneumatici){
        case 1:
            seriale.prezzo += 500;
        break;
        case 2:
            seriale.prezzo += 200;
        break;
        case 3:
            seriale.prezzo += 0;
        break;
        case 4:
            seriale.prezzo += 300;
        break;
        case 5:
            seriale.prezzo += 400;
        break;
    }
    sem_post(semaforo);
    sleep(1);
    printf("Tipi pedane Ducati proposti:\n1 - Acciaio [€20,00]\n2 - Alluminio [€25,00]\n3 - Ergal [€35,00]\n4 - Gomma [€30,00]\n5 - Titanio [€0,00 - Incluse nel set base]\nSelezionare il tipo di pedane della Ducati: ");
    scanf("%hd",&seriale.pedane);
    puts("");
    while(seriale.pedane < 1 || seriale.pedane > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di scelta! Riprovare: ");
        scanf("%hd",&seriale.pedane);
        puts("");
    }
    sem_wait(semaforo);
    switch(seriale.pedane){
        case 1:
            seriale.prezzo += 20;
        break;
        case 2:
            seriale.prezzo += 25;
        break;
        case 3:
            seriale.prezzo += 35;
        break;
        case 4:
            seriale.prezzo += 30;
        break;
        case 5:
            seriale.prezzo += 0;
        break;
    }
    sem_post(semaforo);
    sleep(1);
    printf("Tipi cavalletto Ducati proposti:\n1 - Con forcellone monobraccio [€0,00 - Incluso nel set base]\n2 - Con forchette [€30,00]\n3 - Con perni orizzontali e verticali [€25,00]\n4 - Con piastre [€20,00]\n5 - Con ruote girevoli [€35,00]\nSelezionare il tipo di cavalletto della Ducati: ");
    scanf("%hd",&seriale.cavalletto);
    puts("");
    while(seriale.cavalletto < 1 || seriale.cavalletto > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di scelta! Riprovare: ");
        scanf("%hd",&seriale.cavalletto);
        puts("");
    }
    sem_wait(semaforo);
    switch(seriale.cavalletto){
        case 1:
            seriale.prezzo += 0;
        break;
        case 2:
            seriale.prezzo += 30;
        break;
        case 3:
            seriale.prezzo += 25;
        break;
        case 4:
            seriale.prezzo += 20;
        break;
        case 5:
            seriale.prezzo += 35;
        break;
    }
    sem_post(semaforo);
    getchar();
    sleep(1);
    printf("Inserire il nome della propria configurazione. Se il nome inserito e' quello di un file gia' esistente, quest'ultimo verra' sovrascritto. Il nome deve essere di massimo 100 caratteri: ");
    sem_wait(semaforo);						// Blocco semaforo
    fgets(seriale.nomeConfigurazione,sizeof(seriale.nomeConfigurazione),stdin);
    int i = strlen(seriale.nomeConfigurazione);
    seriale.nomeConfigurazione[i-1] = '.';			// Sostituzione carattere terminatore con "."
    strcat(seriale.nomeConfigurazione,"txt");			// Aggiunta a nome di estensione "txt" per creazione file "nome.txt"
    seriale.grandezza = strlen(seriale.nomeConfigurazione);	// Salvataggio lunghezza nome con annessa estensione ed inserimento in struttura da inviare a server
    sem_post(semaforo);						// Sblocco semaforo
    sleep(1);
    pthread_exit(NULL);						// Terminazione thread
}
// Funzione per gestione scelta secondo menu' - Prima parte (Scrittura seriale)
void *funzioneThread2_1(){
    sleep(1);
    printf("Inserire il 1° numero del codice seriale, quello riguardante il modello della propria Ducati: ");
    scanf("%hd",&seriale.modello);
    puts("");
    while(seriale.modello < 1 || seriale.modello > 10){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di inserimento del 1° numero! Riprovare: ");
        scanf("%hd",&seriale.modello);
        puts("");
    }
    sleep(1);
    printf("Inserire il 2° numero del codice seriale, quello riguardante il colore della propria Ducati: ");
    scanf("%hd",&seriale.colore);
    puts("");
    while(seriale.colore < 1 || seriale.colore > 10){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di inserimento del 2° numero! Riprovare: ");
        scanf("%hd",&seriale.colore);
        puts("");
    }
    sleep(1);
    printf("Inserire il 3° numero del codice seriale, quello riguardante il diametro dei cerchi della propria Ducati: ");
    scanf("%hd",&seriale.diametroCerchi);
    puts("");
    while(seriale.diametroCerchi < 1 || seriale.diametroCerchi > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di inserimento del 3° numero! Riprovare: ");
        scanf("%hd",&seriale.diametroCerchi);
        puts("");
    }
    sleep(1);
    printf("Inserire il 4° numero del codice seriale, quello riguardante il materiale della sella della propria Ducati: ");
    scanf("%hd",&seriale.sella);
    puts("");
    while(seriale.sella < 1 || seriale.sella > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di inserimento del 4° numero! Riprovare: ");
        scanf("%hd",&seriale.sella);
        puts("");
    }
    sleep(1);
    printf("Inserire il 5° numero del codice seriale, quello riguardante lo scarico della propria Ducati: ");
    scanf("%hd",&seriale.scarico);
    puts("");
    while(seriale.scarico < 1 || seriale.scarico > 10){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di inserimento del 5° numero! Riprovare: ");
        scanf("%hd",&seriale.scarico);
        puts("");
    }
    sleep(1);
    printf("Inserire il 6° numero del codice seriale, quello riguardante la frizione della propria Ducati: ");
    scanf("%hd",&seriale.frizione);
    puts("");
    while(seriale.frizione < 1 || seriale.frizione > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di inserimento del 6° numero! Riprovare: ");
        scanf("%hd",&seriale.frizione);
        puts("");
    }
    sleep(1);
    printf("Inserire il 7° numero del codice seriale, quello riguardante il telaio della propria Ducati: ");
    scanf("%hd",&seriale.telaio);
    puts("");
    while(seriale.telaio < 1 || seriale.telaio > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di inserimento del 7° numero! Riprovare: ");
        scanf("%hd",&seriale.telaio);
        puts("");
    }
    sleep(1);
    printf("Inserire il 8° numero del codice seriale, quello riguardante gli pneumatici della propria Ducati: ");
    scanf("%hd",&seriale.pneumatici);
    puts("");
    while(seriale.pneumatici < 1 || seriale.pneumatici > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di inserimento del 8° numero! Riprovare: ");
        scanf("%hd",&seriale.pneumatici);
        puts("");
    }
    sleep(1);
    printf("Inserire il 9° numero del codice seriale, quello riguardante le pedane della propria Ducati: ");
    scanf("%hd",&seriale.pedane);
    puts("");
    while(seriale.pedane < 1 || seriale.pedane > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di inserimento del 9° numero! Riprovare: ");
        scanf("%hd",&seriale.pedane);
        puts("");
    }
    sleep(1);
    printf("Inserire il 10° numero del codice seriale, quello riguardante il cavalletto della propria Ducati: ");
    scanf("%hd",&seriale.cavalletto);
    puts("");
    while(seriale.cavalletto < 1 || seriale.cavalletto > 5){
        sleep(1);
        while(getchar() != '\n');
        printf("Errore in fase di inserimento del 10° numero! Riprovare: ");
        scanf("%hd",&seriale.cavalletto);
        puts("");
    }
    pthread_exit(NULL);						// Terminazione thread
}
// Funzione per gestione scelta secondo menu' - Seconda parte (Lettura della configurazione)
void *funzioneThread2_2(){
    recv(IDSocket,&datiOttenuti,sizeof(datiOttenuti),0);	// Ricezione di configurazione e stampa
    sleep(3);
    printf("Ecco la configurazione corrispondente al seriale inserito:\n\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n\nPrezzo totale IVA inclusa: € %d,00\n",datiOttenuti.modello,datiOttenuti.colore,datiOttenuti.diametroCerchi,datiOttenuti.sella,datiOttenuti.scarico,datiOttenuti.frizione,datiOttenuti.telaio,datiOttenuti.pneumatici,datiOttenuti.pedane,datiOttenuti.cavalletto,datiOttenuti.prezzo);
    fflush(stdout);
    sleep(1);
    pthread_exit(NULL);						// Terminazione thread
};
