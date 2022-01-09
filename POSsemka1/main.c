#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>


typedef struct {
    pthread_mutex_t * mut;
    pthread_cond_t * vloz;
    pthread_cond_t * odober;
} nic;

typedef struct {
    int cisloSocketu;
} Chat;

typedef enum { false, true } bool;
bool prihalseny = false;

char buffer[256];
char username[256];
char password[256];
char sprava[256];
char help[256];
char vstupMenu[256];
int sockfd, n;
char koniecChat[256];
pthread_t vlakno, vlaknoChat;
FILE * file;


//prida retazec pole1 do buffra od pozicie
void pridajZnaky(int pozicia, char pole1[]){
    pozicia = pozicia;
    for (int i = pozicia; i < pozicia + strlen(pole1) + 1; ++i) {
        buffer[i] = pole1[i - pozicia];
    }
}

//retazec ktory pride ako parameter zaobaly do zatvoriek [] cize [retazec]
void zaobalDoZatvoriek(char pole[]){
    char vysledok[strlen(pole) + 2];
    vysledok[0] = '[';
    for (int i = 0; i < strlen(pole); ++i) {
        vysledok[i+1] = pole[i];
    }
    vysledok[strlen(pole) + 1] = ']';
    strncpy(pole, vysledok, strlen(pole) + 2);
}

//vytiahne slovo z retazca zapisanom v tvare slovo1;slovo2;slovo3;...
void najdiSlovoRiadok(){
    bzero(help,256);
    int zaciatok = strcspn(buffer, ";");
    buffer[strcspn(buffer, ";")] = " ";
    int koniec = strcspn(buffer, ";");
    for (int i = zaciatok; i < koniec - 1; ++i) {
        help[i - zaciatok] = buffer[i+1];
    }
}

void * citajChat(void * data) {

    while (1) {
        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);
        printf("\n%s: %s\n", sprava, buffer);

        if((strcmp(buffer, "END") == 0)){
            strcpy(koniecChat, buffer);
            break;
        }
    }
    return NULL;
}

void zacniChat(){
    bzero(koniecChat, 256);
    Chat chat;
    chat.cisloSocketu = sockfd;
    pthread_create(&vlaknoChat, NULL, &citajChat, (void *)&chat);
    while(1){

        printf("Vy: ");
        bzero(buffer, 256);
        fgets(buffer, 255, stdin);
        n = write(sockfd, buffer, strlen(buffer));

        if((strcmp(koniecChat, "END") == 0)){
            pthread_cancel(&vlaknoChat);
            printf("Ukoncili ste chat s pouzivatelom %s", sprava);
            fcntl(sockfd, F_SETFL, O_NONBLOCK);
            break;
        }
    }
}

/*void vytvorFriendList(){
    friendlist = fopen("friendlist.txt", "w+");
    fclose(friendlist);
}

void pridajFrienda(char friendName[]){
    friendlist = fopen("friendlist.txt", "a+");
    fprintf(friendlist, "%s\n",friendName);
    fclose(friendlist);
}

void vypisFriendov(){
    if (friendlist == NULL) {
        printf("You dont have any friends\n");
        fflush(stdout);
        exit(-1);
    } else{
        friendlist = fopen("friendlist.txt", "r+");
        for (int i = 0; i < strlen(friendlist); ++i) {
            bzero(friendName,256);
            fscanf(friendlist,"%s", &friendName);
            printf("%s\n",friendName);
            fflush(stdout);
        }
        fclose(friendlist);
    }
}*/
void posliFile(FILE * subor, int cisloSocketu){
    char data[1024] = {0};

    while(fgets(data, 1024, subor) != NULL) {
        if (send(cisloSocketu, data, sizeof(data), 0) == -1) {
            perror("Posielanie suboru zlyhalo");
            break;
        }
        bzero(data, 1024);
    }
    strcpy(buffer, "END");
    send(cisloSocketu, buffer, sizeof(buffer), 0);
    printf("Subor bol uspesne odoslany\n");
}

//funkcia na prijatie suboru, zapise prichodzie data do suboru
void prijmiSubor(int cisloSocketu){
    fcntl(sockfd, F_SETFL, MCAST_BLOCK_SOURCE);
    int n;
    FILE * fp;
    char buffer[1024];
    fp = fopen("darcek.txt", "w+");
    while (1) {
        n = read(cisloSocketu, buffer, 1024);
        if (strcmp(buffer, "END") == 0){
            break;
        }
        fprintf(fp, "%s", buffer);
        bzero(buffer, 1024);
    }
    fclose(fp);
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    return;
}

