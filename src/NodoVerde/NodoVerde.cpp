#include <iostream>
#include <map>            // mapa
#include <vector>         // vector
#include <utility>        // par ordenado
#include <unistd.h>       // fork()
#include <sys/stat.h>     // IPC pipes
#include <fcntl.h>        // open, close, O_WRONLY, O_RDONLY
#include <thread>         // Posix threads
#include <semaphore.h>    // Semaforos
#include <bits/stdc++.h>  // stringstream
#include <string>         // string
#include <fstream>        // ofstream
#include <ctime>          // time
#include <string.h>
#include <iomanip>        // iostreams
#include <sys/types.h>    //socket UDP
#include <sys/socket.h>   //socket UDP
#include <arpa/inet.h>    //socket UDP
#include <netinet/in.h>   //socket UDP

#include "NodoVerde.h"


//--------------------------Implementación de Funciones---------------------------

/**
\brief Capturar mensajes de error o eventos.
\details Toma el mensaje recibido y lo escribe, junto con la fecha, en el archivo logNodoVerde.txt
\param[in] Un mensaje tipo string.
\author Sebastián Alfaro León
\date 7\09\2020
*/
void logger(std::string modulo,std::string message){
  std::ofstream log;
  log.open("logNodoVerde.tsv",std::ios::app);
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  log << std::put_time(&tm, "%d-%m-%Y %H-%M-%S")<< "\t" <<modulo <<"\t" << message << std::endl;
  log.close();
}

/**
\brief Función para obtener cantidad de vecinos.
\details Esta función retorna la cantidad de vecinos preguntando
\ por el tamaño del vector que contiene los ID y IP de los vecinos.
return  Retorna un entero, cantidad de vecinos.
\author Sebastián Alfaro León
\date 8/9/2020
*/
int uv_ObtenerCantidadVecinos(){
  return vecinos_id_ip.size();
}

/**
\brief Metodo que crea un prceso hijo corre un script de python para pasarle informacion mediante un pipe al proceso padre.
\details Este metodo utiliza un fork() para crear un proceso hijo que posteriormente ejecutara un script de python,
se utiliza un pipe FIFO del sistema operativo para comunicar el proceso padre con el proceso hijo y asi poder
pasarse la informacion contenida en los archivos misDatos.csv y misVecinos.csv linea por linea.
Al pasarse una linea del proceso hijo (Nodo naranja) al proceso padre, esta se envia a procesa  utilizando un
tokenizer.
\author Alejandro Duarte Lobo
\date 8/9/2020
*/
void pn_IntanciarNodoNaranja(){
  int ID_proceso;
  int readError;
  int pipeDeEscritura;
  int pipeDeLectura;
  char nombrePipeNaranja[] = "./../src/NodoNaranja/fifoNaranja";
  std::vector <std::string> tokens;
  std::string intermedio;
  char bufferNaranja[100];
  ID_proceso = fork();//crea el fork
  if (ID_proceso < 0){
    //TODO:Escribe en log
    exit(-1);
  }else if (ID_proceso == 0){
    execlp("python3", "python3", "./../src/NodoNaranja/NodoNaranja.py", NULL);
  }else{
    usleep(1*1000000);//sincroniza con el NodoNaranja(se asegura de que inicie primero)

    if ((pipeDeEscritura = open(nombrePipeNaranja, O_WRONLY)) < 0){
      //TODO:Escribe en log
      exit(-1);
    }else{
      char bufferDeEscritura[] = "1";
      write(pipeDeEscritura, bufferDeEscritura, sizeof(bufferDeEscritura));//envia el signal 1 por el pipe
      close(pipeDeEscritura);
    }

    pipeDeLectura = open(nombrePipeNaranja, O_RDONLY);
    readError = read(pipeDeLectura, bufferNaranja, sizeof(bufferNaranja));//lee el pipe
    if(readError<0){
      //TODO:Escribe en log
      exit(-1);
    }
    close(pipeDeLectura);
    std::string linea=ug_convertirString(bufferNaranja,readError);
    std::stringstream ss(linea);
    while(getline(ss, intermedio, ',')){
      tokens.push_back(intermedio);
    }
    std::cout<<"proceso mis datos:"<<std::endl;
    miID=stoi(tokens[0]);
    miPuerto=stoi(tokens[1]);
    std::cout<<"id:"<<miID <<", puerto:"<<miPuerto<<std::endl;
    tokens.clear();

    if ((pipeDeEscritura = open(nombrePipeNaranja, O_WRONLY)) < 0){
      //TODO:Escribe en log
      exit(-1);
    }
    else{
      char bufferDeEscritura[] = "2";
      write(pipeDeEscritura, bufferDeEscritura, sizeof(bufferDeEscritura));//envia un 2 por el pipe
      close(pipeDeEscritura);
    }

    while (true){
      pipeDeLectura = open(nombrePipeNaranja, O_RDONLY);//abre el pipe para leer
      //lee el contenido del pipe, contiene 1 vecino o "salir"
      readError = read(pipeDeLectura, bufferNaranja, sizeof(bufferNaranja));
      if(readError<0){
        //TODO:Escribe en log
        exit(-1);
      }
      close(pipeDeLectura);
      if (strcmp(bufferNaranja, "salir") == 0){
        close(pipeDeLectura);
        break;
      }
      else{
        std::string linea=ug_convertirString(bufferNaranja,readError);
        std::stringstream ss(linea);
        while(getline(ss, intermedio, ',')){
          tokens.push_back(intermedio);
        }
        std::cout<<"proceso un vecino:"<<std::endl;
        std::cout<<"id:"<<tokens[0] <<", ip:"<<tokens[1]<<", puerto:"<<tokens[2]<<std::endl;
        un_InsertarIPVecinos(std::make_pair(stoi(tokens[0]), tokens[1]));
        un_InsertarPuertoVecinos(std::make_pair(stoi(tokens[0]), stoi(tokens[2])));
        pipeDeEscritura = open(nombrePipeNaranja, O_WRONLY);
        char bufferDeEscritura[] = "3";//envia un 3 a naranja, que esta en modo espera, para continuar
        write(pipeDeEscritura, bufferDeEscritura, sizeof(bufferDeEscritura));
        close(pipeDeEscritura);
        tokens.clear();
      }
      fsync(pipeDeLectura);
    }
  }
  sem_post(&semNaranja);
};

