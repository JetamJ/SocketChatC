#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Vlakno.h"
#include <fcntl.h>
#include <sys/syscall.h>


int sockfd, newsockfd;
socklen_t cli_len;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_t vlaknoChat;
struct sockaddr_in serv_addr, cli_addr;
int n;
char userName[256],prikaz[20],sprava[256];
FILE * fileUsers;
FILE * fileUsersNew;
FILE * fileFriends;
FILE * fileFriendsNew;
FILE * fp;
int port = 5021;
int line, ctr;
char str[256];
char help[256];
char vypniChat[256];

char vyhladavanie[256];
int pocetZaregistrovanych1;
int pocetZaregistrovanych2;

typedef enum { false, true } bool;

typedef struct user{
    int cisloPouzivatela;
    char meno[256];
    int cisloSocketu;
    bool jeVChate;
}Pouzivatel;

Vlakno poleVlakien;

Pouzivatel polePouzivatelov[100];

int counter = 0;

typedef struct {
    int cisloSocketu1;
    int cisloSocketu2;
}chat;

//porata kolko pismem sa nachadza v retazci od znaku [ po ]
int pocetPismen(int zaciatok, char buffer[]){
    int j = 0;
    for (int i = zaciatok; i < strlen(buffer); ++i) {
        j++;
        if (buffer[i] == ']'){
            j = j- 2;
            return j;
        }
    }
}

//rozlozi prikaz od klienta a jeho hodnoty zapsi do spravnych premien
void zapisPrikaz(char buffer[]){
    bzero(userName,256);
    bzero(prikaz,256);
    bzero(sprava,256);
    int prveSlovo = pocetPismen(0,buffer);
    for (int i = 0; i < prveSlovo; ++i) {
        userName[i] = buffer[i+1];
    }
    int druheSlovo = pocetPismen(prveSlovo + 2,buffer);
    for (int j = prveSlovo + 2 ; j < prveSlovo + 2 + druheSlovo; ++j) {
        prikaz[j - (prveSlovo + 2)] = buffer[j+1];
    }
    int tretieSlovo = pocetPismen(prveSlovo + 2 + druheSlovo + 2,buffer);
    for (int k = prveSlovo + 2 +druheSlovo + 2; k < prveSlovo + 2 + druheSlovo + 2 + tretieSlovo; ++k) {
        sprava[k - (prveSlovo + 2 +druheSlovo + 2)] = buffer[k+1];
    }
}

//vytiahne slovo z retazca zapisanom v tvare slovo1;slovo2;slovo3;...
void najdiSlovoRiadok(char buffer[]){
    bzero(help,256);
    int zaciatok = strcspn(buffer, ";");
    buffer[strcspn(buffer, ";")] = " ";
    int koniec = strcspn(buffer, ";");
    for (int i = zaciatok; i < koniec - 1; ++i) {
        help[i - zaciatok] = buffer[i+1];
    }
}

//zisti cislo socketo pouzivatela podla jeho user name
int getCisloSecketu(char meno[]){
    //pthread_mutex_lock(&mutex);
    //printf("Zacina sa zistovat cislo socketu pre menu %s\n",meno);
    for (int i = 0; i < counter; ++i) {
        if (strcmp(polePouzivatelov[i].meno, meno) == 0){
            //printf("Cislo socketu pre %s je %d\n",meno,polePouzivatelov[i].cisloSocketu);
            return polePouzivatelov[i].cisloSocketu;
        }
    }
    //pthread_mutex_unlock(&mutex);
    return -1;
}

//nastavi user name pouzivatelovi v poli polePouzivatelov na zaklade cisla socketu
void aktualizujPouzivatela(int cisloSocketu, char meno[]){
    //pthread_mutex_lock(&mutex);
    //printf("Zacina sa priradovat cislu socketu %d, meno %s\n",cisloSocketu,meno);
    for (int i = 0; i < counter; ++i) {
        if (polePouzivatelov[i].cisloSocketu == cisloSocketu){
            strcpy(polePouzivatelov[i].meno, meno);
            //printf("Socketu %d sa priradilo meno %s\n",polePouzivatelov[i].cisloSocketu, polePouzivatelov[i].meno);
            fflush(stdout);
        }
    }
    //pthread_mutex_unlock(&mutex);
}

