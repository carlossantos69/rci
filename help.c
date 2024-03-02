#include <stddef.h>
#include <stdio.h>

int print_help()
{
    printf("----------------------------------\n");
    printf("-             Usage              -\n");
    printf("----------------------------------\n");
    printf("join (j) ring id  -  entrada de nó \n");
    printf("direct join (dj) id succid succIP succTCP  - entrada de um nó diretamente\n");
    printf("chord (c) - estabelecimento de uma corda com um nó\n");
    printf("remove chord (rc) - eliminnação da corda\n");
    printf("show topology (st) - mostra estado topológico do nó");
    printf("show routing (sr) dest - mostra a tabela de encaminhamento do nó para o nó dest\n");
    printf("show path (sp) dest -  mostra o caminho mais curto para o nó dest\n");
    printf("show forwarding (sf) - mostra a tabela de expedição de um nó\n");
    printf("message (m) dest message - envio da mensagem m para o nó dest\n");
    printf("leave (l) - saida do nó do anel\n");
    printf("exit (x) - fecho da aplicação\n");
    printf("----------------------------------\n");

    return 0;
}