/**
\brief Función que guarda el ID y IP de los vecinos.
\details Esta función recibe un par ordenado, el id y ip de los vecinos,
\ y lo inserta en el vector que contiene ID e Ip de los vecinos.
\param[in] Un par ordenado que contiene un int(ID) y una string(IP).
\author Sebastián Alfaro León
\date 8/09/2020
*/
void un_InsertarIPVecinos(std::pair<int,std::string>id_IP){
  vecinos_id_ip.push_back(id_IP);
};

/**
\brief Función que guarda el ID y Puerto de los vecinos.
\details Esta función recibe un par ordenado, el id y puerto de los vecinos,
 y lo inserta en el vector que contiene los id y puertos de los vecinos.
\param[in] Un par ordenado que contiene un dos int(ID,Puerto).
\author Sebastián Alfaro León
\date 8/09/2020
*/
void un_InsertarPuertoVecinos(std::pair<int,int>id_puerto){
  vecinos_id_puerto.push_back(id_puerto);
};

/**
\brief Función para obtener el IP de un determinado vecino.
\details Esta función recibe una posción del vector que contiene los IP de los vecinos
\ y extrae el IP de esa posición para devolverla
\param[in] Un int, la posicion en el vector de IP.
return  Retorna un string, el  IP del vecino.
\author Sebastián Alfaro León
\date 8/09/2020
*/
std::string uv_ObtenerIPVecino(int posicion){
  return std::get<1>(vecinos_id_ip[posicion]);
}

/**
\brief Función para obtener el Puerto de un determinado vecino.
\details Esta función recibe una posción del vector que contiene los Puerto de los vecinos
\ y extrae el Puerto de esa posición para devolverla
\param[in] Un int, la posicion en el vector de Puerto.
return  Retorna un entero, el número de puerto del vecino.
\author Sebastián Alfaro León
\date 8/09/2020
*/
int uv_ObtenerPuertoVecino(int posicion){
  return std::get<1>(vecinos_id_puerto[posicion]);
}

/**
\brief Función para obetener el Id de un determinado vecino.
\\details Esta función recibe una posción del vector  del que se desea obtener el ID
\param[in] Un int, la posicion en el vector de Puerto.
\author Sebastián Alfaro León
\date 8/09/2020
*/
int uv_ObtenerIDVecinos(int posicion){
  return std::get<0>(vecinos_id_puerto[posicion]);
}

/**
\brief Función para almacenar paquetes entrantes, provenientes de los vecinos.
\details Esta función recibe un paquete proveniente del algún vecino, con destino hacia el nodo azul,
\ y lo almacena en una cola de paquetes entrantes.
\param[in] Una string, el paquete entrante.
\author Sebastián Alfaro León
\date 8/09/2020
*/
void uv_InsertarPaqueteEntrada(std::string paqueteEntrada){
  entrada.push(paqueteEntrada);
}

