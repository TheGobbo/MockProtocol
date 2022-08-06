#include "servidor.h"
#include "Packet.h"
#include "ConexaoRawSocket.h"

// global para servidor
int soquete;
/* sniff sniff */
int main()
{
    int bytes;
    unsigned char buffer[TAM_PACOTE];                   // buffer tem no maximo 68 bytes
    // abre o socket -> lo vira ifconfig to pc que recebe
    // soquete = ConexaoRawSocket("lo");
    soquete = ConexaoRawSocket("enp2s0f1");


    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(soquete, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    while(1){
        bytes = recv(soquete, buffer, sizeof(buffer), 0);           // recebe dados do socket
        if(bytes>0 && is_our_packet(buffer))
        {   // processa pacote se eh nosso pacote
            read_packet(buffer);
            server_switch(buffer);
        }
    }

    close(soquete);
    printf("servidor terminado\n");
}

void server_switch(unsigned char* buffer)
{
    int tipo_lido = get_packet_type(buffer);

	switch (tipo_lido)
	{
        case OK:
            // funcao q redireciona
            break;
        case NACK:
            // funcao q redireciona
            break;
        case ACK:
            // funcao q redireciona
            break;
        case CD:
            cdc(buffer);
            break;
        case LS:
            // funcao q redireciona
            break;
        case MKDIR:
            mkdirc(buffer);
            break;
        case GET:
            get(buffer);
            break;
        case PUT:
            // funcao q redireciona
            break;
        case ERRO:
            // funcao q redireciona
            break;
        case DESC_ARQ:
            // funcao q redireciona
            break;
        case DADOS:
            // funcao q redireciona
            break;
        case FIM:
            // funcao q redireciona
            break;
        case SHOW_NA_TELA:
            // funcao q redireciona
            break;

		default:
			break;
	}
}


void cdc(unsigned char* buffer){
    int resultado;
    unsigned char *resposta;
    char *cd, flag[1];
    int bytes;
    int ret;

    cd = (char *) get_packet_data(buffer);
    char *d = malloc(strcspn(cd, " ")*sizeof(char));    // remove espaco no final da mensagem, se tem espaco da ruim 
    char pwd[PATH_MAX];                                 // "cd ..     " -> "..    " nn existe 
    strncpy(d, cd, strcspn(cd, " "));

    if(getcwd(pwd, sizeof(pwd)))printf("before: %s\n", pwd);
    ret = chdir(d);
    if(getcwd(pwd, sizeof(pwd)))printf("after: %s\n", pwd);

    free(d);
    if(ret == -1){
        resultado = ERRO;
        printf("erro foi : %s\n", strerror(errno));
        switch (errno){
            case 2:
                flag[0] = dir_nn_E;
                break;
            case 13:
                flag[0] = sem_permissao;
                break;
            default:
                break;
        };
    }
    else{
        resultado = OK;
    }
    resposta = make_packet(sequencia(), resultado, flag, 1);
    if(!resposta) // se pacote deu errado
        return;

    bytes = send(soquete, resposta, TAM_PACOTE, 0);                 // envia packet para o socket
    if(bytes<0)                                                     // pega erros, se algum
        printf("error: %s\n", strerror(errno));                     // print detalhes do erro

    free(resposta);
    free(cd);
}

// trata_tipos: case switch p/ cada ENUM uma funcao
// Tipo == ls -> funcao_ls(dados){ dados = a ou l ... } .... 

void mkdirc(unsigned char* buffer){
    int resultado;
    unsigned char *resposta;
    char flag[1];
    char *mkdir; // *dir;
    int bytes;
    int ret;

    // dir = (char *) get_packet_data(buffer);
    // char *d = malloc(strcspn(dir, " ")*sizeof(char));    // remove espaco no final da mensagem, se tem espaco da ruim 
    // strncpy(d, dir, strcspn(dir, " "));
    // mkdir = calloc(1, sizeof("mkdir")+sizeof(d));
    // strcat(strcpy(mkdir, "mkdir "), d);
    // free(d);
    // ret = system(mkdir);
    mkdir = get_packet_data(buffer);
    ret = system(mkdir);

    // 256 : erro q retorna se dir ja existe
    if(ret != 0){        
        resultado = ERRO;
        switch (ret){
            case 256:
                /* 1 = Operação não permitida */
                flag[0] = dir_ja_E;
                break;
            case 13*256:    
                flag[0] = sem_permissao;
                break;
            default:
                break;
        };
        printf("erro %d foi : %s ; flag (%s)\n",errno, strerror(errno), flag);
    }
    else{
        resultado = OK;
    }
    resposta = make_packet(sequencia(), resultado, flag, 1);
    if(!resposta) // se pacote deu errado
        return;

    bytes = send(soquete, resposta, TAM_PACOTE, 0);                 // envia packet para o socket
    if(bytes<0)                                                     // pega erros, se algum
        printf("error: %s\n", strerror(errno));                     // print detalhes do erro

    free_packet(resposta);
    free(mkdir);
}

void get(unsigned char *buffer){
    // int resultado;
    // unsigned char *resposta;
    // char *get, flag[1];
    // int bytes;
    // int ret;

    // get = (char *) get_packet_data(buffer);
    
}