void menu(void * data){
    nic * dataNIC = (nic*)data;
    if (prihalseny == true){
        printf("Ste prihlaseny ako pouzivatel %s, co chcete robit ?\n", username);
        printf("1. zacat chat\n");
        printf("2. zacat groupchat\n");
        printf("3. poslat subor\n");
        printf("4. pridat priatela\n");
        printf("5. ostranit priatela\n");
        printf("6. zobrazit zoznam priatelov\n");
        printf("7. vymazat ucet\n");
        printf("8. odhlasit sa\n");
        fflush(stdout);

        bzero(vstupMenu, 256);
        fgets(vstupMenu, 255, stdin);

        pthread_mutex_lock(dataNIC->mut);

        int odpoved = atoi(vstupMenu);
        switch (odpoved) {
            case 1:
                fcntl(sockfd, F_SETFL, MCAST_BLOCK_SOURCE);
                bzero(buffer, 256);
                bzero(sprava,256);
                strcpy(buffer, username);
                pridajZnaky(strlen(buffer), "[StartChat]");
                printf("Zadajte meno pouzivatela s ktorym si chcete zacat pisat\n ");
                printf("Username: ");
                fgets(sprava, 255, stdin);
                sprava[strcspn(sprava, "\n")] = 0;
                zaobalDoZatvoriek(sprava);
                pridajZnaky(strlen(buffer), sprava);
                n = write(sockfd, buffer, strlen(buffer));

                bzero(buffer, 256);
                n = read(sockfd, buffer, 255);
                if(strcmp(buffer, "OK") == 0) {
                    printf("Zacali ste chat s pouzivatelom %s\n", sprava);
                    fflush(stdout);
                    zacniChat();
                } else {
                    printf("Zadaneho pouzivatela nemate v priateloch(alebo je off) alebo odmietol pozvanku na chat\n");
                    fflush(stdout);
                }
                fcntl(sockfd, F_SETFL, O_NONBLOCK);
                break;
            case 2:

                break;
            case 3:
                fcntl(sockfd, F_SETFL, MCAST_BLOCK_SOURCE);
                bzero(buffer, 256);
                bzero(sprava,256);
                strcpy(buffer, username);
                pridajZnaky(strlen(buffer), "[SendFile]");
                printf("Zadajte meno pouzivatela ktoremu chcete poslat subor\n ");
                printf("Username: ");
                fflush(stdout);
                fgets(sprava, 255, stdin);
                sprava[strcspn(sprava, "\n")] = 0;
                zaobalDoZatvoriek(sprava);
                pridajZnaky(strlen(buffer), sprava);
                n = write(sockfd, buffer, strlen(buffer));

                printf("Zadajte nazov suboru ktory chce odoslat:");
                bzero(buffer, 256);
                fgets(buffer, 255, stdin);
                buffer[strcspn(buffer, "\n")] = 0;
                file = fopen(buffer, "r");
                if (file == NULL){
                    printf("Zadany subor neexistuje");
                    fflush(stdout);
                    fcntl(sockfd, F_SETFL, O_NONBLOCK);
                    return;
                }
                posliFile(file, sockfd);
                close(file);
                fcntl(sockfd, F_SETFL, O_NONBLOCK);
                break;
            case 4:
                fcntl(sockfd, F_SETFL, MCAST_BLOCK_SOURCE);
                bzero(buffer, 256);
                bzero(sprava,256);
                strcpy(buffer, username);
                pridajZnaky(strlen(buffer), "[AddFriend]");
                printf("Zadajte meno pouzivatela ktoreho si chcete pridat\n ");
                printf("Username: ");
                fflush(stdout);
                fgets(sprava, 255, stdin);
                sprava[strcspn(sprava, "\n")] = 0;
                zaobalDoZatvoriek(sprava);
                pridajZnaky(strlen(buffer), sprava);
                n = write(sockfd, buffer, strlen(buffer));

                bzero(buffer, 256);
                n = read(sockfd, buffer, 255);
                if(strcmp(buffer, "OK") == 0) {
                    printf("Uspesne ste si pridali priatela %s\n", sprava);
                    fflush(stdout);
                } else {
                    printf("Dany pouzivatel je offline alebo odmietol vasu ziadost, alebo sa uz nachadza medzi vasimi priatelmi\n");
                    fflush(stdout);
                }
                fcntl(sockfd, F_SETFL, O_NONBLOCK);
                break;
            case 5:
                fcntl(sockfd, F_SETFL, MCAST_BLOCK_SOURCE);
                bzero(buffer, 256);
                bzero(sprava,256);
                strcpy(buffer, username);
                pridajZnaky(strlen(buffer), "[DeleteFriend]");
                printf("Zadajte meno pouzivatela ktoreho si chete odstranit\n ");
                printf("Username: ");
                fgets(sprava, 255, stdin);
                sprava[strcspn(sprava, "\n")] = 0;
                zaobalDoZatvoriek(sprava);
                pridajZnaky(strlen(buffer), sprava);
                n = write(sockfd, buffer, strlen(buffer));

                bzero(buffer, 256);
                n = read(sockfd, buffer, 255);
                if(strcmp(buffer, "OK") == 0) {
                    printf("Uspesne ste si odstranili priatela %s\n", sprava);
                    fflush(stdout);
                } else {
                    printf("Takyto pouzivatel nie je medzi vasimi priatelmi\n");
                    fflush(stdout);
                }
                fcntl(sockfd, F_SETFL, O_NONBLOCK);
                break;
            case 6:
                fcntl(sockfd, F_SETFL, MCAST_BLOCK_SOURCE);
                bzero(buffer, 256);
                bzero(sprava,256);
                strcpy(buffer, username);
                pridajZnaky(strlen(buffer), "[FriendList]");
                pridajZnaky(strlen(buffer), "[]");
                n = write(sockfd, buffer, strlen(buffer));

                bzero(buffer, 256);
                n = read(sockfd, buffer, 255);
                printf("Tvoji priatelia:\n");
                for (int i = 0; i < 10; ++i) {
                    najdiSlovoRiadok();
                    printf("%s\n",help);
                    fflush(stdout);
                    if ((strcmp(help, "") == 0)){
                        break;
                    }
                }
                fcntl(sockfd, F_SETFL, O_NONBLOCK);
                break;
            case 7:
                fcntl(sockfd, F_SETFL, MCAST_BLOCK_SOURCE);
                bzero(buffer, 256);
                bzero(password,256);
                strcpy(buffer, username);
                pridajZnaky(strlen(buffer), "[DeleteAcc]");
                printf("Ak naozaj chcete vymazat tento ucet zadajte heslo\n");
                printf("Password: ");
                fflush(stdout);
                fgets(password, 255, stdin);
                password[strcspn(password, "\n")] = 0;
                zaobalDoZatvoriek(password);
                pridajZnaky(strlen(buffer), password);
                n = write(sockfd, buffer, strlen(buffer));

                bzero(buffer, 256);
                n = read(sockfd, buffer, 255);
                if(strcmp(buffer, "OK") == 0) {
                    printf("Uspesne ste si vymazali ucet\n");
                    fflush(stdout);
                    prihalseny = false;
                } else {
                    printf("Zadali ste zle heslo\n");
                    fflush(stdout);
                }
                fcntl(sockfd, F_SETFL, O_NONBLOCK);
                break;
            case 8:
                prihalseny = false;
                break;
        }
    } else {
        printf("Vitajte v nasej chat aplikacii, ako chcete dalej pokracovat ?\n");
        printf("1. prihlasit sa\n");
        printf("2. zaregistrovat sa\n");
        printf("3. exit\n");
        fflush(stdout);

        bzero(vstupMenu, 256);
        fgets(vstupMenu, 255, stdin);

        pthread_mutex_lock(dataNIC->mut);

        int odpoved = atoi(vstupMenu);
        switch (odpoved) {
            case 1:
                fcntl(sockfd, F_SETFL, MCAST_BLOCK_SOURCE);
                printf("Username: ");
                fflush(stdout);
                bzero(buffer, 256);
                bzero(sprava, 256);
                bzero(password, 256);
                bzero(username, 256);
                fgets(username, 255, stdin);
                username[strcspn(username, "\n")] = 0;
                zaobalDoZatvoriek(username);
                strcpy(buffer, username);
                pridajZnaky(strlen(buffer), "[Login]");
                printf("Password: ");
                fflush(stdout);
                fgets(password, 255, stdin);
                password[strcspn(password, "\n")] = 0;
                zaobalDoZatvoriek(password);
                pridajZnaky(strlen(buffer), password);
                n = write(sockfd, buffer, strlen(buffer));

                bzero(buffer, 256);
                n = read(sockfd, buffer, 255);
                if(strcmp(buffer, "OK") == 0) {
                    prihalseny = true;
                    printf("Uspesne ste sa prihlasili\n");
                    fflush(stdout);
                } else {
                    printf("Zadalie ste nespravne heslo alebo meno\n");
                    fflush(stdout);
                }
                fcntl(sockfd, F_SETFL, O_NONBLOCK);
                break;
            case 2:
                fcntl(sockfd, F_SETFL, MCAST_BLOCK_SOURCE);
                printf("Username: ");
                fflush(stdout);
                bzero(buffer, 256);
                bzero(sprava, 256);
                bzero(password, 256);
                bzero(username, 256);
                fgets(username, 255, stdin);
                username[strcspn(username, "\n")] = 0;
                zaobalDoZatvoriek(username);
                strcpy(buffer, username);
                pridajZnaky(strlen(buffer), "[Create]");
                printf("Password: ");
                fflush(stdout);
                fgets(password, 255, stdin);
                password[strcspn(password, "\n")] = 0;
                zaobalDoZatvoriek(password);
                pridajZnaky(strlen(buffer), password);
                n = write(sockfd, buffer, strlen(buffer));

                bzero(buffer, 256);
                n = read(sockfd, buffer, 255);
                if(strcmp(buffer, "OK") == 0) {
                    prihalseny = true;
                    printf("Uspesne ste sa zaregistrovali\n");
                    fflush(stdout);
                } else {
                    printf("Taketo meno uz existuje\n");
                    fflush(stdout);
                }
                fcntl(sockfd, F_SETFL, O_NONBLOCK);
                break;
            case 3:
                exit(1);
                break;
        }
    }
    pthread_mutex_unlock(dataNIC->mut);
}