/**
\brief Función para almacenar paquetes de salida, con destino hacia los vecinos.
\details Esta función recibe un paquete proveniente del nodo azul, con destino hacia el nodo verde,
\ y lo almacena en una cola de paquetes salientes.
\param[in] Una string, el paquete de salida.
\author Sebastián Alfaro León
\date 8/09/2020
*/
void pa_InsertarPaqueteSalida(std::string paqueteSalida){
  salida.push(paqueteSalida);
}

/**
\brief Función para obtener un paquete de salida, con destino hacia los vecinos.
\details Esta función consulta en la cola de salida el paquete próximo a salir, lo saca de la cola
\ y lo retorna.
\return Retorna una string, que es el paquete de salida.
\author Sebastián Alfaro León
\date 8/09/2020
*/
std::string uv_ObtenerPaqueteSalida(){
  std::string s = "";
  if(salida.size() != 0){
     s = salida.front();
     salida.pop();
  }
  return s;
};

/**
\brief Función para obtener un paquete de entrada, proveniente de los vecinos.
\details Esta función consulta en la cola de entrada el paquete entrante, lo saca de la cola
\ y lo retorna.
\return Retorna una string, que es el paquete de entrada.
\author Sebastián Alfaro León
\date 8/09/2020
*/
std::string pa_ObtenerPaqueteEntrada(){
  std::string e = "";
  if(entrada.size() != 0){
     e = entrada.front();
     entrada.pop();
  }
  return e;
};



/**
\brief Función para almacenar paquetes de salida, con destino hacia los vecinos.
\details Esta función recibe un paquete proveniente del nodo azul, con destino hacia el nodo verde,
\ y lo almacena en una cola de paquetes salientes.
\param[in] Una string, el paquete de salida.
\author Sebastián Alfaro León
\date 8/09/2020
*/
void ua_InsertarPaqueteSalida(std::string paqueteSalida){
  salida.push(paqueteSalida);
}




/**
\brief Función para obtener un paquete de entrada, proveniente de los vecinos.
\details Esta función consulta en la cola de entrada el paquete entrante, lo saca de la cola
\ y lo retorna.
\return Retorna una string, que es el paquete de entrada.
\author Sebastián Alfaro León
\date 8/09/2020
*/
std::string ua_ObtenerPaqueteEntrada(){
  std::string e = "";
  if(entrada.size() != 0){
     e = entrada.front();
     entrada.pop();
  }
  return e;
};


/**
\brief Método parte del agente azul encargado de recibir paquetes del nodo azul.
\details Este método extrae paquetes del pipe cuando se le informa por semáforos que hay información
y la almacena en una cola de salida usando un método utilitario.
\author Daniel Henao.
\date 11/09/20.
*/
void ha_InstanciarAgenteAzulEmisor(){
  int readError;
  int pipeDeLectura;
  int pipeDeEscritura;
  char pipeAzulEmisor[] = "./../bin/IPC/NodoAzul/fifoReceptorAzul";
  // **** Envio de ID a nodo Azul ****
  char intermediario[3];
  char paqueteMiID[] = "";
  sprintf(intermediario, "%d", miID);
  strcat(paqueteMiID,intermediario);
  if ((pipeDeEscritura = open(pipeAzulEmisor, O_WRONLY)) < 0) {
    logger("Agente Azul","Error abriendo pipe para escritura.");
    exit(-1);
  } else {
    write(pipeDeEscritura, paqueteMiID, sizeof(paqueteMiID));//Envia mi ID
    close(pipeDeEscritura);
  }
  // **** Envia mensajes recibidos a nodo azul ****
  while (true){
    usleep(2000000); //Por mientras
    //char bufferDeEscritura[] = ua_ObtenerPaqueteEntrada().c_str();
    char bufferDeEscritura[] = "0201Exito!";
    if ((pipeDeEscritura = open(pipeAzulEmisor, O_WRONLY)) < 0) {
      logger("Agente Azul","Error abriendo pipe para escritura.");
      exit(-1);
    } else {
      write(pipeDeEscritura, bufferDeEscritura, sizeof(bufferDeEscritura));
      close(pipeDeEscritura);
    }
    // Recibo de signal para coordinacion
    if ((pipeDeLectura = open(pipeAzulEmisor, O_RDONLY)) < 0) {
      logger("Agente Azul","Error abriendo pipe para lectura de signal.");
      exit(-1);
    }
    else {
      //lee un signal para coordinacion
      readError = read(pipeDeLectura, bufferDeEscritura, sizeof(bufferDeEscritura));
      close(pipeDeLectura);
      if(readError<0) {
        logger("Agente Azul","Error leyendo signal.");
        exit(-1);
      }
    }
  }
}

