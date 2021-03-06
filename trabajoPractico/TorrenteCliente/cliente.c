#include "funcionesCliente.h"
argumentosThreads *argumentosEnviarArchivos;
arcCmp *cabeza;
arcCmp *finalListaCompartidos;
char ipServidor[16];
void menu(int descriptorSocket);
void lanzador(int opcionMenu, int descriptorSocket);

int main(int argc, char *argv[])
{
    if(leerConfiguraciones() != 0)
    {
        perror("ERROR al leer el archivo de configuraciones.");
        enviarALog(0, "ERROR al leer el archivo de configuraciones.");
        exit(EXIT_FAILURE);
    }
    printf("Conectando a %s", ipServidor);
    //Thread para enviar archivos
    pthread_t threadEnviar;
    pthread_attr_t atributosDelThreadEnviarArchivo;

    if (pthread_attr_init(&atributosDelThreadEnviarArchivo) != 0)
    {
        enviarALog(0, "problema: creando atributos del thread.");
        exit(EXIT_FAILURE);
    }

    cabeza = NULL;
    finalListaCompartidos = NULL;
    int descriptorSocket;
    char mensaje [512];
    struct sockaddr_in estructuraSocketCliente;	// Estructura del socket del cliente
    socklen_t tamanioDireccion;	// Tamaño de estructura SocketCliente

    /* crear el socket */
    if ((descriptorSocket = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        enviarALog(0, "socket");
        exit(EXIT_FAILURE);
    }

    /* Establecer el cliente */
    memset(&estructuraSocketCliente, 0, sizeof(estructuraSocketCliente));
    estructuraSocketCliente.sin_family = AF_INET;
    estructuraSocketCliente.sin_port = htons(50000);	// No olvidar el orden de los bytes en la red
    if (!(inet_aton(ipServidor, &estructuraSocketCliente.sin_addr)))	// Esto hara terminar el programa si la direccion ip es invalida
    {
        enviarALog(0, "inet_atom");
        exit(EXIT_FAILURE);
    }

    /* Conectarse al socket */
    tamanioDireccion = sizeof(estructuraSocketCliente);
    if ((connect(descriptorSocket, (struct sockaddr *) &estructuraSocketCliente, tamanioDireccion)) == -1)
    {
        enviarALog(0, "error:connect");
        exit(EXIT_FAILURE);
    }

    enviarALog(0, "Conectado a socket TCP/IP");
    sprintf(mensaje,"Puerto %d - Dirección %s ", ntohs(estructuraSocketCliente.sin_port),inet_ntoa(estructuraSocketCliente.sin_addr));
    enviarALog(0, mensaje);

    //thread que corre todo el tiempo, esperando si se solicitan archivos desde el servidor.
    argumentosEnviarArchivos = (argumentosThreads *) calloc(1, sizeof(argumentosThreads));
    argumentosEnviarArchivos->descriptorSocket = descriptorSocket;
    if (pthread_create(&threadEnviar, &atributosDelThreadEnviarArchivo, threadEnviarDescargarArchivos, (void *) argumentosEnviarArchivos) != 0)
    {
        enviarALog(0, "error:thread:threadEnviarDescargarArchivos");
        exit(EXIT_FAILURE);
    }

    menu(descriptorSocket);
    exit(EXIT_SUCCESS);
}

//Menú
void menu(int descriptorSocket)
{
    int opcionMenu;					//opción del menú

    do
    {
        printf("Seleccione la opción que desea hacer");
        printf("\n");
        printf("\t1 - Compartir una carpeta\n");
        printf("\t2 - Ver listado de usuarios conectados\n");
        printf("\t3 - Ver listado de archivos disponibles\n");
        printf("\t4 - Descargar un archivo\n");
        printf("\t5 - Salir\n");

        scanf("%d", &opcionMenu);

        if (opcionMenu < 1 || opcionMenu > 5)
        {
            system("clear");
            printf("\tLa opción del menu es inexistente\n\n");
        }
        else
        {
            lanzador(opcionMenu, descriptorSocket);
        }
    }
    while (opcionMenu != 5);
}

//Lanzador de Threads
void lanzador(int opcionMenu, int descriptorSocket)
{
    argumentosThreads *argumentosCarpetaCompartida;
    pthread_attr_t atributosDelThread1;
    pthread_t thread1;

    char rutaDescarga[256];
    char bufferRutaDescarga[256];
    char carpetaEscogida[256];

    //atributos para el thread
    if (pthread_attr_init(&atributosDelThread1) != 0)
    {
        enviarALog(0, "problema: creando atributos del thread.");
        exit(EXIT_FAILURE);
    }

    switch (opcionMenu)
    {
    case 1:
        system("clear");
        argumentosCarpetaCompartida = (argumentosThreads *) calloc(1, sizeof(argumentosThreads));
        printf("Ingrese la carpeta que desee compartir: ");
        scanf("%s", carpetaEscogida);
        strcpy(argumentosCarpetaCompartida->carpetaEscogida, carpetaEscogida);
        argumentosCarpetaCompartida->descriptorSocket = descriptorSocket;
        //Lanzo el thread
        if (pthread_create(&thread1, &atributosDelThread1, threadCompartirCarpeta, (void *) argumentosCarpetaCompartida) != 0)
        {
            enviarALog(0, "error:thread:CompartirCarpeta");
            exit(EXIT_FAILURE);
        }
        break;
    case 2:
        system("clear");
        break;
    case 3:
        system("clear");
        if (enviarOpcion(CLIENTE_SOLICITA_COMPARTIDOS_Y_CLIENTES, descriptorSocket) == 1)
            enviarALog(0, "ERROR:ENVIO:OPCION");
        break;
    case 4:
        printf("Ingrese el nombre de archivo a descargar: ");
        scanf("%s", rutaDescarga);
        if (enviarOpcion(CLIENTE_SOLICITA_DESCARGA, descriptorSocket) == 1)
            enviarALog(0, "ERROR:ENVIO:OPCION");
        memset(bufferRutaDescarga, '\0', 256);
        sprintf(bufferRutaDescarga, "%s", rutaDescarga);
        strcpy(argumentosEnviarArchivos->rutaDescarga, rutaDescarga);
        if(write(descriptorSocket, bufferRutaDescarga, 256) > 0)
            enviarALog(0, "Se envio la ruta y la opcion para descargar, al servidor.");
        else
            enviarALog(0, "ERROR:cliente.c:opcion:4:DescargarArchivo:NO SE PUDO ENVIAR rutaDescarga");
        break;
    case 5:
        system("clear");
        enviarALog(0, "Cliente desconectado: Chau amigo!!");
        break;
    }
}

