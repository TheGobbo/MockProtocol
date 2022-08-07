#include "ConexaoRawSocket.h"
#include "cliente.h"
#include "Packet.h"

// todos no cliente podem ver, mas servidor nao
int soquete;

int main(){
    char pwd[PATH_MAX];
    char comando[COMMAND_BUFF];

    // soquete = ConexaoRawSocket("lo");        // abre o socket -> lo vira ifconfig to pc que manda
    soquete = ConexaoRawSocket("enp1s0f1");     // abre o socket -> lo vira ifconfig to pc que manda

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    while(1){
        if(getcwd(pwd, sizeof(pwd)))    // se pegou pwd, (!NULL)
            printf(GREEN "limbo@anywhere" RESET ":" BLUE "%s" RESET "$ ", pwd);

        fgets(comando, COMMAND_BUFF, stdin);
        client_switch(comando);
    } 

    close(soquete);                                        // fecha socket
    return 0;
}


/* final do comando c : client
 * final do comando s : server
 * comandos aceitos   : cd, mkdir, ls, get, put
 */
void client_switch(char* comando){
    int ret;
    char *parametro = comando;      // = comando pro make nn reclama, dpois tiro
    comando[strcspn(comando, "\n")] = 0;                // remove new line

    // LOCAL //
	if(strncmp(comando, "lsc", 3) == 0)
    {
        parametro[2] = ' ';                               // remove 'c' : lsc -> ls_
        ret = system(parametro);
        if(ret == -1)
            printf("ERRO: %s, errno: %d  parametro: (%s)\n", strerror(errno), errno, parametro);
    }
    else if(strncmp(comando, "cdc", 3) == 0)
    {
        parametro = comando+3;                              // remove "cdc"
        ret = chdir((parametro+strspn(parametro, " ")));    // remove espacos antes do parametro
        if(ret == -1)
            printf("ERRO: %s, errno: %d  parametro: (%s)\n", strerror(errno), errno, parametro);
    }
    else if(strncmp(comando, "mkdirc", 6) == 0)
    {
        parametro[5] = ' ';                               // remove 'c' : mkdirc_[]-> mkdir__[]
        ret = system(parametro);
        if(ret == -1)
            printf("ERRO: %s, errno: %d  parametro: (%s)\n", strerror(errno), errno, parametro);
    }

    // SERVIDOR //
    else if(strncmp(comando, "cds", 3) == 0)
    {   // chdir nao pode ter espacos errados
        parametro += 3;                       // remove "cds"
        parametro += strspn(parametro, " ");    // remove ' '  no inicio do comando
        if(cliente_sinaliza(parametro, CD))
            printf("moveu de diretorio no servidor com sucesso!\n");
        else
            printf("nao foi possivel mover de diretorio no servidor..\n");
    }
    else if(strncmp(comando, "mkdirs", 6) == 0)
    {   // mkdir usa syscall, pode ter espacos
        parametro[5] = ' ';                   // "mkdirs_[...]" -> "mkdir__[...]" 

        if(cliente_sinaliza(comando, MKDIR))
            printf("criou diretorio no servidor com sucesso!\n");
        else
            printf("nao foi possivel criar diretorio no servidor..\n");
    }
    else if(strncmp(comando, "lss", 3) == 0)
    {   

    }
    else if (strncmp(comando, "get", 3) == 0)
    {
        get(comando);
    }
    else if (strncmp(comando, "put", 3) == 0)
    {
        
    }
    else
    {
        if(strncmp(comando, "exit", 4) == 0)
        {   // sair com estilo
            printf(RED "CLIENT TERMINATED\n" RESET);      
            exit(0);
        }
        if(comando[0] != 0)     // diferente de um enter
            printf("comando invalido: %s\n", comando);
    }
}


/* errno: 
 *A - No such file or directory : 2
 *B - Permission denied : 13
 */