/**
\brief Método parte del agente azul encargado de recibir paquetes del nodo azul.

\details Este método extrae paquetes del pipe verifica si tiene un destino
valido y en ese caso la almacena en una cola de salida usando un método
utilitario. Despierta al hilo encargado de enviar.
\author Daniel Henao
\date 11/09/20
*/
void ha_InstanciarAgenteAzulReceptor() {
  int readError;
  int pipeDeLectura;
  int pipeDeEscritura;
  char pipeAzulReceptor[] = "./../bin/IPC/NodoAzul/fifoEmisorAzul";
  char bufferDeLectura[204]; //Parametrizar
  // **** Recibo de mensajes salientes del nodo azul ****
  while (true) {
    if ((pipeDeLectura = open(pipeAzulReceptor, O_RDONLY)) < 0) {
      logger("Agente Azul","Error abriendo pipe para lectura.");
      exit(-1);
    } else {
      readError = read(pipeDeLectura, bufferDeLectura, sizeof(bufferDeLectura));
      close(pipeDeLectura);
      if(readError<0){
        logger("Modulo Azul","Error leyendo paquete de nodo azul.");
        exit(-1);
      }
    }
    std::string paquete = ug_convertirString(bufferDeLectura,readError);
    std::cout<<"Se Recibio: "<<paquete<<std::endl;
    if (strcmp(paquete.substr(4,5).c_str(), "salir") == 0){
      std::cout << "Finalizado agente receptor!" << std::endl;
      break;
    }
    // **** Envio de signal para coordinacion ****
    if ((pipeDeEscritura = open(pipeAzulReceptor, O_WRONLY)) < 0) {
      logger("Agente Azul","Error abriendo pipe para lectura.");
      exit(-1);
    } else {
      char bufferDeEscritura[] = "signal";//envia mensaje para coordinar
      write(pipeDeEscritura, bufferDeEscritura, sizeof(bufferDeEscritura));
      close(pipeDeEscritura);
    }
    // *** Almacenamiento de paquetes ****
    std::string id = paquete.substr (2,2);
    std::cout << "ID destino: "<<id<< std::endl;
    int indiceCliente = 2; //uv_ObtenerIndiceCliente(id);
    if (indiceCliente == -1) {
      logger("Modulo Azul","Error Paquete con destino invalido.");
    }
    else {
      candadoColaSaliente.lock();
      ua_InsertarPaqueteSalida(paquete);
      candadoColaSaliente.unlock();
      //sem_post(&semClientes[indiceCliente]);
    }
  }
}

/**
\brief Método que crea un proceso hijo para ejecutar un programa de python que se comunicará por pipes.

\details Este método invoca al nodo azul y crea 2 pipes FIFO para comunicarse con él, el proceso padre
crea hilos encargados de recibir y enviar paquetes por los pipe pipes respectivos.
\author Daniel Henao.
\date 11/09/20.
*/
void ha_InstanciarNodoAzul() {
  pid_t ID_proceso;
  ID_proceso = fork();//crea el fork
  if (ID_proceso < 0) {
    logger("Modulo Azul","Error instanciando el nodo Azul.");
    exit(-1);
  } else if (ID_proceso == 0) {
    execlp("python3", "python3", "./../src/NodoAzul/NodoAzul.py", NULL);
  } else {
    usleep(1*1000000);//sincroniza con el NodoAzul(se asegura de que inicie primero)
    std::thread agenteAzulReceptor(ha_InstanciarAgenteAzulReceptor);
    std::thread agenteAzulEmisor(ha_InstanciarAgenteAzulEmisor);
    agenteAzulEmisor.join();
    agenteAzulReceptor.join();
  }
}

/**
\brief Metodo que convierte un arreglo de caracteres un un string de C++

\details Este metodo convierte un arreglo de caracteres en un string de C++ concatenando cada uno de los
caracteres individuales del arreglo en el string.

\param[in] arr Puntero al primer elemento de un arreglo de caracteres
\param[in] tamano Corresponde al tamano del arreglo de caracteres
\return linea Corresponde al string creado a partir de concatenar los elementos del arreglo.
\author Alejandro Duarte Lobo
\date 8/09/2020
*/
std::string ug_convertirString(char* arr, int tamano){
  std::string linea="";
  for(int i=0; i<tamano; i++){
    linea=linea+arr[i];
  }
  return linea;
}

