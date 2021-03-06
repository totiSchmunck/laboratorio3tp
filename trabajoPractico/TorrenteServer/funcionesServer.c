#include "funcionesServer.h"
extern arcCmp *cabeza;
extern arcCmp *finalListaCompartidos;
extern tipoConectados *finalClientesConectados;
extern tipoConectados *clientesConectados;
extern int cantidadDescargasTotales;
extern char arrayIpsFiltradas[256][16];
extern int cantidadDeConexiones;
int recibirCompartidos(int srcfd)
{
    char buffNom[256];
    char buffTam[64];
    int cnt;
    int i = 0, n = 0;
    char rndChar[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789./";
    char salt[3];
    srandom(time(0));
    salt[0] = rndChar[random() % 64];
    salt[1] = rndChar[random() % 64];
    salt[3] = 0;

    //leo la cantidad - 64bytes
    if ((cnt = read(srcfd, buffTam, 64)) > 0)
        n = atoi(buffTam);		//cantidad de archivos a recibir
    if (n > 0)
    {
        //cargo en la primera pos. lo primero que recibo
        if ((cnt = read(srcfd, buffNom, 256)) > 0)
            if ((cnt = read(srcfd, buffTam, 64)) > 0)
                insertarEnLista(srcfd, buffNom, atoi(buffTam), crypt(buffNom, salt));
        //y avanzo con el resto
        while (i < n - 1)
        {
            salt[0] = rndChar[random() % 64];
            salt[1] = rndChar[random() % 64];
            salt[3] = 0;

            if ((cnt = read(srcfd, buffNom, 256)) > 0)
                if ((cnt = read(srcfd, buffTam, 64)) > 0)
                    insertarEnLista(srcfd, buffNom, atoi(buffTam), crypt(buffNom, salt));
            i++;
        }
    }
    return 0;
}

void mostrarListaCompartidos()
{
    printf("\n\t\t\tMostrando la lista.\n");
    arcCmp *regArc = cabeza;
    while (regArc != NULL)
    {
        printf("\nFileName: %s, Tamaño: %d, ID: %s, Cantidad de Descargas: %d", regArc->nom, regArc->tam, regArc->id, regArc->cantidadDescargas);
        regArc = regArc->sig;
    }
}

int enviarListaCompartidos(arcCmp * copia, int cliente)
{
    int bytesEscritos;
    char bufferCliente[8];
    char bufferNombreArchivo[256];
    char bufferTamanio[64];
    char bufferIdArchivo[256];
    char mensaje [512];
    sprintf(mensaje,"Enviando la lista a cliente %d, envio la opción para que la reciba.", cliente);
    enviarALog(mensaje);
    arcCmp *regArc = cabeza;
    //Le aviso al cliente que tiene que recibir la lista.
    if (enviarOpcion(SERVIDOR_ENVIA_LISTA_COMPARTIDOS, cliente) == 1)
    {
        enviarALog("ERROR:enviarListaCompartidos:OPCION_SERVIDOR_ENVIA_LISTA_COMPARTIDOS");
        exit(EXIT_FAILURE);
    }
    //Envio cantidad de archivos que tiene que recibir.
    int cantidad = contarCantArch();
    memset(bufferTamanio,'\0', 64);
    sprintf(bufferTamanio, "%d", cantidad);
    if((bytesEscritos = write(cliente, bufferTamanio, sizeof(bufferTamanio))) == 1)
    {
        enviarALog("ERROR:funcionesServer.c:enviarListaCompartidos:No se pudo enviar la cantidad.");
        exit(EXIT_FAILURE);
    }
    while (regArc != NULL)
    {
        memset(bufferCliente, '\0', 8);
        memset(bufferNombreArchivo,'\0', 256);
        memset(bufferTamanio,'\0', 64);
        memset(bufferIdArchivo, '\0', 256);
        sprintf(bufferCliente, "%d", regArc->cliente);
        sprintf(bufferNombreArchivo, "%s", regArc->nom);
        sprintf(bufferTamanio, "%d", regArc->tam);
        sprintf(bufferIdArchivo, "%s", regArc->id);
        //TODO: validar writes
        bytesEscritos = write(cliente, bufferCliente, 8);
        bytesEscritos = write(cliente, bufferNombreArchivo, 256);
        bytesEscritos = write(cliente, bufferTamanio, 64);
        bytesEscritos = write(cliente, bufferIdArchivo, 256);
        regArc = regArc->sig;
    }
}

//Inserta al final de una lista de archivos compartidos
void insertarEnLista(int cliente, char nombreDeArchivo[], int tamanioArchivo, char idArchivo[])
{
    arcCmp *nuevoArchivoCompartido = (arcCmp *) malloc(sizeof(arcCmp));
    strcpy(nuevoArchivoCompartido->nom, nombreDeArchivo);
    strcpy(nuevoArchivoCompartido->id, idArchivo);
    nuevoArchivoCompartido->tam = tamanioArchivo;
    nuevoArchivoCompartido->cliente = cliente;
    nuevoArchivoCompartido->cantidadDescargas = 0;
    nuevoArchivoCompartido->sig = NULL;
    if (finalListaCompartidos != NULL)
    {
        finalListaCompartidos->sig = nuevoArchivoCompartido; //Engancho el ultimo con el nuevo.
        finalListaCompartidos = finalListaCompartidos->sig; //Avanzo el final de lista porque agregue uno
    }
    else     //Si es el primero de la lista es el ultimo.
    {
        finalListaCompartidos = nuevoArchivoCompartido;
        cabeza = finalListaCompartidos;
    }
}
//Agrega un cliente a la lista de conectados
void insertarCliente(int socketDescriptor)
{
    enviarALog("Sume un cliente a la lista de clientes.");
    tipoConectados *nuevoCliente = (tipoConectados*)malloc(sizeof(tipoConectados));
    nuevoCliente->socketDescriptor = socketDescriptor;
    nuevoCliente->sig = NULL;
    if(finalClientesConectados)
    {
        finalClientesConectados->sig = nuevoCliente;
        finalClientesConectados = finalClientesConectados->sig;
    }
    else
    {
        finalClientesConectados = nuevoCliente;
        clientesConectados = finalClientesConectados;
    }
}
//Elimina un cliente a la lista de conectados
void eliminarCliente(int socketDescriptor)
{
    enviarALog("Se elimino un cliente de la lista de conectados.");
    tipoConectados *temp;
    tipoConectados *copia;
    if((clientesConectados != NULL) && (clientesConectados->socketDescriptor == socketDescriptor))
    {
        temp = clientesConectados->sig;
        free(clientesConectados);
        clientesConectados = temp;
    }
    copia = clientesConectados;
    if(copia != NULL)
    {
        while((copia->sig != NULL) && (copia->sig->socketDescriptor == socketDescriptor))
        {
            temp = copia->sig->sig;
            free(copia->sig);
            copia->sig = temp;
        }
    }
}

void mostrarListaConectados()
{
    tipoConectados *copia = clientesConectados;
    while(copia)
    {
        printf("\nCliente: %d\n", copia->socketDescriptor);
        copia = copia->sig;
    }
}

int cantidadDeClientes()
{
    int cantidadDeClientes = 0;
    tipoConectados *copia = clientesConectados;
    while(copia)
    {
        cantidadDeClientes++;
        copia = copia->sig;
    }
    return cantidadDeClientes;
}

void borrarClienteDeLaLista(int socketDelCliente)  //Borro todos los archivos de un cliente
{
    arcCmp *temp;
    arcCmp *copia;
    int archivosEliminados = 0;
    char mensaje[256];
    while((cabeza != NULL) && (cabeza->cliente == socketDelCliente))
    {
        archivosEliminados++;
        temp = cabeza->sig;
        free(cabeza);
        cabeza = temp;
    }
    copia = cabeza;
    if(copia != NULL)
    {
        while(copia->sig != NULL)
        {
            if(copia->sig->cliente == socketDelCliente)
            {
                archivosEliminados++;
                temp = copia->sig->sig;
                free(copia->sig);
                copia->sig = temp;
            }
            else
            {
                copia = copia->sig;
            }
        }
    }
    sprintf(mensaje, "Se dejaron de compartir %d archivos del cliente.", archivosEliminados);
    enviarALog(mensaje);
}

void *fc_threads(void *argumentos) //El thread de cada cliente. Atiende las peticiones.
{
    strarg *argumentosThread = (strarg *) argumentos;
    int bytesLeidos;
    int opcion;	//El cliente me envía esta opción, siguiendo el protocolo
    char buffer[64];
    char bufferRutaDescarga[256];
    char buff[65];
    char mensaje [256];
    int leido, clienteRecibe;
    do
    {
        //leo la opcion - 64bytes
        opcion = -1;
        memset(buffer, '\0', 64);
        if ((bytesLeidos = read(argumentosThread->descriptorCliente, buffer, 64)) > 0)
            opcion = atoi(buffer);
        sprintf(mensaje,"%d me envio la opcion: %d", argumentosThread->descriptorCliente, opcion);
        enviarALog(mensaje);
        //dispatcher
        switch (opcion)
        {
        case CLIENTE_ENVIA_COMPARTIDOS:
            enviarALog("Recibo los compartidos");
            recibirCompartidos(argumentosThread->descriptorCliente);
            guardarListaCompartidosEnLog();
            sprintf(mensaje, "La lista de compartidos ahora tiene %d archivos.", contarCantArch());
            enviarALog(mensaje);
            break;
        case CLIENTE_SOLICITA_COMPARTIDOS_Y_CLIENTES:
            enviarALog("Enviando los compartidos al cliente");
            enviarListaCompartidos(cabeza, argumentosThread->descriptorCliente);
            break;
        case CLIENTE_SOLICITA_DESCARGA:
            sprintf(mensaje, "El cliente %d solicita una descarga", argumentosThread->descriptorCliente); //Tengo que validar la ruta que me envíe, solicitarla al otro cliente y quedar a la espera.
            enviarALog(mensaje);
            if(read(argumentosThread->descriptorCliente, bufferRutaDescarga, 256) < 0)
            {
                enviarALog("ERROR:funcionesServer.c:fc_threads:CLIENTE_SOLICITA_DESCARGA:No se pudo obtener la Ruta");
                exit(EXIT_FAILURE);
            }
            if(enviarArchivoCliente(argumentosThread->descriptorCliente, bufferRutaDescarga) == -1)
            {
                enviarALog("ERROR:funcionesServer.c:enviarArchivoCliente:No se encontro el archivo.");
                exit(EXIT_FAILURE);
            }
            break;
        case CLIENTE_ENVIA_ARCHIVO_COMPARTIDO:
            sprintf(mensaje, "El cliente %d me envia un archivo.", argumentosThread->descriptorCliente);
            enviarALog(mensaje);
            leido = read(argumentosThread->descriptorCliente, buff, 64);
            buff[leido]='\0';
            clienteRecibe = atoi(buff);
            sprintf(mensaje, "Archivo Compartido para enviar a: %d", clienteRecibe);
            enviarALog(mensaje);
            copiarArchivo(clienteRecibe, argumentosThread->descriptorCliente);
            break;
        case -1:
            sprintf(mensaje, "El cliente %d se desconecto: eliminandolo de la lista.", argumentosThread->descriptorCliente);
            enviarALog(mensaje);
            borrarClienteDeLaLista(argumentosThread->descriptorCliente);
            eliminarCliente(argumentosThread->descriptorCliente);
            guardarListaCompartidosEnLog();
        }
    }
    while(opcion != -1);
    return NULL;
}

//Cuenta la cantidad de archivos compartidos que hay en la lista.
int contarCantArch ()
{
    arcCmp *regArchivo;
    regArchivo = cabeza;
    int cantidadArchivos = 0;
    while (regArchivo != NULL)
    {
        cantidadArchivos++;
        regArchivo = regArchivo->sig;
    }
    return cantidadArchivos;
}

int guardarListaCompartidosEnLog()
{
    enviarALog("Guardando la lista de compartidos en Log.");
    int descriptorArchivoDisco;
    char bufferCliente[15];
    char bufferNombre[256];
    char bufferTamanio[64];
    char bufferId[256];
    if((descriptorArchivoDisco = open("./logs/listaCompartidosServer.txt", O_CREAT|O_WRONLY|O_TRUNC, 0777)) == -1)
    {
        perror("ERROR:funcionesCliente.c:GuardarEnLog:No se pudo abrir el archivo.");
        exit(EXIT_FAILURE);
    }
    arcCmp *regArc = cabeza;
    while (regArc != NULL)
    {
        sprintf(bufferCliente, "Cliente: %d", regArc->cliente);
        sprintf(bufferNombre, "\t FileName: %s\t", regArc->nom);
        sprintf(bufferTamanio, "Tamaño: %d\t", regArc->tam);
        sprintf(bufferId, "Id: %s\n", regArc->id);
        write(descriptorArchivoDisco, bufferCliente, strlen(bufferCliente));
        write(descriptorArchivoDisco, bufferNombre, strlen(bufferNombre));
        write(descriptorArchivoDisco, bufferTamanio, strlen(bufferTamanio));
        write(descriptorArchivoDisco, bufferId, strlen(bufferId));
        regArc = regArc->sig;
    }
    close(descriptorArchivoDisco);
    return 0;
}

void copiarArchivo(int clienteRecibe, int clienteEnvia)
{
    char buf[256];				//tam del buffer
    char mensaje[256];
    int tamanioArchivoARecibir;
    int totalBytesLeidos = 0;
    int totalBytesEscritos = 0;
    int cantidadLeida, bytesEscritos = 0;

    //Le aviso al cliente que tiene que recibirlo.
    if (enviarOpcion(RECIBIR_ARCHIVO_COMPARTIDO, clienteRecibe) == 1)
    {
        enviarALog("ERROR:ENVIO:OPCION_RECIBIR_ARCHIVO_COMPARTIDO");
        exit(EXIT_FAILURE);
    }

    //Leo el tamaño del archivo.
    if((cantidadLeida = read(clienteEnvia, buf, 255)) == -1)
    {
        enviarALog("ERROR:funcionesServer.c:copiarArchivo:No pude leer el tamaño del archivo.");
        exit(EXIT_FAILURE);
    }
    buf[cantidadLeida] = '\0';
    tamanioArchivoARecibir = atoi(buf);

    sprintf(mensaje, "El tamaño del archivo que estoy copiando es %d bytes.", tamanioArchivoARecibir);
    enviarALog(mensaje);
    //Envia el tamaño a clienteRecibe
    if(write(clienteRecibe, buf, cantidadLeida) == -1)
    {
        enviarALog("ERROR:funcionesServer.c:copiarArchivo:No pude enviar el tamaño del archivo.");
        exit(EXIT_FAILURE);
    }

    //Copio el archivo que me estan enviando, al cliente que debe recibirlo.
    while (totalBytesLeidos < tamanioArchivoARecibir)
    {
        if((cantidadLeida = read(clienteEnvia, buf, 256)) == -1)
        {
            enviarALog("ERROR:funcionesServer.c:copiarArchivo:Error al leer el archivo desde el cliente.");
            exit(EXIT_FAILURE);
        }
        //buf[cantidadLeida]='\0';
        if ((bytesEscritos = write(clienteRecibe, buf, cantidadLeida)) != cantidadLeida)
        {
            enviarALog("ERROR:funcionesServer.c:copiarArchivo:write:Error al enviar archivo recibido.");
            exit(EXIT_FAILURE);
        }
        totalBytesLeidos += cantidadLeida;
        totalBytesEscritos += bytesEscritos;
    }
    sprintf(mensaje, "Termine de copiar un archivo desde %d: Se leyo %d, y se escribio al cliente %d: %d bytes.", clienteEnvia, totalBytesLeidos, clienteRecibe, totalBytesEscritos);
    enviarALog(mensaje);
}

int enviarArchivoCliente(int clienteRecibe, char rutaArchivo[])
{
    cantidadDescargasTotales++;
    char mensaje[256];
    char bufferRuta[256];
    memset(bufferRuta, '\0', 256);
    sprintf(bufferRuta, "%s", rutaArchivo);
    int clienteEnvia;
    if((clienteEnvia = buscarArchivo(rutaArchivo)) == -1)
    {
        return -1;
    }
    else
    {
        if (enviarOpcion(SERVIDOR_SOLICITA_ARCHIVO, clienteEnvia) == 1)
            err_quit("ERROR:ENVIO:OPCION");
        if(write(clienteEnvia, bufferRuta, 256) < 0) //Envio la ruta del archivo que espero que me mande
            err_quit("ERROR:ENVIO:RUTA");
        if (enviarOpcion(clienteRecibe, clienteEnvia) == 1)
            err_quit("ERROR:ENVIO:OPCION");
        sprintf(mensaje, "Le pedi el archivo %s al cliente %d.", rutaArchivo, clienteEnvia);
        enviarALog(mensaje);
        return 0;
    }
}

int buscarArchivo(char rutaArchivo[])  //Devuelve el descriptor del propietario de un archivo
{
    arcCmp *regArc = cabeza;
    while(regArc != NULL && (strcmp(rutaArchivo, regArc->nom) != 0))
    {
        regArc = regArc->sig;
    }
    if (regArc == NULL)
    {
        return -1;
    }
    else
    {
        regArc->cantidadDescargas++;
        return regArc->cliente;
    }

}

int enviarOpcion (int opc, int descriptorSocket)
{
    char buffer[64];
    int bytesEscritos;
    memset(buffer,'\0', 64);
    sprintf(buffer, "%d", opc);
    if((bytesEscritos = write(descriptorSocket, buffer, 64)) < 0)
        return 1;
    else
        return 0;
}

int enviarALog(char *msg)
{
    int logFileDescriptor;
    char buffer[1024];
    char nombreLog[256];
    //fecha
    time_t curtime;
    struct tm *lt;
    curtime = time(NULL);
    lt = localtime(&curtime);
    sprintf(nombreLog, "./logs/logServer%d.log", getpid());
    if((logFileDescriptor = open(nombreLog, O_CREAT|O_APPEND|O_WRONLY, 0600)) == -1)
    {
        err_quit("ERROR:enviarALog:Error al abrir el archivo");
    }
    sprintf(buffer, "%d-%d-%d %d-%d-%d - [%ld]: %s\n", lt->tm_year+1900, lt->tm_mon+1, lt->tm_mday, lt->tm_hour, lt->tm_min, lt->tm_sec, pthread_self(), msg);
    if(write(logFileDescriptor, buffer, strlen(buffer)) == -1)
    {
        err_quit("ERROR:enviarALog:Problema al escribir log.");
    }
    close(logFileDescriptor);
}

int leerConfiguraciones()
{
    int fileDescriptor, bytesLeidos, totalBytesLeidos, j = 0, i = 0;
    char mensaje[256];
    char buffer;
    char campo[16];
    if((fileDescriptor = open("configuraciones.txt", O_RDONLY)) == -1)
    {
        perror("ERROR:funcionesServer.c:leerConfiguraciones:Error al abrir el archivo.");
        enviarALog("ERROR:funcionesServer.c:leerConfiguraciones:Error al abrir el archivo.");
        exit(EXIT_FAILURE);
    }
    while(bytesLeidos > 0 && buffer != ';')
    {
        if((bytesLeidos = read(fileDescriptor, &buffer, 1)) == -1)
        {
            perror("ERROR:funcionesServer.c:leerConfiguraciones:Error al leer del archivo.");
            enviarALog("ERROR:funcionesServer.c:leerConfiguraciones:Error al leer del archivo.");
            exit(EXIT_FAILURE);
        }
        campo[j] = buffer;
        j++;
    }
    campo[j] = '\0';
    cantidadDeConexiones = atoi(campo);
    sprintf(mensaje, "Cantidad maxima de conexiones simultaneas: %d", cantidadDeConexiones);
    enviarALog(mensaje);
    j = 0;
    while(bytesLeidos > 0)
    {
        if((bytesLeidos = read(fileDescriptor, &buffer, 1)) == -1)
        {
            perror("ERROR:funcionesServer.c:leerConfiguraciones:Error al leer del archivo.");
            enviarALog("ERROR:funcionesServer.c:leerConfiguraciones:Error al leer del archivo.");
            exit(EXIT_FAILURE);
        }
        totalBytesLeidos+=bytesLeidos;
        if(buffer == ';')
        {
            campo[j] = '\0';
            strcpy(arrayIpsFiltradas[i], campo);
            sprintf(mensaje, "Se leyo una IP filtrada: %s", arrayIpsFiltradas[i]);
            i++;
            enviarALog(mensaje);
            j = 0;
        }
        else
        {
            campo[j] = buffer;
            j++;
        }
    }
    sprintf(mensaje, "Se leyeron %d bytes del archivo de configuración.", totalBytesLeidos);
    enviarALog(mensaje);
    return 0;
}

int ipFiltrada(char *ipConectada)
{
    int i = 0;
    while(i < 256 && (strcmp(ipConectada, arrayIpsFiltradas[i]) != 0))
        i++;
    if(i < 256)
        return -1;
    else
        return 0;
}