//zisti kolko riadkov sa nachadza v subore users.txt kde su ulozene prihlasovacie udaje pouzivatelov
int getPocetRegistrovanych(){
    pocetZaregistrovanych1 = 0;
    fileUsers = fopen("users.txt","r");
    for (int i = getc(fileUsers); i != EOF; i = getc(fileUsers)) {
        if (i == '\n') {
            pocetZaregistrovanych1 = pocetZaregistrovanych1 + 1;
        }
    }
    fclose(fileUsers);
    return pocetZaregistrovanych1;
}

//zisti kolko riadkov sa nachadza v subore friends.txt kde su ulozeny priatelia kazdeho pouzivatela
int getPocetRegistrovanych2(){
    pocetZaregistrovanych2 = 0;
    fileFriends = fopen("friends.txt","r");
    for (int i = getc(fileFriends); i != EOF; i = getc(fileFriends)) {
        if (i == '\n') {
            pocetZaregistrovanych2 = pocetZaregistrovanych2 + 1;
        }
    }
    fclose(fileFriends);
    return pocetZaregistrovanych2;
}

//zisti ci sa pouzivatel name2 nachadaza vo friendliste pouzivatela name1
int zistiCiJeFriend(char name1[], char name2[], char buffer[]){
    fileFriends = fopen("friends.txt", "r");
    for (int j = 0; j < 2; ++j) {
        bzero(vyhladavanie, 256);
        fgets(vyhladavanie, strlen(name1) + 1, fileFriends);
        if(strcmp(vyhladavanie, name1) == 0){
            bzero(vyhladavanie, 256);
            fscanf(fileFriends,"%s", &vyhladavanie);
            bzero(buffer, 256);
            strcpy(buffer,vyhladavanie);
            break;
        }
        bzero(vyhladavanie, 256);
        fscanf(fileFriends,"%s", &vyhladavanie);
        bzero(vyhladavanie, 256);
        fgets(vyhladavanie, strlen(name1) + 1, fileFriends);
    }

    for (int k = 0; k < 2; ++k) {
        najdiSlovoRiadok(buffer);
        if(strcmp(help, name2) == 0){
            fclose(fileFriends);
            return 1;
        }
    }
    fclose(fileFriends);
    return -1;
}

void nastavParameterChat(int cisloSocketu, bool stav){
    for (int i = 0; i < counter; ++i) {
        if (polePouzivatelov[i].cisloSocketu == cisloSocketu){
            polePouzivatelov[i].jeVChate = stav;
        }
    }
}

//metoda urcena pre vlakno na chat, riesi komunikaciu s pouzivatelom2
void * zacniChat(void * data){
    printf("Vlakno cislo %d, zacalo chat\n", syscall(__NR_gettid));
    chat * chatData = (chat*)data;
    char buffer2[256];
    while (1) {
        bzero(buffer2, 256);
        n = read(chatData->cisloSocketu2, buffer2, 255);

        n = write(chatData->cisloSocketu1, buffer2, 255);
        if ((strcmp(buffer2, "Bye\n") == 0)) {
            strcpy(vypniChat, buffer2);
            break;
        }
    }
}

//funkcia na prijatie suboru, zapise prichodzie data do suboru
void prijmiSubor(int cisloSocketu){
    int n;
    char buffer[1024];

    fp = fopen("test_udaje.txt", "w+");
    while (1) {
        n = read(cisloSocketu, buffer, 1024);
        if (strcmp(buffer, "END") == 0){
            break;
        }
        fprintf(fp, "%s", buffer);
        bzero(buffer, 1024);
    }
    fclose(fp);
    return;
}