/*void hocico(){
    struct sockaddr_in serv_addr2;
    struct hostent *server2;

    server2 = gethostbyname("localhost");
    if (server2 == NULL) {
        fprintf(stderr, "Error, no such host\n");
    }

    bzero((char *) &serv_addr2, sizeof(serv_addr2));
    serv_addr2.sin_family = AF_INET;
    bcopy(
            (char *) server2->h_addr,
            (char *) &serv_addr2.sin_addr.s_addr,
            server2->h_length
    );
    serv_addr2.sin_port = htons(5022);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
    }

    if (connect(sockfd, (struct sockaddr *) &serv_addr2, sizeof(serv_addr2)) < 0) {
        perror("Error connecting to socket");
    }
}*/

void * zberVstupu(void * data) {
    while (1) {
        menu(data);

        int i = strcmp("Bye\n", buffer);
        if (i == 0) {
            break;
        }
    }
    return NULL;
}




int main(int argc, char *argv[]) {
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error, no such host\n");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy(
            (char *) server->h_addr,
            (char *) &serv_addr.sin_addr.s_addr,
            server->h_length
    );
    serv_addr.sin_port = htons(atoi(argv[2]));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
    }

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        //perror("Error connecting to socket");
    }

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t vloz,odober = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_init(&vloz, NULL);
    pthread_cond_init(&odober, NULL);

    nic nic;
    nic.mut = &mutex;
    nic.vloz = &vloz;
    nic.odober = &odober;

    pthread_create(&vlakno, NULL, &zberVstupu, (void *) &nic);

    while (1) {
        pthread_mutex_lock(&mutex);
        bzero(buffer, 256);
        n = read(sockfd, buffer, 255);
        if (strcmp(buffer, "StartChat") == 0) {
            fcntl(sockfd, F_SETFL, MCAST_BLOCK_SOURCE);
            pthread_cancel(vlakno);

            bzero(buffer, 256);
            strcpy(buffer, "Starting chat");
            n = write(sockfd, buffer, 255);


            bzero(sprava, 256);
            n = read(sockfd, sprava, 255);
            system("clear");
            printf("Vas priatel %s chce svami zacat chatovat, chcete prijat ano/nie?", sprava);
            fflush(stdout);
            bzero(buffer, 256);
            fgets(buffer, 255, stdin);
            n = write(sockfd, buffer, 255);
            if(strcmp(buffer, "ano\n") == 0) {
                printf("Zacali ste chat s pouzivatelom %s\n", sprava);
                fflush(stdout);
                zacniChat();
            }
            pthread_create(&vlakno, NULL, &zberVstupu, (void *) &nic);
        }

        if (strcmp(buffer, "AddFriend") == 0) {
            pthread_cancel(vlakno);
            system("clear");
            printf("Vas priatel si vas chce pridat medzi priatelov, chcete prijat ano/nie?");
            bzero(buffer, 256);
            fgets(buffer, 255, stdin);
            n = write(sockfd, buffer, 255);
            pthread_create(&vlakno, NULL, &zberVstupu, (void *) &nic);
        }

        if (strcmp(buffer, "DeleteFriend") == 0) {
            system("clear");
            printf("Uzivatel si vas odstranil zo zoznamu priatelov\n");
        }

        if (strcmp(buffer, "SendFile") == 0) {
            system("clear");
            prijmiSubor(sockfd);
            printf("Dostali ste subor\n");
        }
        pthread_mutex_unlock(&mutex);

        int i = strcmp("Bye", buffer);
        if (i == 0) {
            break;
        }
    }

    printf("%s\n", buffer);
    close(sockfd);
}