int cliente_sinaliza(char *comando, int tipo)
{

    int bytes, timeout, seq, lost_conn;
    unsigned char resposta[TAM_PACOTE];
    char* data;

    seq = sequencia();
    /* cria pacote com parametro para cd no server */
    unsigned char *packet = make_packet(seq, tipo, comando, strlen(comando));
    if(!packet)
        fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");

    // read_packet(packet);

    timeout = lost_conn = 0;
    // exit if received 3 timeouts, or return if OK
    while(lost_conn<3){ 

        bytes = send(soquete, packet, TAM_PACOTE, 0);       // envia packet para o socket
        if(bytes < 0){                                      // pega erros, se algum
            printf("error: %s\n", strerror(errno));  
        }

        /* said do loop soh se server responde ok */
        timeout = 0;
        while(1)
        {   
            if(timeout == 3){
                lost_conn++;
                break;
            }
            bytes = recv(soquete, resposta, TAM_PACOTE, 0);
            // if(errno == EAGAIN)    // nao tem oq ler
            if(errno)
            {
                printf("recv error : %s; errno: %d\n", strerror(errno), errno);
                timeout++;
            }

            if( bytes>0 && 
                is_our_packet((unsigned char *)resposta) 
                && get_packet_sequence(resposta) == get_seq()
            ){  
                data = get_packet_data(resposta);
                switch (get_packet_type(resposta))
                {
                    case OK:
                        printf("resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
                        free_packet(packet);
                        free(data);
                        return true;

                    case NACK:
                        break;  // exit response loop & re-send 

                    case ERRO:
                        printf("resposta: (%s) ; mensagem: (%s)\n", get_type_packet(resposta), data);
                        free_packet(packet);
                        free(data);
                        return false;
                }
                free(data);
            }
        }
    }

    printf("Erro de comunicacao, servidor nao responde :(\n");

    free_packet(packet);
    return false;
}

// função para tratar o get
void get(char *comando){

    int bytes, timeout, seq, fim, lost_conn;
    unsigned char resposta[TAM_PACOTE];

    /* filtra pacote, envia somente parametro do get */
    comando += 3;                       // remove "get"
    comando += strspn(comando, " ");    // remove ' '  no inicio do comando

    /* cria pacote com parametro para get no server */
    unsigned char *packet = make_packet(sequencia(), GET, comando, strlen(comando));
    if(!packet)
        fprintf(stderr, "ERRO NA CRIACAO DO PACOTE\n");

    timeout = fim = lost_conn = 0;
    // exit if fim received or 3 timeouts
    while(!fim && lost_conn<3){ 
        bytes = send(soquete, packet, TAM_PACOTE, 0);       // envia packet para o socket
        if(bytes < 0){                                      // pega erros, se algum
            printf("error: %s\n", strerror(errno));  
        }
        printf("%d bytes enviados no socket %d\n", bytes, soquete);
        // recv(soquete, packet, TAM_PACOTE, 0); //pra lidar com loop back

        /* said do loop soh se server responde fim */
        timeout = 0;
        while(1)
        {   
            if(timeout == 3){
                lost_conn++;
                break;
            }
            bytes = recv(soquete, resposta, TAM_PACOTE, 0);
            // if(errno == EAGAIN)    // nao tem oq ler
            if(errno)
            {
                printf("recv error : %s; errno: %d\n", strerror(errno), errno);
                timeout++;
            }
            seq = get_packet_sequence(resposta);
            if( bytes>0 && 
                is_our_packet((unsigned char *)resposta) && 
                seq == next_seq()
            ){
                switch (get_packet_type(resposta))
                {
                    case DESC_ARQ:
                            // MEXER AQUI !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        return;

                    case NACK:
                        break;  // exit response loop & re-send 

                    case ERRO:
                        printf("erro: servidor respondeu %s\n", get_packet_data(resposta));
                        return;
                }
            }
        }
    }
}