void posliFile(FILE * subor, int cisloSocketu){
    char data[1024] = {0};
    char buffer[256];

    while(fgets(data, 1024, subor) != NULL) {
        if (send(cisloSocketu, data, sizeof(data), 0) == -1) {
            perror("Posielanie suboru zlyhalo");
            break;
        }
        bzero(data, 1024);
    }
    sleep(1);
    strcpy(buffer, "END");
    send(cisloSocketu, buffer, sizeof(buffer), 0);
    printf("Subor bol uspesne odoslany\n");
}

//prida priatela zadanemu uzivatelovi, zapise ho do suboru friends.txt na spravne miesto
void pridajPriatela(char name1[], char name2[], char buffer[]){
    pocetZaregistrovanych2 = getPocetRegistrovanych2();
    ctr = 0;
    fileFriends = fopen("friends.txt", "r");
    fileFriendsNew = fopen("temporary.txt", "w+");

    for (int j = 0; j < pocetZaregistrovanych2; ++j) {
        bzero(vyhladavanie, 256);
        fgets(vyhladavanie, strlen(name1) + 1, fileFriends);
        if(strcmp(vyhladavanie, name1) == 0){
            line = j + 1;
            break;
        }
        bzero(vyhladavanie, 256);
        fscanf(fileFriends,"%s", &vyhladavanie);
        bzero(vyhladavanie, 256);
        fgets(vyhladavanie, strlen(name1) + 1, fileFriends);
    }
    rewind (fileFriends);
    while (!feof(fileFriends))
    {
        bzero(str, 256);
        fscanf(fileFriends, "%s", &str);
        if (!feof(fileFriends))
        {
            ctr++;
            if (ctr != line)
            {
                fprintf(fileFriendsNew, "%s\n", str);
            } else {
                fprintf(fileFriendsNew, "%s", str);
                fprintf(fileFriendsNew, "%s;\n", name2);
            }
        }
    }
    fclose(fileFriends);
    fclose(fileFriendsNew);
    rename("temporary.txt", "friends.txt");
}

//vymaze pouzivatela name2 z friendlistu pouzivatela name1
void vymazPriatela(char name1[], char name2[], char buffer[]){
    pocetZaregistrovanych2 = getPocetRegistrovanych2();
    fileFriends = fopen("friends.txt", "r");
    ctr = 0;
    for (int j = 0; j < pocetZaregistrovanych2; ++j) {
        bzero(vyhladavanie, 256);
        fgets(vyhladavanie, strlen(name1) + 1, fileFriends);
        if(strcmp(vyhladavanie, name1) == 0){
            bzero(vyhladavanie, 256);
            fscanf(fileFriends,"%s", &vyhladavanie);
            bzero(buffer, 256);
            strcpy(buffer,vyhladavanie);
            fclose(fileFriends);
            break;
        }
        bzero(vyhladavanie, 256);
        fscanf(fileFriends,"%s", &vyhladavanie);
        bzero(vyhladavanie, 256);
        fgets(vyhladavanie, strlen(name1) + 1, fileFriends);
    }

    fileFriends = fopen("friends.txt", "r");
    fileFriendsNew = fopen("temporary.txt", "w+");

    for (int j = 0; j < pocetZaregistrovanych2; ++j) {
        bzero(vyhladavanie, 256);
        fgets(vyhladavanie, strlen(name1) + 1, fileFriends);
        if(strcmp(vyhladavanie, name1) == 0){
            line = j + 1;
            break;
        }
        bzero(vyhladavanie, 256);
        fscanf(fileFriends,"%s", &vyhladavanie);
        bzero(vyhladavanie, 256);
        fgets(vyhladavanie, strlen(name1) + 1, fileFriends);
    }
    rewind(fileFriends);
    while (!feof(fileFriends))
    {
        bzero(str, 256);
        fscanf(fileFriends, "%s", &str);
        if (!feof(fileFriends))
        {
            ctr++;
            if (ctr != line)
            {
                fprintf(fileFriendsNew, "%s\n", str);
            } else {
                fprintf(fileFriendsNew, "%s;", name1);
                for (int k = 0; k < pocetZaregistrovanych2; ++k) {
                    najdiSlovoRiadok(buffer);
                    if ((strcmp(help, name2) == 0) || (strcmp(help, "") == 0) ){
                    } else {
                        fprintf(fileFriendsNew, "%s;", help);
                    }
                }
                fprintf(fileFriendsNew, "\n");
            }
        }
    }
    fclose(fileFriends);
    fclose(fileFriendsNew);
    rename("temporary.txt", "friends.txt");
}