/**
\brief Establece una conexion UDP con un vecino verde y envia mensajes
\details Recibe por parametro los datos de un vecino del vector de vecinos  y establece una conexion
con el servidor vecino. Una vez conectado, este cliente puede enviar mensajes
a ese servidor.
\param[in] int de puerto destino
\param[in] string de IP destino
\param[in] int de posicion en el vector de semaforos para dormir/despertar
\author Jostyn Delgado Segura
\date 8\09\2020
*/
void uv_EstablecerConexionCliente(int puerto_destino, std::string ip_destino, int pos){

  struct sockaddr_in servaddr={0};//opciones de socket igual que en el server y el from de antes
  int sockfd = socket(AF_INET, SOCK_DGRAM,0);//se crea el socket igual que antes
  if(sockfd<0){
    logger("Modulo Verde","Error creacion socket cliente");
  }
  servaddr.sin_family=AF_INET;//ipv4
  servaddr.sin_port=htons(puerto_destino);//puerto little endian
  servaddr.sin_addr.s_addr=inet_addr(ip_destino.c_str());//ip

  while(finalizar){
    sem_wait(&semClientes[pos]);//duerme el hilo hasta que tenga un paquete
    candadoColaSaliente.lock();
    std::string mensaje = uv_ObtenerPaqueteSalida();
    candadoColaSaliente.unlock();
    const char * msg = (mensaje).c_str();//para mandarlo por el socket
    int len=sendto(sockfd,(const char *)msg, strlen(msg),0,(const struct sockaddr *)&servaddr, sizeof(servaddr));
    if (len <1){
      logger("Modulo Verde","Error de envio mensaje");
    }
  }
}
/**
\brief Abre un socket servidor UDP
\details Crea un socket UDP servidor que se encarga de escuchar los
mensajes provenientes de los clientes vecinos con el puerto asgignado a este nodo verde
a ese servidor.
\author Jostyn Delgado Segura
\date 8\09\2020
*/
void uv_EstablecerServidor(){
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);//define un socket ipv4 UDP con los parametros
  int length, fromlen;//tamanos que se usan en otros parametros
  struct sockaddr_in server;//structs que contienen ip puerto y otras opciones este es el propio
  struct sockaddr_in from;// este es de donde recibe mensajes
  length = sizeof(server);
  bzero(&server, length);
  server.sin_family = AF_INET;//opcion ipv4
  server.sin_addr.s_addr =INADDR_ANY; //inet_addr(myIP.c_str());//esto es para decir cual es la IP
  server.sin_port = htons(miPuerto);//esto es para decir cual es el puerto htons es para pasar de big a little endian
  if (bind(sockfd, (struct sockaddr *)&server, length) < 0){
    logger("Modulo Verde","Error creacion socket servidor");
  }
  fromlen = sizeof(struct sockaddr_in);
  char buffer[200];//buffer donde recibe el mensaje
  while(finalizar){
    memset(buffer,0,sizeof(buffer));//se pone el buffer en blanco
    recvfrom(sockfd, &buffer, sizeof(buffer), 0, (struct sockaddr *)&from, (socklen_t *)&fromlen);// recibe mensaje en buffer

    candadoColaEntrante.lock();
    uv_InsertarPaqueteEntrada(buffer);
    candadoColaEntrante.unlock();
  }
}
//
//------------------------------------------------------------------------------

int main() {

  finalizar = false;

  sem_init(&semNaranja,0,0);
  std::thread agenteNaranja(pn_IntanciarNodoNaranja);
  sem_wait(&semNaranja);

  std::thread agenteAzul(ha_InstanciarNodoAzul);
  //semaforo?

/*
  std::thread servidorUDP(uv_EstablecerServidor);
  std::vector<std::thread> clientesUDP;
  semClientes.resize(uv_ObtenerCantidadVecinos());

  for (int i = 0; i < uv_ObtenerCantidadVecinos(); i++){
    clientesUDP.push_back(std::thread(uv_EstablecerConexionCliente,uv_ObtenerPuertoVecino(i),uv_ObtenerIPVecino(i),i));
    sem_init(&semClientes[i],0,0);
    clientes_id_indice.insert(std::pair<int,int>(uv_ObtenerIDVecinos(i),i));
  }
*/
  agenteNaranja.join();
  agenteAzul.join();
  /*servidorUDP.join();
  for (int i = 0; i < int(clientesUDP.size()); i++){
    clientesUDP[i].join();
  }*/
}