//spracuje prikaz ktory uzivatel zada --- Login, Create, AddFriend, Friendlist, Startchat, DeleteFriend, DeleteAccount
void spracujPrikaz(char prikaz[], void * data, char buffer[]){
    Pouzivatel * dataP = (Pouzivatel*)data;
    if (strcmp(prikaz, "Create") == 0){
        aktualizujPouzivatela(dataP->cisloSocketu, userName);
        int cisloSocketu = getCisloSecketu(userName);
        pocetZaregistrovanych1 = getPocetRegistrovanych();
        fileUsers = fopen("users.txt", "a+");
        for (int i = 0; i < pocetZaregistrovanych1; ++i) {
            bzero(vyhladavanie,256);
            fscanf(fileUsers,"%s", &vyhladavanie);
            if(strcmp(vyhladavanie, userName) == 0){
                bzero(buffer, 256);
                strcpy(buffer,"NOK");
                n = write(cisloSocketu, buffer, strlen(buffer) + 1);
                return;
            }
        }
        fprintf(fileUsers, "%s\n", userName);
        fprintf(fileUsers, "%s\n", sprava);
        fclose(fileUsers);
        fileFriends = fopen("friends.txt", "a+");
        fseek(fileFriends,0,SEEK_END);
        fprintf(fileFriends, "%s;\n", userName);
        fclose(fileFriends);

        bzero(buffer, 256);
        strcpy(buffer,"OK");
        n = write(cisloSocketu, buffer, strlen(buffer) + 1);
    }

    if (strcmp(prikaz, "Login") == 0){
        aktualizujPouzivatela(dataP->cisloSocketu, userName);
        int cisloSocketu = getCisloSecketu(userName);
        pocetZaregistrovanych1 = getPocetRegistrovanych();
        fileUsers = fopen("users.txt", "r");
        for (int i = 0; i < pocetZaregistrovanych1; ++i) {
            bzero(vyhladavanie,256);
            fscanf(fileUsers,"%s", &vyhladavanie);
            if (strcmp(vyhladavanie, userName) == 0){
                bzero(vyhladavanie,256);
                fscanf(fileUsers,"%s", &vyhladavanie);
                if(strcmp(vyhladavanie, sprava) == 0){
                    bzero(buffer, 256);
                    strcpy(buffer,"OK");
                    //printf("Server posiela odpoved");
                    n = write(cisloSocketu, buffer, strlen(buffer) + 1);
                    return;
                } else {
                    bzero(buffer, 256);
                    //printf("Server posiela odpoved");
                    strcpy(buffer,"NOK");
                    n = write(cisloSocketu, buffer, strlen(buffer) + 1);
                    return;
                }
            }
        }
        bzero(buffer, 256);
        //printf("Server posiela odpoved");
        strcpy(buffer,"NOK");
        n = write(cisloSocketu, buffer, strlen(buffer) + 1);
        fclose(fileUsers);
    }

    if (strcmp(prikaz, "StartChat") == 0) {
        int cisloSocketu1 = getCisloSecketu(userName);
        int cisloSocketu2 = getCisloSecketu(sprava);
        bzero(vypniChat,256);
        if (zistiCiJeFriend(userName, sprava, buffer) == -1) {
            bzero(buffer, 256);
            strcpy(buffer, "NOK");
            n = write(cisloSocketu1, buffer, strlen(buffer) + 1);
            return;
        }
        if (cisloSocketu2 == -1) {
            bzero(buffer, 256);
            strcpy(buffer,"NOK");
            n = write(cisloSocketu1, buffer, strlen(buffer) + 1);
            return;
        } else {
            nastavParameterChat(cisloSocketu1, true);
            nastavParameterChat(cisloSocketu2, true);
            bzero(buffer, 256);
            strcpy(buffer, "StartChat");
            n = write(cisloSocketu2, buffer, strlen(buffer) + 1);
            sleep(1);

            bzero(buffer, 256);
            strcpy(buffer, userName);
            n = write(cisloSocketu2, buffer, strlen(buffer) + 1);

            bzero(buffer, 256);
            n = read(cisloSocketu2, buffer, 255);
            if(strcmp(buffer, "ano\n") == 0) {
                bzero(buffer, 256);
                strcpy(buffer, "OK");
                n = write(cisloSocketu1, buffer, 255);
                chat chat;
                chat.cisloSocketu1 = cisloSocketu1;
                chat.cisloSocketu2 = cisloSocketu2;
                //printf("Idem vytvorit vlakno pre chat z vlakna %d", syscall(__NR_gettid));
                pthread_create(&vlaknoChat,NULL,&zacniChat, &chat);
                while (1) {
                    //printf("Vlakno cislo %d, pracuje\n", syscall(__NR_gettid));
                    bzero(buffer, 256);
                    n = read(cisloSocketu1, buffer, 255);

                    n = write(cisloSocketu2, buffer, 255);
                    if ((strcmp(buffer, "Bye\n") == 0) || (strcmp(vypniChat, "Bye\n") == 0)) {
                        pthread_cancel(vlaknoChat);
                        printf("Ukoncili sme vlakno na chat\n");
                        nastavParameterChat(cisloSocketu1, false);
                        nastavParameterChat(cisloSocketu2, false);
                        break;
                    }
                    }
                nastavParameterChat(cisloSocketu1, false);
                nastavParameterChat(cisloSocketu2, false);
                bzero(buffer, 256);
                strcpy(buffer,"END");
                n = write(cisloSocketu1, buffer, 255);
                n = write(cisloSocketu2, buffer, 255);
            } else {
                bzero(buffer, 256);
                strcpy(buffer,"NOK");
                n = write(cisloSocketu1, buffer, strlen(buffer) + 1);
            }
        }
    }


    if (strcmp(prikaz, "AddFriend") == 0) {
        int cisloSocketu1 = getCisloSecketu(userName);
        int cisloSocketu2 = getCisloSecketu(sprava);
        if (zistiCiJeFriend(userName, sprava, buffer) == 1) {
            bzero(buffer, 256);
            strcpy(buffer, "NOK");
            n = write(cisloSocketu1, buffer, strlen(buffer) + 1);
            return;
        }
        if (cisloSocketu2 == -1) {
            bzero(buffer, 256);
            strcpy(buffer, "NOK");
            n = write(cisloSocketu1, buffer, strlen(buffer) + 1);
            return;
        } else {
            bzero(buffer, 256);
            strcpy(buffer, "AddFriend");
            n = write(cisloSocketu2, buffer, strlen(buffer) + 1);

            bzero(buffer, 256);
            n = read(cisloSocketu2, buffer, 255);
            if (strcmp(buffer, "ano\n") == 0) {
                pridajPriatela(userName, sprava, buffer);
                pridajPriatela(sprava, userName,  buffer);
                bzero(buffer, 256);
                strcpy(buffer, "OK");
                n = write(cisloSocketu1, buffer, strlen(buffer) + 1);
            } else {
                bzero(buffer, 256);
                strcpy(buffer, "NOK");
                n = write(cisloSocketu1, buffer, strlen(buffer) + 1);
            }
        }
    }


    if (strcmp(prikaz, "DeleteFriend") == 0) {
        int cisloSocketu = getCisloSecketu(userName);
        int cisloSocketu2 = getCisloSecketu(sprava);
        if (zistiCiJeFriend(userName, sprava, buffer) == -1){
            bzero(buffer, 256);
            strcpy(buffer,"NOK");
            n = write(cisloSocketu, buffer, strlen(buffer) + 1);
            return;
        }
        vymazPriatela(userName,sprava,buffer);
        vymazPriatela(sprava,userName,buffer);
        bzero(buffer, 256);
        strcpy(buffer,"OK");
        n = write(cisloSocketu, buffer, strlen(buffer) + 1);
        bzero(buffer, 256);
        strcpy(buffer, "DeleteFriend");
        n = write(cisloSocketu2, buffer, strlen(buffer) + 1);
    }


    if (strcmp(prikaz, "FriendList") == 0){
        int cisloSocketu = getCisloSecketu(userName);
        pocetZaregistrovanych2 = getPocetRegistrovanych2();
        fileFriends = fopen("friends.txt", "r");

        for (int j = 0; j < pocetZaregistrovanych2; ++j) {
            bzero(vyhladavanie, 256);
            fgets(vyhladavanie, strlen(userName) + 1, fileFriends);
            if(strcmp(vyhladavanie, userName) == 0){
                bzero(vyhladavanie, 256);
                fscanf(fileFriends,"%s", &vyhladavanie);
                bzero(buffer, 256);
                strcpy(buffer,vyhladavanie);
                n = write(cisloSocketu, buffer, strlen(buffer) + 1);
                fclose(fileFriends);
                return;
            }
            bzero(vyhladavanie, 256);
            fscanf(fileFriends,"%s", &vyhladavanie);
            bzero(vyhladavanie, 256);
            fgets(vyhladavanie, strlen(userName) + 1, fileFriends);
        }
        fclose(fileFriends);
    }

    if (strcmp(prikaz, "DeleteAcc") == 0){
        int cisloSocketu = getCisloSecketu(userName);
        pocetZaregistrovanych1 = getPocetRegistrovanych();
        pocetZaregistrovanych2 = getPocetRegistrovanych2();
        ctr = 0;
        fileUsers = fopen("users.txt", "r");
        fileUsersNew = fopen("temp.txt", "w");

        for (int i = 0; i < pocetZaregistrovanych1; ++i) {
            bzero(vyhladavanie, 256);
            fscanf(fileUsers, "%s", &vyhladavanie);
            if (strcmp(vyhladavanie, userName) == 0) {
                line = i+1;
                bzero(vyhladavanie, 256);
                fscanf(fileUsers, "%s", &vyhladavanie);
                if(strcmp(vyhladavanie, sprava) == 0) {
                    rewind (fileUsers);
                    while (!feof(fileUsers))
                    {
                        bzero(str, 256);
                        fscanf(fileUsers, "%s", &str);
                        if (!feof(fileUsers))
                        {
                            ctr++;
                            if (ctr != line && ctr != line+1)
                            {
                                fprintf(fileUsersNew, "%s\n", str);
                            }
                        }
                    }
                    fclose(fileUsers);
                    fclose(fileUsersNew);
                    remove(fileUsers);
                    rename("temp.txt", "users.txt");

                    //vymazanie so zoznamu friendov
                    fileFriends = fopen("friends.txt", "r");
                    fileFriendsNew = fopen("temporary.txt", "w+");
                    ctr = 0;
                    for (int j = 0; j < pocetZaregistrovanych2; ++j) {
                        bzero(vyhladavanie, 256);
                        fgets(vyhladavanie, strlen(userName) + 1, fileFriends);
                        if (strcmp(vyhladavanie, userName) == 0) {
                            line = j + 1;
                            break;
                        }
                        bzero(vyhladavanie, 256);
                        fscanf(fileFriends,"%s", &vyhladavanie);
                        bzero(vyhladavanie, 256);
                        fgets(vyhladavanie, strlen(userName) + 1, fileFriends);
                    }
                    rewind (fileFriends);
                    while (!feof(fileFriends))
                    {
                        bzero(str, 256);
                        fscanf(fileFriends, "%s", &str);
                        if (!feof(fileFriends))
                        {
                            ctr++;
                            if (ctr != line)
                            {
                                fprintf(fileFriendsNew, "%s\n", str);
                            }
                        }
                    }
                    fclose(fileFriends);
                    fclose(fileFriendsNew);
                    rename("temporary.txt", "friends.txt");

                    bzero(buffer, 256);
                    strcpy(buffer,"OK");
                    n = write(cisloSocketu, buffer, strlen(buffer) + 1);
                    return;
                } else {
                    fclose(fileUsers);
                    fclose(fileUsersNew);
                    bzero(buffer, 256);
                    strcpy(buffer,"NOK");
                    n = write(cisloSocketu, buffer, strlen(buffer) + 1);
                    return;
                }
            }
        }
    }

    if (strcmp(prikaz, "SendFile") == 0){
        int cisloSocketu1 = getCisloSecketu(userName);
        int cisloSocketu2 = getCisloSecketu(sprava);
        prijmiSubor(cisloSocketu1);
        bzero(buffer, 256);
        strcpy(buffer, "SendFile");
        n = write(cisloSocketu2, buffer, strlen(buffer) + 1);
        fp = fopen("test_udaje.txt","r");
        posliFile(fp,cisloSocketu2);
        fclose(fp);
    }
}

//zakladna metoda pre pouzivatela ktora stale pocuva na spravy od klienta
void * obsluha(void * data){
    char bufferSobitny[256];
    Pouzivatel * dataP = (Pouzivatel*)data;
    //printf("Vytvorilo sa vlakno %d\n", syscall(__NR_gettid));
    while(1) {

        if(dataP->jeVChate == false) {

            // printf("Server Caka na prikaz od uzivatel:%d socket:%d\n",dataP->cisloPouzivatela ,dataP->cisloSocketu);
            //printf("Server Caka na prikaz\n");
            //printf("Socket cislo %d, vlakno cislo %d\n",dataP->cisloSocketu, syscall(__NR_gettid));
            //fflush(stdout);
            n = read(dataP->cisloSocketu, bufferSobitny, 255);
            if(dataP->jeVChate == false) {
                printf("%s\n", bufferSobitny);
                fflush(stdout);

                zapisPrikaz(bufferSobitny);

                spracujPrikaz(prikaz, data, bufferSobitny);
                printf("Server posiela %s\n", bufferSobitny);

                bzero(bufferSobitny, 256);
                int i = strcmp("Bye", bufferSobitny);
                if (i == 0) {
                    break;
                }
            }
        }
    }
    close(dataP->cisloSocketu);
}

//vytvori socket pre server
void vytvorZakladnyScoket(){
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
      perror("Error creating socket");
    }
    //fcntl(sockfd, F_SETFL, O_NONBLOCK);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("Error binding socket address");
    }
}

//caka na pripojenie od klienta, ak je uspesne vytvori prenho socket a vlakno
int main(int argc, char *argv[]) {

    initThreads(&poleVlakien, 50);

    vytvorZakladnyScoket();

    listen(sockfd, 5);

    while (1) {
    cli_len = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
    //fcntl(newsockfd, F_SETFL, O_NONBLOCK);
        if (newsockfd < 0) {
            perror("ERROR on accept");
            return 3;
        }  else {
            polePouzivatelov[counter].cisloPouzivatela = counter;
            polePouzivatelov[counter].cisloSocketu = newsockfd;
            polePouzivatelov[counter].jeVChate = false;
            pthread_create(&poleVlakien.arrayOfThreads[poleVlakien.used], NULL, &obsluha, &polePouzivatelov[counter]);
            counter++;
        }
    }
    freeThreads(&poleVlakien);
    close(sockfd);
    return 0;
}


